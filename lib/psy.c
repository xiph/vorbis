/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.48 2001/06/18 22:19:26 xiphmont Exp $

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

/* Why Bark scale for encoding but not masking computation? Because
   masking has a strong harmonic dependancy */

/* the beginnings of real psychoacoustic infrastructure.  This is
   still not tightly tuned */
void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    _ogg_free(i);
  }
}

vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i){
  vorbis_info_psy *ret=_ogg_malloc(sizeof(vorbis_info_psy));
  memcpy(ret,i,sizeof(vorbis_info_psy));
  return(ret);
}

/* Set up decibel threshold slopes on a Bark frequency scale */
/* ATH is the only bit left on a Bark scale.  No reason to change it
   right now */
static void set_curve(float *ref,float *c,int n, float crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK(i+1)*2*n/crate);
    float base=ref[i];
    if(j<endpos){
      float delta=(ref[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
	c[j]=base;
	base+=delta;
      }
    }
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

static void interp_curve(float *c,float *c1,float *c2,float del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.f-del);
}

static void setup_curve(float **c,
			int band,
			float *curveatt_dB){
  int i,j;
  float ath[EHMER_MAX];
  float tempc[P_LEVELS][EHMER_MAX];
  float *ATH=ATH_Bark_dB_lspconservative; /* just for limiting here */

  memcpy(c[0]+2,c[4]+2,sizeof(float)*EHMER_MAX);
  memcpy(c[2]+2,c[4]+2,sizeof(float)*EHMER_MAX);

  /* we add back in the ATH to avoid low level curves falling off to
     -infinity and unneccessarily cutting off high level curves in the
     curve limiting (last step).  But again, remember... a half-band's
     settings must be valid over the whole band, and it's better to
     mask too little than too much, so be pessimal. */

  for(i=0;i<EHMER_MAX;i++){
    float oc_min=band*.5+(i-EHMER_OFFSET)*.125;
    float oc_max=band*.5+(i-EHMER_OFFSET+1)*.125;
    float bark=toBARK(fromOC(oc_min));
    int ibark=floor(bark);
    float del=bark-ibark;
    float ath_min,ath_max;

    if(ibark<26)
      ath_min=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_min=ATH[25];

    bark=toBARK(fromOC(oc_max));
    ibark=floor(bark);
    del=bark-ibark;

    if(ibark<26)
      ath_max=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_max=ATH[25];

    ath[i]=min(ath_min,ath_max);
  }

  /* The c array is comes in as dB curves at 20 40 60 80 100 dB.
     interpolate intermediate dB curves */
  for(i=1;i<P_LEVELS;i+=2){
    interp_curve(c[i]+2,c[i-1]+2,c[i+1]+2,.5);
  }

  /* normalize curves so the driving amplitude is 0dB */
  /* make temp curves with the ATH overlayed */
  for(i=0;i<P_LEVELS;i++){
    attenuate_curve(c[i]+2,curveatt_dB[i]);
    memcpy(tempc[i],ath,EHMER_MAX*sizeof(float));
    attenuate_curve(tempc[i],-i*10.f);
    max_curve(tempc[i],c[i]+2);
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
    min_curve(tempc[j],tempc[j-1]);
    min_curve(c[j]+2,tempc[j]);
  }

  /* add fenceposts */
  for(j=0;j<P_LEVELS;j++){

    for(i=0;i<EHMER_MAX;i++)
      if(c[j][i+2]>-200.f)break;  
    c[j][0]=i;

    for(i=EHMER_MAX-1;i>=0;i--)
      if(c[j][i+2]>-200.f)
	break;
    c[j][1]=i;

  }
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate){
  long i,j,lo=0,hi=0;
  long maxoc;
  memset(p,0,sizeof(vorbis_look_psy));


  p->eighth_octave_lines=vi->eighth_octave_lines;
  p->shiftoc=rint(log(vi->eighth_octave_lines*8)/log(2))-1;

  p->firstoc=toOC(.25f*rate/n)*(1<<(p->shiftoc+1))-vi->eighth_octave_lines;
  maxoc=toOC((n*.5f-.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;

  if(vi->ath)
    p->ath=_ogg_malloc(n*sizeof(float));
  p->octave=_ogg_malloc(n*sizeof(long));
  p->bark=_ogg_malloc(n*sizeof(unsigned long));
  p->vi=vi;
  p->n=n;

  /* set up the lookups for a given blocksize and sample rate */
  /* Vorbis max sample rate is currently limited by 26 Bark (54kHz) */
  if(vi->ath)
    set_curve(vi->ath, p->ath,n,rate);
  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i); 

    for(;lo+vi->noisewindowlomin<i && 
	  toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);
    
    for(;hi<n && (hi<i+vi->noisewindowhimin ||
	  toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);
    
    p->bark[i]=(hi<<16)+lo;

  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i*.5f+.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=_ogg_malloc(P_BANDS*sizeof(float **));
  p->noisemedian=_ogg_malloc(n*sizeof(int));
  p->noiseoffset=_ogg_malloc(n*sizeof(float));
  p->peakatt=_ogg_malloc(P_BANDS*sizeof(float *));
  for(i=0;i<P_BANDS;i++){
    p->tonecurves[i]=_ogg_malloc(P_LEVELS*sizeof(float *));
    p->peakatt[i]=_ogg_malloc(P_LEVELS*sizeof(float));
  }

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++){
      p->tonecurves[i][j]=_ogg_malloc((EHMER_MAX+2)*sizeof(float));
    }

  /* OK, yeah, this was a silly way to do it */
  memcpy(p->tonecurves[0][4]+2,tone_125_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][6]+2,tone_125_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][8]+2,tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][10]+2,tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[2][4]+2,tone_125_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][6]+2,tone_125_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][8]+2,tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][10]+2,tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[4][4]+2,tone_250_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][6]+2,tone_250_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][8]+2,tone_250_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][10]+2,tone_250_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[6][4]+2,tone_500_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][6]+2,tone_500_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][8]+2,tone_500_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][10]+2,tone_500_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[8][4]+2,tone_1000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][6]+2,tone_1000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][8]+2,tone_1000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][10]+2,tone_1000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[10][4]+2,tone_2000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][6]+2,tone_2000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][8]+2,tone_2000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][10]+2,tone_2000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[12][4]+2,tone_4000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][6]+2,tone_4000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][8]+2,tone_4000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][10]+2,tone_4000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[14][4]+2,tone_8000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][6]+2,tone_8000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][8]+2,tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][10]+2,tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[16][4]+2,tone_8000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][6]+2,tone_8000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][8]+2,tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][10]+2,tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);

  /* interpolate curves between */
  for(i=1;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2){
      memcpy(p->tonecurves[i][j]+2,p->tonecurves[i-1][j]+2,EHMER_MAX*sizeof(float));
      /*interp_curve(p->tonecurves[i][j],
		   p->tonecurves[i-1][j],
		   p->tonecurves[i+1][j],.5);*/
      min_curve(p->tonecurves[i][j]+2,p->tonecurves[i+1][j]+2);
    }

  /* set up the final curves */
  for(i=0;i<P_BANDS;i++)
    setup_curve(p->tonecurves[i],i,vi->toneatt[i]);

  /* set up attenuation levels */
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++){
      p->peakatt[i][j]=p->vi->peakatt[i][j];
    }

  /* set up rolling noise median */
  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5)*rate/(2.*n))*2.+2.;
    int inthalfoc;
    float del;
    
    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;

    p->noisemedian[i]=rint(
      (p->vi->noisemedian[inthalfoc*2]*(1.-del) + 
       p->vi->noisemedian[inthalfoc*2+2]*del)*1024.f);
    p->noiseoffset[i]=
      p->vi->noisemedian[inthalfoc*2+1]*(1.-del) + 
      p->vi->noisemedian[inthalfoc*2+3]*del -
      140.f;
  }
  /*_analysis_output("mediancurve",0,p->noisemedian,n,0,0);*/
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
	_ogg_free(p->peakatt[i]);
      }
      _ogg_free(p->tonecurves);
      _ogg_free(p->noisemedian);
      _ogg_free(p->noiseoffset);
      _ogg_free(p->peakatt);
    }
    memset(p,0,sizeof(vorbis_look_psy));
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

  int choice=(int)((amp+dBoffset)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-16)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

static void seed_peak(float *seed,
		      const float *att,
		      float amp,
		      int oc,
		      int linesper,
		      float dBoffset){
  long seedptr;

  int choice=(int)((amp+dBoffset)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  seedptr=oc-(linesper>>1);

  amp+=att[choice];
  if(seed[seedptr]<amp)seed[seedptr]=amp;

}

static void seed_loop(vorbis_look_psy *p,
		      const float ***curves,
		      const float **att,
		      const float *f, 
		      const float *flr,
		      float *minseed,
		      float *maxseed,
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

      if(max>flr[i]){
	oc=oc>>p->shiftoc;
	if(oc>=P_BANDS)oc=P_BANDS-1;
	if(oc<0)oc=0;
	if(vi->tonemaskp)
	  seed_curve(minseed,
		     curves[oc],
		     max,
		     p->octave[i]-p->firstoc,
		     p->total_octave_lines,
		     p->eighth_octave_lines,
		     dBoffset);
	if(vi->peakattp)
	  seed_peak(maxseed,
		    att[oc],
		    max,
		    p->octave[i]-p->firstoc,
		    p->eighth_octave_lines,
		    dBoffset);
      }
  }
}

static void bound_loop(vorbis_look_psy *p,
		       float *f, 
		       float *seeds,
		       float *flr,
		       float att){
  long n=p->n,i;

  long off=(p->eighth_octave_lines>>1)+p->firstoc;
  long *ocp=p->octave;

  for(i=0;i<n;i++){
    long oc=ocp[i]-off;
    float v=f[i]+att;
    if(seeds[oc]<v)seeds[oc]=v;
  }
}

static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=alloca(n*sizeof(long));
  float *ampstack=alloca(n*sizeof(float));
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
static void max_seeds(vorbis_look_psy *p,float *minseed,float *maxseed,
		      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(minseed,linesper,n); /* for masking */
  seed_chase(maxseed,linesper,n); /* for peak att */
 
  pos=p->octave[0]-p->firstoc-(linesper>>1);
  while(linpos+1<p->n){
    float min=minseed[pos];
    float max=maxseed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    while(pos+1<=end){
      pos++;
      if((minseed[pos]>NEGINF && minseed[pos]<min) || min==NEGINF)
	min=minseed[pos];
      if(maxseed[pos]>max)max=maxseed[pos];
    }
    if(max<min)max=min;
    
    /* seed scale is log.  Floor is linear.  Map back to it */
    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<max)flr[linpos]=max;
  }
  
  {
    float min=minseed[p->total_octave_lines-1];
    float max=maxseed[p->total_octave_lines-1];
    if(max<min)max=min;
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<max)flr[linpos]=max;
  }
  
}

/* set to match vorbis_quantdblook.h */
#define BINCOUNT 280
#define LASTBIN  (BINCOUNT-1)

static int psy_dBquant(const float *x){
  int i= *x*2.f+279.5f;
  if(i>279)return(279);
  if(i<0)return(0);
  return i;
}


static void bark_noise_median(int n,const long *b,const float *f,
			      float *noise,
			      float lowidth,float hiwidth,
			      int lomin,int himin,
			      const int *thresh,const float *off,
			      int fixed){
  int i=0,lo=-1,hi=-1,fixedc=0;
  int  median=LASTBIN>>1;

  int barkradix[BINCOUNT];
  int barkcountbelow=0;

  int fixedradix[BINCOUNT];
  int fixedcountbelow=0;

  memset(barkradix,0,sizeof(barkradix));

  if(fixed>0){
    memset(fixedradix,0,sizeof(fixedradix));

    /* bootstrap the fixed window case seperately */
    for(i=0;i<(fixed>>1);i++){
      int bin=psy_dBquant(f+i);
      fixedradix[bin]++;
      fixedc++;
      if(bin<=median)
	fixedcountbelow++;
    }
  }

  for(i=0;i<n;i++){
    /* find new lo/hi */
    int bi=b[i]>>16;
    for(;hi<bi;hi++){
      int bin=psy_dBquant(f+hi);
      barkradix[bin]++;
      if(bin<=median)
	barkcountbelow++;
    }
    bi=b[i]&0xffff;
    for(;lo<bi;lo++){
      int bin=psy_dBquant(f+lo);
      barkradix[bin]--;
      if(bin<=median)
	barkcountbelow--;
    }

    if(fixed>0){
      bi=i+(fixed>>1);
      if(bi<n){
	int bin=psy_dBquant(f+bi);
	fixedradix[bin]++;
	fixedc++;
	if(bin<=median)
	  fixedcountbelow++;
      }
      
      bi-=fixed;
      if(bi>=0){
	int bin=psy_dBquant(f+bi);
	fixedradix[bin]--;
	fixedc--;
	if(bin<=median)
	  fixedcountbelow--;
      }
    }

    /* move the median if needed */
    {
      int bark_th = (thresh[i]*(hi-lo)+512)/1024;
      
      if(fixed>0){
	int fixed_th = (thresh[i]*(fixedc)+512)/1024;
	
	while(bark_th>=barkcountbelow && 
	      fixed_th>=fixedcountbelow /* && median<LASTBIN by rep invariant */
	      ){
	  median++;
	  barkcountbelow+=barkradix[median];
	  fixedcountbelow+=fixedradix[median];
	}
	
	while(bark_th<barkcountbelow ||
	      fixed_th<fixedcountbelow /* && median>=0 by rep invariant */
	      ){
	  barkcountbelow-=barkradix[median];
	  fixedcountbelow-=fixedradix[median];
	  median--;
	}
      }else{
	while(bark_th>=barkcountbelow){
	  median++;
	  barkcountbelow+=barkradix[median];
	}
	
	while(bark_th<barkcountbelow){
	  barkcountbelow-=barkradix[median];
	  median--;
	}
      }
    }

    noise[i]= (median+1)*.5f+off[i];
  }

}

float _vp_compute_mask(vorbis_look_psy *p,
		      float *fft, 
		      float *mdct, 
		      float *mask, 
		      float specmax){
  int i,n=p->n;
  float localmax=NEGINF;
  static int seq=0;

  float *minseed=alloca(sizeof(float)*p->total_octave_lines);
  float *maxseed=alloca(sizeof(float)*p->total_octave_lines);
  for(i=0;i<p->total_octave_lines;i++)minseed[i]=maxseed[i]=NEGINF;

  /* Find the highest peak so we know the limits */
  for(i=0;i<n;i++)
    if(fft[i]>localmax)localmax=fft[i];
  if(specmax<localmax)specmax=localmax;

  /* noise masking */
  if(p->vi->noisemaskp){
    bark_noise_median(n,p->bark,mdct,mask,
		      p->vi->noisewindowlo,
		      p->vi->noisewindowhi,
		      p->vi->noisewindowlomin,
		      p->vi->noisewindowhimin,
		      p->noisemedian,
		      p->noiseoffset,
		      (const long *)(p->vi->noisewindowfixed));
    /* suppress any noise curve > specmax+p->vi->noisemaxsupp */
    for(i=0;i<n;i++)
      if(mask[i]>specmax+p->vi->noisemaxsupp)
	mask[i]=specmax+p->vi->noisemaxsupp;
    _analysis_output("noise",seq,mask,n,0,0);
  }else{
    for(i=0;i<n;i++)mask[i]=NEGINF;
  }

  /* set the ATH (floating below localmax, not global max by a
     specified att) */
  if(p->vi->ath){
    float att=localmax+p->vi->ath_adjatt;
    if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;

    for(i=0;i<n;i++){
      float av=p->ath[i]+att;
      if(av>mask[i])mask[i]=av;
    }
  }


  /* tone/peak masking */

  /* XXX apply decay to the fft here */

  seed_loop(p,
	    (const float ***)p->tonecurves,
	    (const float **)p->peakatt,fft,mask,minseed,maxseed,specmax);
  bound_loop(p,mdct,maxseed,mask,p->vi->bound_att_dB);
  max_seeds(p,minseed,maxseed,mask);

  /* doing this here is clean, but we need to find a faster way to do
     it than to just tack it on */

  for(i=0;i<n;i++)if(mdct[i]>=mask[i])break;
  if(i==n)
    for(i=0;i<n;i++)mask[i]=NEGINF;
  else
    for(i=0;i<n;i++)fft[i]=max(mdct[i],fft[i]);
  seq++;

  return(specmax);
}

float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd){
  vorbis_info *vi=vd->vi;
  codec_setup_info *ci=vi->codec_setup;
  int n=ci->blocksizes[vd->W]/2;
  float secs=(float)n/vi->rate;

  amp+=secs*ci->ampmax_att_per_sec;
  if(amp<-9999)amp=-9999;
  return(amp);
}



