/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: psychoacoustics not including preecho
 last mod: $Id$

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

#define NEGINF -9999.f

/*
  rephase   = reverse phase limit (postpoint)
                                                0    1    2    3    4    5    6    7    8  */
static double stereo_threshholds[]=           {0.0, 0.5, 1.0, 1.5, 2.5, 4.5, 8.5,16.5, 9e10};
static double stereo_threshholds_rephase[]=   {0.0, 0.5, 0.5, 1.0, 1.5, 1.5, 2.5, 2.5, 9e10};

static double stereo_threshholds_low[]=       {0.0, 0.5, 0.5, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0};
static double stereo_threshholds_high[]=      {0.0, 0.5, 0.5, 0.5, 1.0, 3.0, 5.5, 8.5, 0.0};


static int m3n32[] = {21,13,10,4};
static int m3n44[] = {15,9,7,3};
static int m3n48[] = {14,8,6,3};

static int temp_bfn[128] = {
 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 6, 7, 7, 7, 7,
 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,
12,12,12,12,13,13,13,13,14,14,14,14,15,15,15,15,
16,16,16,16,17,17,17,17,18,18,18,18,19,19,19,19,
20,20,20,20,21,21,21,21,22,22,22,22,23,23,23,23,
24,24,24,24,25,25,25,24,23,22,21,20,19,18,17,16,
15,14,13,12,11,10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0,
};

static float nnmid_th=0.2;


vorbis_look_psy_global *_vp_global_look(vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  vorbis_look_psy_global *look=_ogg_calloc(1,sizeof(*look));

  look->channels=vi->channels;

  look->ampmax=-9999.;
  look->gi=gi;
  return(look);
}

void _vp_global_free(vorbis_look_psy_global *look){
  if(look){
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

void _vi_gpsy_free(vorbis_info_psy_global *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

static void min_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(float *c,float att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static float ***setup_tone_curves(float curveatt_dB[P_BANDS],float binHz,int n,
				  float center_boost, float center_decay_rate){
  int i,j,k,m;
  float ath[EHMER_MAX];
  float workc[P_BANDS][P_LEVELS][EHMER_MAX];
  float athc[P_LEVELS][EHMER_MAX];
  float *brute_buffer=alloca(n*sizeof(*brute_buffer));

  float ***ret=_ogg_malloc(sizeof(*ret)*P_BANDS);

  memset(workc,0,sizeof(workc));

  for(i=0;i<P_BANDS;i++){
    /* we add back in the ATH to avoid low level curves falling off to
       -infinity and unnecessarily cutting off high level curves in the
       curve limiting (last step). */

    /* A half-band's settings must be valid over the whole band, and
       it's better to mask too little than too much */  
    int ath_offset=i*4;
    for(j=0;j<EHMER_MAX;j++){
      float min=999.;
      for(k=0;k<4;k++)
	if(j+k+ath_offset<MAX_ATH){
	  if(min>ATH[j+k+ath_offset])min=ATH[j+k+ath_offset];
	}else{
	  if(min>ATH[MAX_ATH-1])min=ATH[MAX_ATH-1];
	}
      ath[j]=min;
    }

    /* copy curves into working space, replicate the 50dB curve to 30
       and 40, replicate the 100dB curve to 110 */
    for(j=0;j<6;j++)
      memcpy(workc[i][j+2],tonemasks[i][j],EHMER_MAX*sizeof(*tonemasks[i][j]));
    memcpy(workc[i][0],tonemasks[i][0],EHMER_MAX*sizeof(*tonemasks[i][0]));
    memcpy(workc[i][1],tonemasks[i][0],EHMER_MAX*sizeof(*tonemasks[i][0]));
    
    /* apply centered curve boost/decay */
    for(j=0;j<P_LEVELS;j++){
      for(k=0;k<EHMER_MAX;k++){
	float adj=center_boost+abs(EHMER_OFFSET-k)*center_decay_rate;
	if(adj<0. && center_boost>0)adj=0.;
	if(adj>0. && center_boost<0)adj=0.;
	workc[i][j][k]+=adj;
      }
    }

    /* normalize curves so the driving amplitude is 0dB */
    /* make temp curves with the ATH overlayed */
    for(j=0;j<P_LEVELS;j++){
      attenuate_curve(workc[i][j],curveatt_dB[i]+100.-(j<2?2:j)*10.-P_LEVEL_0);
      memcpy(athc[j],ath,EHMER_MAX*sizeof(**athc));
      attenuate_curve(athc[j],+100.-j*10.f-P_LEVEL_0);
      max_curve(athc[j],workc[i][j]);
    }

    /* Now limit the louder curves.
       
       the idea is this: We don't know what the playback attenuation
       will be; 0dB SL moves every time the user twiddles the volume
       knob. So that means we have to use a single 'most pessimal' curve
       for all masking amplitudes, right?  Wrong.  The *loudest* sound
       can be in (we assume) a range of ...+100dB] SL.  However, sounds
       20dB down will be in a range ...+80], 40dB down is from ...+60],
       etc... */
    
    for(j=1;j<P_LEVELS;j++){
      min_curve(athc[j],athc[j-1]);
      min_curve(workc[i][j],athc[j]);
    }
  }

  for(i=0;i<P_BANDS;i++){
    int hi_curve,lo_curve,bin;
    ret[i]=_ogg_malloc(sizeof(**ret)*P_LEVELS);

    /* low frequency curves are measured with greater resolution than
       the MDCT/FFT will actually give us; we want the curve applied
       to the tone data to be pessimistic and thus apply the minimum
       masking possible for a given bin.  That means that a single bin
       could span more than one octave and that the curve will be a
       composite of multiple octaves.  It also may mean that a single
       bin may span > an eighth of an octave and that the eighth
       octave values may also be composited. */
    
    /* which octave curves will we be compositing? */
    bin=floor(fromOC(i*.5)/binHz);
    lo_curve=  ceil(toOC(bin*binHz+1)*2);
    hi_curve=  floor(toOC((bin+1)*binHz)*2);
    if(lo_curve>i)lo_curve=i;
    if(lo_curve<0)lo_curve=0;
    if(hi_curve>=P_BANDS)hi_curve=P_BANDS-1;

    for(m=0;m<P_LEVELS;m++){
      ret[i][m]=_ogg_malloc(sizeof(***ret)*(EHMER_MAX+2));
      
      for(j=0;j<n;j++)brute_buffer[j]=999.;
      
      /* render the curve into bins, then pull values back into curve.
	 The point is that any inherent subsampling aliasing results in
	 a safe minimum */
      for(k=lo_curve;k<=hi_curve;k++){
	int l=0;

	for(j=0;j<EHMER_MAX;j++){
	  int lo_bin= fromOC(j*.125+k*.5-2.0625)/binHz;
	  int hi_bin= fromOC(j*.125+k*.5-1.9375)/binHz+1;
	  
	  if(lo_bin<0)lo_bin=0;
	  if(lo_bin>n)lo_bin=n;
	  if(lo_bin<l)l=lo_bin;
	  if(hi_bin<0)hi_bin=0;
	  if(hi_bin>n)hi_bin=n;

	  for(;l<hi_bin && l<n;l++)
	    if(brute_buffer[l]>workc[k][m][j])
	      brute_buffer[l]=workc[k][m][j];
	}

	for(;l<n;l++)
	  if(brute_buffer[l]>workc[k][m][EHMER_MAX-1])
	    brute_buffer[l]=workc[k][m][EHMER_MAX-1];

      }

      /* be equally paranoid about being valid up to next half ocatve */
      if(i+1<P_BANDS){
	int l=0;
	k=i+1;
	for(j=0;j<EHMER_MAX;j++){
	  int lo_bin= fromOC(j*.125+i*.5-2.0625)/binHz;
	  int hi_bin= fromOC(j*.125+i*.5-1.9375)/binHz+1;
	  
	  if(lo_bin<0)lo_bin=0;
	  if(lo_bin>n)lo_bin=n;
	  if(lo_bin<l)l=lo_bin;
	  if(hi_bin<0)hi_bin=0;
	  if(hi_bin>n)hi_bin=n;

	  for(;l<hi_bin && l<n;l++)
	    if(brute_buffer[l]>workc[k][m][j])
	      brute_buffer[l]=workc[k][m][j];
	}

	for(;l<n;l++)
	  if(brute_buffer[l]>workc[k][m][EHMER_MAX-1])
	    brute_buffer[l]=workc[k][m][EHMER_MAX-1];

      }


      for(j=0;j<EHMER_MAX;j++){
	int bin=fromOC(j*.125+i*.5-2.)/binHz;
	if(bin<0){
	  ret[i][m][j+2]=-999.;
	}else{
	  if(bin>=n){
	    ret[i][m][j+2]=-999.;
	  }else{
	    ret[i][m][j+2]=brute_buffer[bin];
	  }
	}
      }

      /* add fenceposts */
      for(j=0;j<EHMER_OFFSET;j++)
	if(ret[i][m][j+2]>-200.f)break;  
      ret[i][m][0]=j;
      
      for(j=EHMER_MAX-1;j>EHMER_OFFSET+1;j--)
	if(ret[i][m][j+2]>-200.f)
	  break;
      ret[i][m][1]=j;

    }
  }

  return(ret);
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
		  vorbis_info_psy_global *gi,int n,long rate){
  long i,j,lo=-99,hi=1;
  long maxoc;
  memset(p,0,sizeof(*p));

  p->eighth_octave_lines=gi->eighth_octave_lines;
  p->shiftoc=rint(log(gi->eighth_octave_lines*8.f)/log(2.f))-1;

  p->firstoc=toOC(.25f*rate*.5/n)*(1<<(p->shiftoc+1))-gi->eighth_octave_lines;
  maxoc=toOC((n+.25f)*rate*.5/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;
  p->ath=_ogg_malloc(n*sizeof(*p->ath));

  p->octave=_ogg_malloc(n*sizeof(*p->octave));
  p->bark=_ogg_malloc(n*sizeof(*p->bark));
  p->vi=vi;
  p->n=n;
  p->rate=rate;

  /* AoTuV HF weighting etc. */
  p->n25p=n/4;
  p->n33p=n/3;
  p->n75p=n*3/4;
  if(rate < 26000){
  	/* below 26kHz */
  	p->m_val = 0;
  	for(i=0; i<4; i++) p->m3n[i] = 0;
  	p->tonecomp_endp=0; // dummy
  	p->tonecomp_thres=.25;
  	p->st_freqlimit=n;
  	p->min_nn_lp=0;
  }else if(rate < 38000){
  	/* 32kHz */
  	p->m_val = .93;
  	for(i=0; i<4; i++) p->m3n[i] = m3n32[i];
  	if(n==128)      { p->tonecomp_endp= 124; p->tonecomp_thres=.5;
  	                 p->st_freqlimit=n; p->min_nn_lp=   0;}
  	else if(n==256) { p->tonecomp_endp= 248; p->tonecomp_thres=.7;
  	                 p->st_freqlimit=n; p->min_nn_lp=   0;}
  	else if(n==1024){ p->tonecomp_endp= 992; p->tonecomp_thres=.5;
  	                 p->st_freqlimit=n; p->min_nn_lp= 832;}
  	else if(n==2048){ p->tonecomp_endp=1984; p->tonecomp_thres=.7;
  	                 p->st_freqlimit=n; p->min_nn_lp=1664;}
  }else if(rate > 46000){
  	/* 48kHz */
  	p->m_val = 1.205;
  	for(i=0; i<4; i++) p->m3n[i] = m3n48[i];
  	if(n==128)      { p->tonecomp_endp=  83; p->tonecomp_thres=.5;
  	                 p->st_freqlimit=  89; p->min_nn_lp=   0;}
  	else if(n==256) { p->tonecomp_endp= 166; p->tonecomp_thres=.7;
  	                 p->st_freqlimit= 178; p->min_nn_lp=   0;}
  	else if(n==1024){ p->tonecomp_endp= 664; p->tonecomp_thres=.5;
  	                 p->st_freqlimit= 712; p->min_nn_lp= 576;}
  	else if(n==2048){ p->tonecomp_endp=1328; p->tonecomp_thres=.7;
  	                 p->st_freqlimit=1424; p->min_nn_lp=1152;}
  }else{
  	/* 44.1kHz */
  	p->m_val = 1.;
  	for(i=0; i<4; i++) p->m3n[i] = m3n44[i];
  	if(n==128)      { p->tonecomp_endp=  90; p->tonecomp_thres=.5;
  	                 p->st_freqlimit=  96; p->min_nn_lp=   0;}
  	else if(n==256) { p->tonecomp_endp= 180; p->tonecomp_thres=.7;
  	                 p->st_freqlimit= 192; p->min_nn_lp=   0;}
  	else if(n==1024){ p->tonecomp_endp= 720; p->tonecomp_thres=.5;
  	                 p->st_freqlimit= 768; p->min_nn_lp= 608;}
  	else if(n==2048){ p->tonecomp_endp=1440; p->tonecomp_thres=.7;
  	                 p->st_freqlimit=1536; p->min_nn_lp=1216;}
  }

  /* set up the lookups for a given blocksize and sample rate */

  for(i=0,j=0;i<MAX_ATH-1;i++){
    int endpos=rint(fromOC((i+1)*.125-2.)*2*n/rate);
    float base=ATH[i];
    if(j<endpos){
      float delta=(ATH[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
        p->ath[j]=base+100.;
        base+=delta;
      }
    }
  }

  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i); 

    for(;lo+vi->noisewindowlomin<i && 
	  toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);
    
    for(;hi<=n && (hi<i+vi->noisewindowhimin ||
	  toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);
    
    p->bark[i]=((lo-1)<<16)+(hi-1);

  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i+.25f)*.5*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=setup_tone_curves(vi->toneatt,rate*.5/n,n,
				  vi->tone_centerboost,vi->tone_decay);
  
  /* set up rolling noise median */
  p->noiseoffset=_ogg_malloc(P_NOISECURVES*sizeof(*p->noiseoffset));
  for(i=0;i<P_NOISECURVES;i++)
    p->noiseoffset[i]=_ogg_malloc(n*sizeof(**p->noiseoffset));
  
  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5)*rate/(2.*n))*2.;
    int inthalfoc;
    float del;
    
    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;
    
    for(j=0;j<P_NOISECURVES;j++)
      p->noiseoffset[j][i]=
	p->vi->noiseoff[j][inthalfoc]*(1.-del) + 
	p->vi->noiseoff[j][inthalfoc+1]*del;
    
  }
#if 0
  {
    static int ls=0;
    _analysis_output_always("noiseoff0",ls,p->noiseoffset[0],n,1,0,0);
    _analysis_output_always("noiseoff1",ls,p->noiseoffset[1],n,1,0,0);
    _analysis_output_always("noiseoff2",ls++,p->noiseoffset[2],n,1,0,0);
  }
#endif
}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->bark)_ogg_free(p->bark);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  _ogg_free(p->tonecurves[i][j]);
	}
	_ogg_free(p->tonecurves[i]);
      }
      _ogg_free(p->tonecurves);
    }
    if(p->noiseoffset){
      for(i=0;i<P_NOISECURVES;i++){
        _ogg_free(p->noiseoffset[i]);
      }
      _ogg_free(p->noiseoffset);
    }
    memset(p,0,sizeof(*p));
  }
}

/* octave/(8*eighth_octave_lines) x scale and dB y scale */
static void seed_curve(float *seed,
		       const float **curves,
		       float amp,
		       int oc, int n,
		       int linesper,float dBoffset){
  int i,post1;
  int seedptr;
  const float *posts,*curve;

  int choice=(int)((amp+dBoffset-P_LEVEL_0)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-EHMER_OFFSET)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

static void seed_loop(vorbis_look_psy *p,
		      const float ***curves,
		      const float *f, 
		      const float *flr,
		      float *seed,
		      float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  float dBoffset=vi->max_curve_dB-specmax;

  /* prime the working vector with peak values */

  for(i=0;i<n;i++){
    float max=f[i];
    long oc=p->octave[i];
    while(i+1<n && p->octave[i+1]==oc){
      i++;
      if(f[i]>max)max=f[i];
    }
    
    if(max+6.f>flr[i]){
      oc=oc>>p->shiftoc;

      if(oc>=P_BANDS)oc=P_BANDS-1;
      if(oc<0)oc=0;

      seed_curve(seed,
		 curves[oc],
		 max,
		 p->octave[i]-p->firstoc,
		 p->total_octave_lines,
		 p->eighth_octave_lines,
		 dBoffset);
    }
  }
}

static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=alloca(n*sizeof(*posstack));
  float *ampstack=alloca(n*sizeof(*ampstack));
  long   stack=0;
  long   pos=0;
  long   i;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
	if(seeds[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}else{
	  if(i<posstack[stack-1]+linesper){
	    if(stack>1 && ampstack[stack-1]<=ampstack[stack-2] &&
	       i<posstack[stack-2]+linesper){
	      /* we completely overlap, making stack-1 irrelevant.  pop it */
	      stack--;
	      continue;
	    }
	  }
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;

	}
      }
    }
  }

  /* the stack now contains only the positions that are relevant. Scan
     'em straight through */

  for(i=0;i<stack;i++){
    long endpos;
    if(i<stack-1 && ampstack[i+1]>ampstack[i]){
      endpos=posstack[i+1];
    }else{
      endpos=posstack[i]+linesper+1; /* +1 is important, else bin 0 is
					discarded in short frames */
    }
    if(endpos>n)endpos=n;
    for(;pos<endpos;pos++)
      seeds[pos]=ampstack[i];
  }
  
  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */

}

/* bleaugh, this is more complicated than it needs to be */
#include<stdio.h>
static void max_seeds(vorbis_look_psy *p,
		      float *seed,
		      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(seed,linesper,n); /* for masking */
 
  pos=p->octave[0]-p->firstoc-(linesper>>1);

  while(linpos+1<p->n){
    float minV=seed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    if(minV>p->vi->tone_abs_limit)minV=p->vi->tone_abs_limit;
    while(pos+1<=end){
      pos++;
      if((seed[pos]>NEGINF && seed[pos]<minV) || minV==NEGINF)
	minV=seed[pos];
    }
    
    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
  {
    float minV=seed[p->total_octave_lines-1];
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
}

static void bark_noise_hybridmp(int n,const long *b,
                                const float *f,
                                float *noise,
                                const float offset,
                                const int fixed){
  
  float *N=alloca(n*sizeof(*N));
  float *X=alloca(n*sizeof(*N));
  float *XX=alloca(n*sizeof(*N));
  float *Y=alloca(n*sizeof(*N));
  float *XY=alloca(n*sizeof(*N));

  float tN, tX, tXX, tY, tXY;
  int i;

  int lo, hi;
  float R, A, B, D;
  float w, x, y;

  tN = tX = tXX = tY = tXY = 0.f;

  y = f[0] + offset;
  if (y < 1.f) y = 1.f;

  w = y * y * .5;
    
  tN += w;
  tX += w;
  tY += w * y;

  N[0] = tN;
  X[0] = tX;
  XX[0] = tXX;
  Y[0] = tY;
  XY[0] = tXY;

  for (i = 1, x = 1.f; i < n; i++, x += 1.f) {
    
    y = f[i] + offset;
    if (y < 1.f) y = 1.f;

    w = y * y;
    
    tN += w;
    tX += w * x;
    tXX += w * x * x;
    tY += w * y;
    tXY += w * x * y;

    N[i] = tN;
    X[i] = tX;
    XX[i] = tXX;
    Y[i] = tY;
    XY[i] = tXY;
  }
  
  for (i = 0, x = 0.f;; i++, x += 1.f) {
    
    lo = b[i] >> 16;
    if( lo>=0 ) break;
    hi = b[i] & 0xffff;
    
    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];    
    tXY = XY[hi] - XY[-lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;
    if (R < 0.f)
      R = 0.f;
    
    noise[i] = R - offset;
  }
  
  for ( ;; i++, x += 1.f) {
    
    lo = b[i] >> 16;
    hi = b[i] & 0xffff;
    if(hi>=n)break;
    
    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;
    if (R < 0.f) R = 0.f;
    
    noise[i] = R - offset;
  }
  for ( ; i < n; i++, x += 1.f) {
    
    R = (A + x * B) / D;
    if (R < 0.f) R = 0.f;
    
    noise[i] = R - offset;
  }
  
  if (fixed <= 0) return;
  
  for (i = 0, x = 0.f;; i++, x += 1.f) {
    hi = i + fixed / 2;
    lo = hi - fixed;
    if(lo>=0)break;

    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];
    tXY = XY[hi] - XY[-lo];
    
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;

    if (R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ;; i++, x += 1.f) {
    
    hi = i + fixed / 2;
    lo = hi - fixed;
    if(hi>=n)break;
    
    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + x * B) / D;
    
    if (R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ; i < n; i++, x += 1.f) {
    R = (A + x * B) / D;
    if (R - offset < noise[i]) noise[i] = R - offset;
  }
}

static float FLOOR1_fromdB_INV_LOOKUP[256]={
  0.F, 8.81683e+06F, 8.27882e+06F, 7.77365e+06F, // 1-4
  7.29930e+06F, 6.85389e+06F, 6.43567e+06F, 6.04296e+06F, // 5-8
  5.67422e+06F, 5.32798e+06F, 5.00286e+06F, 4.69759e+06F, // 9-12
  4.41094e+06F, 4.14178e+06F, 3.88905e+06F, 3.65174e+06F, // 13-16
  3.42891e+06F, 3.21968e+06F, 3.02321e+06F, 2.83873e+06F, // 17-20
  2.66551e+06F, 2.50286e+06F, 2.35014e+06F, 2.20673e+06F, // 21-24
  2.07208e+06F, 1.94564e+06F, 1.82692e+06F, 1.71544e+06F, // 25-28
  1.61076e+06F, 1.51247e+06F, 1.42018e+06F, 1.33352e+06F, // 29-32
  1.25215e+06F, 1.17574e+06F, 1.10400e+06F, 1.03663e+06F, // 33-36
  973377.F, 913981.F, 858210.F, 805842.F, // 37-40
  756669.F, 710497.F, 667142.F, 626433.F, // 41-44
  588208.F, 552316.F, 518613.F, 486967.F, // 45-48
  457252.F, 429351.F, 403152.F, 378551.F, // 49-52
  355452.F, 333762.F, 313396.F, 294273.F, // 53-56
  276316.F, 259455.F, 243623.F, 228757.F, // 57-60
  214798.F, 201691.F, 189384.F, 177828.F, // 61-64
  166977.F, 156788.F, 147221.F, 138237.F, // 65-68
  129802.F, 121881.F, 114444.F, 107461.F, // 69-72
  100903.F, 94746.3F, 88964.9F, 83536.2F, // 73-76
  78438.8F, 73652.5F, 69158.2F, 64938.1F, // 77-80
  60975.6F, 57254.9F, 53761.2F, 50480.6F, // 81-84
  47400.3F, 44507.9F, 41792.0F, 39241.9F, // 85-88
  36847.3F, 34598.9F, 32487.7F, 30505.3F, // 89-92
  28643.8F, 26896.0F, 25254.8F, 23713.7F, // 93-96
  22266.7F, 20908.0F, 19632.2F, 18434.2F, // 97-100
  17309.4F, 16253.1F, 15261.4F, 14330.1F, // 101-104
  13455.7F, 12634.6F, 11863.7F, 11139.7F, // 105-108
  10460.0F, 9821.72F, 9222.39F, 8659.64F, // 109-112
  8131.23F, 7635.06F, 7169.17F, 6731.70F, // 113-116
  6320.93F, 5935.23F, 5573.06F, 5232.99F, // 117-120
  4913.67F, 4613.84F, 4332.30F, 4067.94F, // 121-124
  3819.72F, 3586.64F, 3367.78F, 3162.28F, // 125-128
  2969.31F, 2788.13F, 2617.99F, 2458.24F, // 129-132
  2308.24F, 2167.39F, 2035.14F, 1910.95F, // 133-136
  1794.35F, 1684.85F, 1582.04F, 1485.51F, // 137-140
  1394.86F, 1309.75F, 1229.83F, 1154.78F, // 141-144
  1084.32F, 1018.15F, 956.024F, 897.687F, // 145-148
  842.910F, 791.475F, 743.179F, 697.830F, // 149-152
  655.249F, 615.265F, 577.722F, 542.469F, // 153-156
  509.367F, 478.286F, 449.101F, 421.696F, // 157-160
  395.964F, 371.803F, 349.115F, 327.812F, // 161-164
  307.809F, 289.026F, 271.390F, 254.830F, // 165-168
  239.280F, 224.679F, 210.969F, 198.096F, // 169-172
  186.008F, 174.658F, 164.000F, 153.993F, // 173-176
  144.596F, 135.773F, 127.488F, 119.708F, // 177-180
  112.404F, 105.545F, 99.1046F, 93.0572F, // 181-184
  87.3788F, 82.0469F, 77.0404F, 72.3394F, // 185-188
  67.9252F, 63.7804F, 59.8885F, 56.2341F, // 189-192
  52.8027F, 49.5807F, 46.5553F, 43.7144F, // 193-196
  41.0470F, 38.5423F, 36.1904F, 33.9821F, // 197-200
  31.9085F, 29.9614F, 28.1332F, 26.4165F, // 201-204
  24.8045F, 23.2910F, 21.8697F, 20.5352F, // 205-208
  19.2822F, 18.1056F, 17.0008F, 15.9634F, // 209-212
  14.9893F, 14.0746F, 13.2158F, 12.4094F, // 213-216
  11.6522F, 10.9411F, 10.2735F, 9.64662F, // 217-220
  9.05798F, 8.50526F, 7.98626F, 7.49894F, // 221-224
  7.04135F, 6.61169F, 6.20824F, 5.82941F, // 225-228
  5.47370F, 5.13970F, 4.82607F, 4.53158F, // 229-232
  4.25507F, 3.99542F, 3.75162F, 3.52269F, // 233-236
  3.30774F, 3.10590F, 2.91638F, 2.73842F, // 237-240
  2.57132F, 2.41442F, 2.26709F, 2.12875F, // 241-244
  1.99885F, 1.87688F, 1.76236F, 1.65482F, // 245-248
  1.55384F, 1.45902F, 1.36999F, 1.28640F, // 249-252
  1.20790F, 1.13419F, 1.06499F, 1.F // 253-256
};

void _vp_remove_floor(vorbis_look_psy *p,
		      float *mdct,
		      int *codedflr,
		      float *residue,
		      int sliding_lowpass){ 

  int i,n=p->n;
 
  if(sliding_lowpass>n)sliding_lowpass=n;
  
  for(i=0;i<sliding_lowpass;i++){
    residue[i]=
      mdct[i]*FLOOR1_fromdB_INV_LOOKUP[codedflr[i]];
  }

  for(;i<n;i++)
    residue[i]=0.;
}

void _vp_noisemask(vorbis_look_psy *p,
		   float noise_compand_level,
		   float *logmdct, 
		   float *logmask){

  int i,n=p->n;
  float *work=alloca(n*sizeof(*work));

  bark_noise_hybridmp(n,p->bark,logmdct,logmask,
		      140.,-1);

  for(i=0;i<n;i++)work[i]=logmdct[i]-logmask[i];

  bark_noise_hybridmp(n,p->bark,work,logmask,0.,
		      p->vi->noisewindowfixed);

  for(i=0;i<n;i++)work[i]=logmdct[i]-work[i];
  
#if 0
  {
    static int seq=0;

    float work2[n];
    for(i=0;i<n;i++){
      work2[i]=logmask[i]+work[i];
    }
    
    if(seq&1)
      _analysis_output("median2R",seq/2,work,n,1,0,0);
    else
      _analysis_output("median2L",seq/2,work,n,1,0,0);
    
    if(seq&1)
      _analysis_output("envelope2R",seq/2,work2,n,1,0,0);
    else
      _analysis_output("envelope2L",seq/2,work2,n,1,0,0);
    seq++;
  }
#endif

  /* aoTuV M5 extension */
  i=0;
  if((p->vi->noisecompand_high[NOISE_COMPAND_LEVELS-1] > 1) && (noise_compand_level > 0)){
  	int thter = p->n33p;
  	for(;i<thter;i++){
    	int dB=logmask[i]+.5;
    	if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
    	if(dB<0)dB=0;
    	logmask[i]= work[i]+p->vi->noisecompand[dB]-
    	  ((p->vi->noisecompand[dB]-p->vi->noisecompand_high[dB])*noise_compand_level);
  	}
  }
  for(;i<n;i++){
    int dB=logmask[i]+.5;
    if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
    if(dB<0)dB=0;
    logmask[i]= work[i]+p->vi->noisecompand[dB];
  }

}

void _vp_tonemask(vorbis_look_psy *p,
		  float *logfft,
		  float *logmask,
		  float global_specmax,
		  float local_specmax){

  int i,n=p->n;

  float *seed=alloca(sizeof(*seed)*p->total_octave_lines);
  float att=local_specmax+p->vi->ath_adjatt;
  for(i=0;i<p->total_octave_lines;i++)seed[i]=NEGINF;
  
  /* set the ATH (floating below localmax, not global max by a
     specified att) */
  if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;
  
  for(i=0;i<n;i++)
    logmask[i]=p->ath[i]+att;

  /* tone masking */
  seed_loop(p,(const float ***)p->tonecurves,logfft,logmask,seed,global_specmax);
  max_seeds(p,seed,logmask);

}

void _vp_offset_and_mix(vorbis_look_psy *p,
			float *noise,
			float *tone,
			int offset_select,
			float *logmask,
			float *mdct,
			float *logmdct,
			float *lastmdct, float *tempmdct,
			float low_compand,
			int end_block,
			int blocktype, int modenumber,
			int nW_modenumber,
			int lW_blocktype, int lW_modenumber, int lW_no){

  int i,j,n=p->n;
  int m2_sw=0,  padth; /* aoTuV for M2 */
  int it_sw, *m3n, m3_count; /* aoTuV for M3 */
  int m4_end, lp_pos, m4_start; /* aoTuV for M4 */
  float de, coeffi, cx; /* aoTuV for M1 */
  float toneth; /* aoTuV for M2 */
  float noise_rate, noise_rate_low, noise_center, rate_mod; /* aoTuV for M3 */
  float m4_thres; /* aoTuV for M4 */
  float toneatt=p->vi->tone_masteratt[offset_select];

  cx = p->m_val;
  m3n = p->m3n;
  m4_start=p->vi->normal_start;
  m4_end = p->tonecomp_endp;
  m4_thres = p->tonecomp_thres;
  lp_pos=9999;
  
  end_block+=p->vi->normal_partition;
  if(end_block>n)end_block=n;
  
  /* Collapse of low(mid) frequency is prevented. (for 32/44/48kHz q-2) */
  if(low_compand<0 || toneatt<25.)low_compand=0;
  else low_compand*=(toneatt-25.);
  
  /** @ M2 PRE **/
  if(p->vi->normal_thresh<.48){
  	if((cx > 0.5) && !modenumber && blocktype && (n==128)){
    	if(p->vi->normal_thresh>.35) padth = 10+(int)(p->vi->flacint*100);
    	else padth = 10;
    	m2_sw=1;
    }
  }

  /** @ M3 PRE **/
  m3_count = 3;
  if(toneatt < 3) m3_count = 2; // q6~
  if((n == 128) && !modenumber && !blocktype){
  	if(!lW_blocktype && !lW_modenumber){ /* last window "short" - type "impulse" */
  		if(lW_no < 8){
  			/* impulse - @impulse case1 */
  			noise_rate = 0.7-(float)(lW_no-1)/17;
  			noise_center = (float)(lW_no*m3_count);
  		}else{
  			/* impulse - @impulse case2 */
  			noise_rate = 0.3;
  			noise_center = 25;
  			if((lW_no*m3_count) < 24) noise_center = lW_no*m3_count;
  		}
  		if(offset_select == 1){
  			for(i=0; i<128; i++) tempmdct[i] -= 5;
  		}
  	}else{ /* non_impulse - @Short(impulse) case */
  		noise_rate = 0.7;
  		noise_center = 0;
  		if(offset_select == 1){
  			for(i=0; i<128; i++) tempmdct[i] = lastmdct[i] - 5;
  		}
  	}
  	noise_rate_low = 0;
  	it_sw = 1;
  }else{
  	it_sw = 0;
  }
  
  /** @ M3&M4 PRE **/
  if(cx < 0.5){
  	it_sw = 0; /* for M3 */
  	m4_end=end_block; /* for M4 */
  }else if(p->vi->normal_thresh>1.){
  	m4_start = 9999;
  }else{
  	if(m4_end>end_block)lp_pos=m4_end;
  	else lp_pos=end_block;
  }

  for(i=0;i<n;i++){
    float val= noise[i]+p->noiseoffset[offset_select][i];
    float tval= tone[i]+toneatt;
    if(i<=m4_start)tval-=low_compand;
    if(val>p->vi->noisemaxsupp)val=p->vi->noisemaxsupp;
    
    /* AoTuV */
    /** @ M2 MAIN **
    floor is pulled below suitably. (padding block only) (#2)
    by Aoyumi @ 2006/06/14
    */
    if(m2_sw){
    	// the conspicuous low level pre-echo of the padding block origin is reduced. 
    	if((logmdct[i]-lastmdct[i]) > 20){
    		if(i > m3n[3]) val -= (logmdct[i]-lastmdct[i]-20)/padth;
    		else val -= (logmdct[i]-lastmdct[i]-20)/(padth+padth);
    	}
    }
    
    /* AoTuV */
    /** @ M3 MAIN **
    Dynamic impulse block noise control. (#4)
    48/44.1/32kHz only.
    by Aoyumi @ 2006/02/02
    */
    if(it_sw){
    	for(j=1; j<=temp_bfn[i]; j++){
    		float tempbuf = logmdct[i]-(75/temp_bfn[i]*j)-5;
			if( (tempmdct[i+j] < tempbuf) && (tempmdct[i+j] < (logmdct[i+j]-5)) )
			 tempmdct[i+j] = logmdct[i+j] - 5;
		}
    	if(val > tval){
    		if( (val>lastmdct[i]) && (logmdct[i]>(tempmdct[i]+noise_center)) ){
    			float valmask=0;
    			tempmdct[i] = logmdct[i];
    			
    			if(logmdct[i]>lastmdct[i]){
    				rate_mod = noise_rate;
    			}else{
    				rate_mod = noise_rate_low;
				}
				if(i > m3n[1]){
						if((val-tval)>30) valmask=((val-tval-30)/10+30)*rate_mod;
						else valmask=(val-tval)*rate_mod;
				}else if(i > m3n[2]){
						if((val-tval)>20) valmask=((val-tval-20)/10+20)*rate_mod;
						else valmask=(val-tval)*rate_mod;
				}else if(i > m3n[3]){
						if((val-tval)>10) valmask=((val-tval-10)/10+10)*rate_mod*0.5;
						else valmask=(val-tval)*rate_mod*0.5;
				}else{
					if((val-tval)>10) valmask=((val-tval-10)/10+10)*rate_mod*0.3;
					else valmask=(val-tval)*rate_mod*0.3;
				}
				if((val-valmask)>lastmdct[i])val-=valmask;
				else val=lastmdct[i];
			}
   		}
   	}
   	
   	/* This affects calculation of a floor curve. */
   	if(i>=lp_pos)logmdct[i]=-160;
   	
     /* AoTuV */
	/** @ M4 MAIN **
	The purpose of this portion is working Noise Normalization more correctly. 
	(There is this in order to prevent extreme boost of floor)
	  m4_start = start point
	  m4_end   = end point
	  m4_thres = threshold
	by Aoyumi @ 2006/03/20
	*/
    //logmask[i]=max(val,tval);
    if(val>tval){
		logmask[i]=val;
	}else if((i>m4_start) && (i<m4_end) && (logmdct[i]>-140)){
		if(logmdct[i]>val){
			if(logmdct[i]<tval)tval-=(tval-val)*m4_thres;
		}else{
			if(val<tval)tval-=(tval-val)*m4_thres;
		}
		logmask[i]=tval;
	}else logmask[i]=tval;

    /* AoTuV */
    /** @ M1 **
	The following codes improve a noise problem.  
	A fundamental idea uses the value of masking and carries out
	the relative compensation of the MDCT. 
	However, this code is not perfect and all noise problems cannot be solved. 
	by Aoyumi @ 2004/04/18
    */

    if(offset_select == 1) {
      coeffi = -17.2;       /* coeffi is a -17.2dB threshold */
      val = val - logmdct[i];  /* val == mdct line value relative to floor in dB */
      
      if(val > coeffi){
	/* mdct value is > -17.2 dB below floor */
	
	de = 1.0-((val-coeffi)*0.005*cx);
	/* pro-rated attenuation:
	   -0.00 dB boost if mdct value is -17.2dB (relative to floor) 
	   -0.77 dB boost if mdct value is 0dB (relative to floor) 
	   -1.64 dB boost if mdct value is +17.2dB (relative to floor) 
	   etc... */
	
	if(de < 0) de = 0.0001;
      }else
	/* mdct value is <= -17.2 dB below floor */
	
	de = 1.0-((val-coeffi)*0.0003*cx);
      /* pro-rated attenuation:
	 +0.00 dB atten if mdct value is -17.2dB (relative to floor) 
	 +0.45 dB atten if mdct value is -34.4dB (relative to floor) 
	 etc... */
      
      mdct[i] *= de;
      
    }
  }

  /** @ M3 SET lastmdct **/
  if(offset_select == 1){
	if(n == 1024){
		if(!nW_modenumber){
			for(i=0; i<128; i++){
				lastmdct[i] = logmdct[i*8];
				for(j=1; j<8; j++){
					if(lastmdct[i] > logmdct[i*8+j]){
						lastmdct[i] = logmdct[i*8+j];
					}
				}
			}
		}
	}else if(n == 128){
		for(i=0; i<128; i++) lastmdct[i] = logmdct[i];
	}
  }
}

float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd){
  vorbis_info *vi=vd->vi;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;

  int n=ci->blocksizes[vd->W]/2;
  float secs=(float)n/vi->rate;

  amp+=secs*gi->ampmax_att_per_sec;
  if(amp<-9999)amp=-9999;
  return(amp);
}

static void couple_lossless(float A, float B, 
			    float *qA, float *qB){
  int test1=fabs(*qA)>fabs(*qB);
  test1-= fabs(*qA)<fabs(*qB);
  
  if(!test1)test1=((fabs(A)>fabs(B))<<1)-1;
  if(test1==1){
    *qB=(*qA>0.f?*qA-*qB:*qB-*qA);
  }else{
    float temp=*qB;  
    *qB=(*qB>0.f?*qA-*qB:*qB-*qA);
    *qA=temp;
  }

  if(*qB>fabs(*qA)*1.9999f){
    *qB= -fabs(*qA)*2.f;
    *qA= -*qA;
  }
}

static float hypot_lookup[32]={
  -0.009935, -0.011245, -0.012726, -0.014397, 
  -0.016282, -0.018407, -0.020800, -0.023494, 
  -0.026522, -0.029923, -0.033737, -0.038010, 
  -0.042787, -0.048121, -0.054064, -0.060671, 
  -0.068000, -0.076109, -0.085054, -0.094892, 
  -0.105675, -0.117451, -0.130260, -0.144134, 
  -0.159093, -0.175146, -0.192286, -0.210490, 
  -0.229718, -0.249913, -0.271001, -0.292893};

static void precomputed_couple_point(float premag,
				     int floorA,int floorB,
				     float *mag, float *ang){
  
  int test=(floorA>floorB)-1;
  int offset=31-abs(floorA-floorB);
  float floormag=hypot_lookup[((offset<0)-1)&offset]+1.f; // floormag = 0.990065 ~ 0.707107

  floormag*=FLOOR1_fromdB_INV_LOOKUP[(floorB&test)|(floorA&(~test))];

  *mag=premag*floormag;
  *ang=0.f;
}

/* just like below, this is currently set up to only do
   single-step-depth coupling.  Otherwise, we'd have to do more
   copying (which will be inevitable later) */

/* doing the real circular magnitude calculation is audibly superior
   to (A+B)/sqrt(2) */
static float dipole_hypot(float a, float b){
  if(a>0.){
    if(b>0.)return sqrt(a*a+b*b);
    if(a>-b)return sqrt(a*a-b*b);
    return -sqrt(b*b-a*a);
  }
  if(b<0.)return -sqrt(a*a+b*b);
  if(-a>b)return -sqrt(a*a-b*b);
  return sqrt(b*b-a*a);
}
static float round_hypot(float a, float b){
  if(a>0.){
    if(b>0.)return sqrt(a*a+b*b);
    if(a>-b)return sqrt(a*a+b*b);
    return -sqrt(b*b+a*a);
  }
  if(b<0.)return -sqrt(a*a+b*b);
  if(-a>b)return -sqrt(a*a+b*b);
  return sqrt(b*b+a*a);
}
/* modified hypot by aoyumi 
    better method should be found. */
static float min_indemnity_dipole_hypot(float a, float b){
  float thnor=0.92;
  float threv=0.84;
  float a2 = a*a;
  float b2 = b*b;
  if(a>0.){
    if(b>0.)return sqrt(a2+b2*thnor);
    if(a>-b)return sqrt(a2-b2+b2*threv); 
    return -sqrt(b2-a2+a2*threv);
  }
  if(b<0.)return -sqrt(a2+b2*thnor);
  if(-a>b)return -sqrt(a2-b2+b2*threv);
  return sqrt(b2-a2+a2*threv);
}

/* revert to round hypot for now */
float **_vp_quantize_couple_memo(vorbis_block *vb,
				 vorbis_info_psy_global *g,
				 vorbis_look_psy *p,
				 vorbis_info_mapping0 *vi,
				 float **mdct){
  
  int i,j,n=p->n;
  float **ret=_vorbis_block_alloc(vb,vi->coupling_steps*sizeof(*ret));
  int limit=g->coupling_pointlimit[p->vi->blockflag][PACKETBLOBS/2];
  
  if(1){ // set new hypot
  	for(i=0;i<vi->coupling_steps;i++){
    	float *mdctM=mdct[vi->coupling_mag[i]];
    	float *mdctA=mdct[vi->coupling_ang[i]];
    	
    	ret[i]=_vorbis_block_alloc(vb,n*sizeof(**ret));
    	for(j=0;j<n;j++)
    	 ret[i][j]=min_indemnity_dipole_hypot(mdctM[j],mdctA[j]);
  	}
  }else{
    for(i=0;i<vi->coupling_steps;i++){
    	float *mdctM=mdct[vi->coupling_mag[i]];
    	float *mdctA=mdct[vi->coupling_ang[i]];
    	
    	ret[i]=_vorbis_block_alloc(vb,n*sizeof(**ret));
    	for(j=0;j<limit;j++)
    	 ret[i][j]=dipole_hypot(mdctM[j],mdctA[j]);
    	for(;j<n;j++)
      	 ret[i][j]=round_hypot(mdctM[j],mdctA[j]);
  	}
  }
  return(ret);
}

/* this is for per-channel noise normalization */
static int apsort(const void *a, const void *b){
  float f1=fabs(**(float**)a);
  float f2=fabs(**(float**)b);
  return (f1<f2)-(f1>f2);
}

/*** optimization of sort (for 8 or 32 element) ***/
#ifdef OPT_SORT
#define C(o,a,b)\
  (fabs(data[o+a])>=fabs(data[o+b]))
#define O(o,a,b,c,d)\
  {n[o]=o+a;n[o+1]=o+b;n[o+2]=o+c;n[o+3]=o+d;}
#define SORT4(o)\
  if(C(o,2,3))if(C(o,0,1))if(C(o,0,2))if(C(o,1,2))O(o,0,1,2,3)\
        else if(C(o,1,3))O(o,0,2,1,3)\
          else O(o,0,2,3,1)\
      else if(C(o,0,3))if(C(o,1,3))O(o,2,0,1,3)\
          else O(o,2,0,3,1)\
        else O(o,2,3,0,1)\
    else if(C(o,1,2))if(C(o,0,2))O(o,1,0,2,3)\
        else if(C(o,0,3))O(o,1,2,0,3)\
          else O(o,1,2,3,0)\
      else if(C(o,1,3))if(C(o,0,3))O(o,2,1,0,3)\
          else O(o,2,1,3,0)\
        else O(o,2,3,1,0)\
  else if(C(o,0,1))if(C(o,0,3))if(C(o,1,3))O(o,0,1,3,2)\
        else if(C(o,1,2))O(o,0,3,1,2)\
          else O(o,0,3,2,1)\
      else if(C(o,0,2))if(C(o,1,2))O(o,3,0,1,2)\
          else O(o,3,0,2,1)\
        else O(o,3,2,0,1)\
    else if(C(o,1,3))if(C(o,0,3))O(o,1,0,3,2)\
        else if(C(o,0,2))O(o,1,3,0,2)\
          else O(o,1,3,2,0)\
      else if(C(o,1,2))if(C(o,0,2))O(o,3,1,0,2)\
          else O(o,3,1,2,0)\
        else O(o,3,2,1,0)

static void sortindex_fix8(int *index,
                           float *data,
                           int offset){
  int i,j,k,n[8];
  index+=offset;
  data+=offset;
  SORT4(0)
  SORT4(4)
  j=0;k=4;
  for(i=0;i<8;i++)
    index[i]=n[(k>=8)||(j<4)&&C(0,n[j],n[k])?j++:k++]+offset;
}

static void sortindex_fix32(int *index,
                            float *data,
                            int offset){
  int i,j,k,n[32];
  for(i=0;i<32;i+=8)
    sortindex_fix8(index,data,offset+i);
  index+=offset;
  for(i=j=0,k=8;i<16;i++)
    n[i]=index[(k>=16)||(j<8)&&C(0,index[j],index[k])?j++:k++];
  for(i=j=16,k=24;i<32;i++)
    n[i]=index[(k>=32)||(j<24)&&C(0,index[j],index[k])?j++:k++];
  for(i=j=0,k=16;i<32;i++)
    index[i]=n[(k>=32)||(j<16)&&C(0,n[j],n[k])?j++:k++];
}

static void sortindex_shellsort(int *index,
                                float *data,
                                int offset,
                                int count){
  int gap,pos,left,right,i,j;
  index+=offset;
  for(i=0;i<count;i++)index[i]=i+offset;
  gap=1;
  while (gap<=count)gap=gap*3+1;
  gap/=3;
  if(gap>=4)gap/=3;
  while(gap>0){
    for(pos=gap;pos<count;pos++){
      for(left=pos-gap;left>=0;left-=gap){
        i=index[left];j=index[left+gap];
        if(!C(0,i,j)){
          index[left]=j;
          index[left+gap]=i;
        }else break;
      }
    }
    gap/=3;
  }
}

static void sortindex(int *index,
                      float *data,
                      int offset,
                      int count){
  if(count==8)sortindex_fix8(index,data,offset);
  else if(count==32)sortindex_fix32(index,data,offset);
  else sortindex_shellsort(index,data,offset,count);
}

#undef C
#undef O
#undef SORT4

#endif
/*** OPT_SORT End ***/


int **_vp_quantize_couple_sort(vorbis_block *vb,
			       vorbis_look_psy *p,
			       vorbis_info_mapping0 *vi,
			       float **mags){

#ifdef OPT_SORT
  if(p->vi->normal_point_p){
    int i,j,n=p->n;
    int **ret=_vorbis_block_alloc(vb,vi->coupling_steps*sizeof(*ret));
    int partition=p->vi->normal_partition;
    
    for(i=0;i<vi->coupling_steps;i++){
      ret[i]=_vorbis_block_alloc(vb,n*sizeof(**ret));
      
      for(j=0;j<n;j+=partition){
      sortindex(ret[i],mags[i],j,partition);
      }
    }
    return(ret);
  }
  return(NULL);
#else
  if(p->vi->normal_point_p){
    int i,j,k,n=p->n;
    int **ret=_vorbis_block_alloc(vb,vi->coupling_steps*sizeof(*ret));
    int partition=p->vi->normal_partition;
    float **work=alloca(sizeof(*work)*partition);
    
    for(i=0;i<vi->coupling_steps;i++){
      ret[i]=_vorbis_block_alloc(vb,n*sizeof(**ret));
      
      for(j=0;j<n;j+=partition){
	for(k=0;k<partition;k++)work[k]=mags[i]+k+j;
	qsort(work,partition,sizeof(*work),apsort);
	for(k=0;k<partition;k++)ret[i][k+j]=work[k]-mags[i];
      }
    }
    return(ret);
  }
  return(NULL);
#endif
}

void _vp_noise_normalize_sort(vorbis_look_psy *p,
			      float *magnitudes,int *sortedindex){
#ifdef OPT_SORT
  int j,n=p->n;
  vorbis_info_psy *vi=p->vi;
  int partition=vi->normal_partition;
  int start=vi->normal_start;

  for(j=start;j<n;j+=partition){
    if(j+partition>n)partition=n-j;
    sortindex(sortedindex-start,magnitudes,j,partition);
  }
#else
  int i,j,n=p->n;
  vorbis_info_psy *vi=p->vi;
  int partition=vi->normal_partition;
  float **work=alloca(sizeof(*work)*partition);
  int start=vi->normal_start;

  for(j=start;j<n;j+=partition){
    if(j+partition>n)partition=n-j;
    for(i=0;i<partition;i++)work[i]=magnitudes+i+j;
    qsort(work,partition,sizeof(*work),apsort);
    for(i=0;i<partition;i++){
      sortedindex[i+j-start]=work[i]-magnitudes;
    }
  }
#endif
}

void _vp_noise_normalize(vorbis_look_psy *p,
			 float *in,float *out,int *sortedindex){
  int i,j=0,n=p->n,min_energy;
  vorbis_info_psy *vi=p->vi;
  int partition=vi->normal_partition;
  int start=vi->normal_start;

  if(start>n)start=n;

  if(vi->normal_channel_p){
    for(;j<start;j++)
      out[j]=rint(in[j]);
    
    for(;j+partition<=n;j+=partition){
      float acc=0.;
      int k;
      int energy_loss=0;
      int nn_num=0;
      int freqband_mid=j+16;
      int freqband_flag=0;
      
      for(i=j;i<j+partition;i++){
        if(rint(in[i])==0.f){
        	acc+=in[i]*in[i];
        	energy_loss++;
        }
      }
      /* When an energy loss is large, NN processing is carried out in the middle of partition. */
      /*if(energy_loss==32 && fabs(in[freqband_mid])>nnmid_th){
      	if(in[freqband_mid]*in[freqband_mid]<.25f){
      		i=0;
      		if(acc>=vi->normal_thresh){
      			out[freqband_mid]=unitnorm(in[freqband_mid]);
      			acc-=1.;
      			freqband_flag=1;
      			nn_num++;
      		}
      	}
      }*/
      
      /* NN main */
      for(i=0;i<partition;i++){
      	k=sortedindex[i+j-start];
      	if(in[k]*in[k]>=.25f){ // or rint(in[k])!=0.f
      		out[k]=rint(in[k]);
      		//acc-=in[k]*in[k];
      	}else{
      		if(acc<vi->normal_thresh)break;
      		if(freqband_flag && freqband_mid==k)continue;
      		out[k]=unitnorm(in[k]);
      		acc-=1.;
      		nn_num++;
      	}
      }
      
      /* The minimum energy complement */
      /*min_energy=32-energy_loss+nn_num;
      if(min_energy<2 || (j<=p->min_nn_lp && min_energy==2)){
      	k=sortedindex[i+j-start];
      	if(freqband_flag && freqband_mid==k){
      		i++;
      		k=sortedindex[i+j-start];
	    }
	    if(!(fabs(in[k])<0.3)){
	    	out[k]=unitnorm(in[k]);
	    	i++;
	    }
	  }*/
	  
	  // The last process
      for(;i<partition;i++){
      	k=sortedindex[i+j-start];
      	if(freqband_flag && freqband_mid==k)continue;
      	else out[k]=0.;
      }
    }
  }
  
  for(;j<n;j++)
    out[j]=rint(in[j]);
  
}

void _vp_couple(int blobno,
		vorbis_info_psy_global *g,
		vorbis_look_psy *p,
		vorbis_info_mapping0 *vi,
		float **res,
		float **mag_memo,
		int   **mag_sort,
		int   **ifloor,
		int   *nonzero,
		int  sliding_lowpass,
		float **mdct, float **res_org){

  int i,j,k,n=p->n;

  /* perform any requested channel coupling */
  /* point stereo can only be used in a first stage (in this encoder)
     because of the dependency on floor lookups */
  for(i=0;i<vi->coupling_steps;i++){

    /* once we're doing multistage coupling in which a channel goes
       through more than one coupling step, the floor vector
       magnitudes will also have to be recalculated an propogated
       along with PCM.  Right now, we're not (that will wait until 5.1
       most likely), so the code isn't here yet. The memory management
       here is all assuming single depth couplings anyway. */

    /* make sure coupling a zero and a nonzero channel results in two
       nonzero channels. */
    if(nonzero[vi->coupling_mag[i]] ||
       nonzero[vi->coupling_ang[i]]){
     

      float *rM=res[vi->coupling_mag[i]];
      float *rA=res[vi->coupling_ang[i]];
      float *rMo=res_org[vi->coupling_mag[i]];
      float *rAo=res_org[vi->coupling_ang[i]];
      float *qM=rM+n;
      float *qA=rA+n;
      float *mdctM=mdct[vi->coupling_mag[i]];
      float *mdctA=mdct[vi->coupling_ang[i]];
      int *floorM=ifloor[vi->coupling_mag[i]];
      int *floorA=ifloor[vi->coupling_ang[i]];
      float prepoint=stereo_threshholds[g->coupling_prepointamp[blobno]];
      float postpoint=stereo_threshholds[g->coupling_postpointamp[blobno]];
      float sth_low=stereo_threshholds_low[g->coupling_prepointamp[blobno]];
      float sth_high=stereo_threshholds_high[g->coupling_postpointamp[blobno]];
      float postpoint_backup;
      float st_thresh;
      int partition=(p->vi->normal_point_p?p->vi->normal_partition:p->n);
      int limit=g->coupling_pointlimit[p->vi->blockflag][blobno];
      int pointlimit=limit;
      int freqlimit=p->st_freqlimit;
      unsigned char Mc_treshp[2048];
      unsigned char Ac_treshp[2048];
      int lof_st;
      int hif_st;
      int hif_stcopy;
      int old_lof_st=0;
      int old_hif_st=0;
      int Afreq_num=0;
      int Mfreq_num=0;
      int stcont_start=0; // M6 start point
      
      nonzero[vi->coupling_mag[i]]=1; 
      nonzero[vi->coupling_ang[i]]=1; 
       
      postpoint_backup=postpoint;
      
      /** @ M6 PRE **/
      // lossless only?
      if(!stereo_threshholds[g->coupling_postpointamp[blobno]])stcont_start=n;
      else{
      	// exception handling
      	if((postpoint-sth_high)<prepoint)sth_high=postpoint-prepoint;
      	// start point setup
      	for(j=0;j<n;j++){
      		stcont_start=j;
      		if(p->noiseoffset[1][j]>=-2)break;
      	}
      	// start point correction & threshold setup 
      	st_thresh=.1;
      	if(p->m_val<.5){
      		// low frequency limit
      		if(stcont_start<limit)stcont_start=limit;
      	}else if(p->vi->normal_thresh>1.)st_thresh=.5;
      	for(j=0;j<=freqlimit;j++){ // or j<n
      		if(fabs(rM[j])<st_thresh)Mc_treshp[j]=1;
      		else Mc_treshp[j]=0;
      		if(fabs(rA[j])<st_thresh)Ac_treshp[j]=1;
      		else Ac_treshp[j]=0;
      	}
      }
      
      for(j=0;j<n;j+=partition){
	float acc=0.f;
	float rpacc;
	int energy_loss=0;
	int nn_num=0;

	for(k=0;k<partition;k++){
	  int l=k+j;
	  float a=mdctM[l];
	  float b=mdctA[l];
	  float dummypoint;
	  float hypot_reserve;
	  float slow=0.f;
	  float shigh=0.f;
	  float slowM=0.f;
	  float slowA=0.f;
	  float shighM=0.f;
	  float shighA=0.f;
	  float rMs=fabs(rMo[l]);
      float rAs=fabs(rAo[l]);

	  postpoint=postpoint_backup;
	  
	  /* AoTuV */
	  /** @ M6 MAIN **
	    The threshold of a stereo is changed dynamically. 
	    by Aoyumi @ 2006/06/04
	  */
	  if(l>=stcont_start){
	  	int m;
	  	int lof_num;
	  	int hif_num;
	  	
	  	// (It may be better to calculate this in advance) 
	  	lof_st=l-(l/2)*.167;
	  	hif_st=l+l*.167;
	  
	  	hif_stcopy=hif_st;
	  	
	  	// limit setting
	  	if(hif_st>freqlimit)hif_st=freqlimit;
	  	
	  	if(old_lof_st || old_hif_st){
	  		if(hif_st>l){
	  			// hif_st, lof_st ...absolute value
	  			// lof_num, hif_num ...relative value
	  			
	  			// low freq.(lower)
	  			lof_num=lof_st-old_lof_st;
	  			if(lof_num==0){
	  				Afreq_num+=Ac_treshp[l-1];
	  				Mfreq_num+=Mc_treshp[l-1];
	  			}else if(lof_num==1){
	  				Afreq_num+=Ac_treshp[l-1];
	  				Mfreq_num+=Mc_treshp[l-1];
	  				Afreq_num-=Ac_treshp[old_lof_st];
	  				Mfreq_num-=Mc_treshp[old_lof_st];
	  			}//else puts("err. low");
	  			
	  			// high freq.(higher)
	  			hif_num=hif_st-old_hif_st;
	  			if(hif_num==0){
	  				Afreq_num-=Ac_treshp[l];
	  				Mfreq_num-=Mc_treshp[l];
	  			}else if(hif_num==1){
	  				Afreq_num-=Ac_treshp[l];
	  				Mfreq_num-=Mc_treshp[l];
	  				Afreq_num+=Ac_treshp[hif_st];
	  				Mfreq_num+=Mc_treshp[hif_st];
	  			}else if(hif_num==2){
	  				Afreq_num-=Ac_treshp[l];
	  				Mfreq_num-=Mc_treshp[l];
	  				Afreq_num+=Ac_treshp[hif_st];
	  				Mfreq_num+=Mc_treshp[hif_st];
	  				Afreq_num+=Ac_treshp[hif_st-1];
	  				Mfreq_num+=Mc_treshp[hif_st-1];
	  			}//else puts("err. high");
	  		}
	  	}else{
	  		for(m=lof_st; m<=hif_st; m++){
	  			if(m==l)continue;
	  			if(Ac_treshp[m]) Afreq_num++;
	  			if(Mc_treshp[m]) Mfreq_num++;
			}
	  	}
	  	if(l>=limit){
	  		shigh=sth_high/(hif_stcopy-lof_st);
	  		shighA=shigh*Afreq_num;
	  		shighM=shigh*Mfreq_num;
	  		if((shighA+rAs)>(shighM+rMs))shigh=shighA;
	  		else shigh=shighM;
		}else{
			slow=sth_low/(hif_stcopy-lof_st);
			slowA=slow*Afreq_num;
			slowM=slow*Mfreq_num;
			if(p->noiseoffset[1][l]<-1){
				slowA*=(p->noiseoffset[1][l]+2);
				slowM*=(p->noiseoffset[1][l]+2);
			}
		}
		old_lof_st=lof_st;
	  	old_hif_st=hif_st;
	  }

	  if(l>=limit){
	    postpoint-=shigh;
	    /* The following prevents an extreme reduction of residue. (2ch stereo only) */
	  	if( ((a>0.) && (b<0.)) || ((b>0.) && (a<0.)) ){
	  		hypot_reserve = fabs(fabs(a)-fabs(b));
	  		if(hypot_reserve < 0.001){ // 0~0.000999-
	  			dummypoint = stereo_threshholds_rephase[g->coupling_postpointamp[blobno]];
	  			dummypoint = dummypoint+((postpoint-dummypoint)*(hypot_reserve*1000));
	  			if(postpoint > dummypoint) postpoint = dummypoint;
	  		}
      	}
	  }
	  
	  if(l<sliding_lowpass){
	    if((l>=limit && rMs<postpoint && rAs<postpoint) ||
	       (rMs<(prepoint-slowM) && rAs<(prepoint-slowA))){


	      precomputed_couple_point(mag_memo[i][l],
				       floorM[l],floorA[l],
				       qM+l,qA+l);

	      //if(rint(qM[l])==0.f)acc+=qM[l]*qM[l];
	      if(rint(qM[l])==0.f){
	      	energy_loss++;
	      	if(l>=limit)acc+=qM[l]*qM[l];
	      }
	    }else{
	      couple_lossless(rM[l],rA[l],qM+l,qA+l);
	    }
	  }else{
	    qM[l]=0.;
	    qA[l]=0.;
	  }
	}

	if(p->vi->normal_point_p){
	  int freqband_mid=j+16;
	  int freqband_flag=0;
	  int min_energy;
	  
	  rpacc=acc;
	  /* When the energy loss of a partition is large, NN is performed in the middle of partition.
	      for 48/44.1/32kHz */
	  if(energy_loss==32 && fabs(qM[freqband_mid])>nnmid_th && acc>=p->vi->normal_thresh
	   && freqband_mid<sliding_lowpass && freqband_mid>=pointlimit && rint(qM[freqband_mid])==0.f){
	  	if( ((mdctM[freqband_mid]>0.) && (mdctA[freqband_mid]<0.)) ||
	  	 ((mdctA[freqband_mid]>0.) && (mdctM[freqband_mid]<0.)) ){
	  	 acc-=1.f;
	  	 rpacc-=1.32;
	  	}else{
	  	 acc-=1.f;
	  	 rpacc-=1.f;
	  	}
	  	qM[freqband_mid]=unitnorm(qM[freqband_mid]);
	  	freqband_flag=1;
	  	nn_num++;
	  }
	  /* NN main (point stereo) */
	  for(k=0;k<partition && acc>=p->vi->normal_thresh;k++){
	    int l;
	    l=mag_sort[i][j+k];
	    if(freqband_mid==l && freqband_flag)continue;
	    if(l<sliding_lowpass && l>=pointlimit && rint(qM[l])==0.f){
	      if( ((mdctM[l]>0.) && (mdctA[l]<0.)) || ((mdctA[l]>0.) && (mdctM[l]<0.)) ){
	        if(rpacc<p->vi->normal_thresh)continue;
	        acc-=1.f;
	        rpacc-=1.32;
	      }else{
	        acc-=1.f;
	        rpacc-=1.f;
	      }
	      qM[l]=unitnorm(qM[l]);
	      nn_num++;
	    }
	  }
	  /* The minimum energy complement.
	      for 48/44.1/32kHz */
	  min_energy=32-energy_loss+nn_num;
	  if(min_energy<2 || (j<=p->min_nn_lp && min_energy==2)){
	  	int l;
	  	float ab;
	    for(;k<partition;k++){
	    	l=mag_sort[i][j+k];
	    	ab=fabs(qM[l]);
	    	if(ab<0.04)break;
	    	if( ((mdctM[l]>0.) && (mdctA[l]<0.)) || ((mdctA[l]>0.) && (mdctM[l]<0.))
	    	 && ab<0.11)break; // 0.11
	    	if(rint(qM[l])==0.f && l>=pointlimit){
	    		qM[l]=unitnorm(qM[l]);
	    		break;
	    	}
	    }
	  }
	}
      }
    }
  }
}

/*  aoTuV M5
	noise_compand_level of low frequency is determined from the level of high frequency. 
	by Aoyumi @ 2005/09/14
	
	return value
	[normal compander] 0 <> 1.0 [high compander] 
	-1 @ disable
*/
float lb_loudnoise_fix(vorbis_look_psy *p,
		float noise_compand_level,
		float *logmdct,
		int lW_modenumber,
		int blocktype, int modenumber){

	int i, n=p->n, nq1=p->n25p, nq3=p->n75p;
	double hi_th=0;
	
	if(p->m_val < 0.5)return(-1); /* 48/44.1/32kHz only */
	if(p->vi->normal_thresh>.45)return(-1); /* under q3 */

	/* select trans. block(short>>long case). */
	if(!modenumber)return(-1);
	if(blocktype || lW_modenumber)return(noise_compand_level);

	/* calculation of a threshold. */
	for(i=nq1; i<nq3; i++){
		if(logmdct[i]>-130)hi_th += logmdct[i];
		else hi_th += -130;
	}
	hi_th /= n;
	
	/* calculation of a high_compand_level */
	if(hi_th > -40.) noise_compand_level=-1;
	else if(hi_th < -50.) noise_compand_level=1.;
	else noise_compand_level=1.-((hi_th+50)/10);

	return(noise_compand_level);
}
