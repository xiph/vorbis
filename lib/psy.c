/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.32 2000/11/14 00:05:31 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

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

/* Set up decibel threshhold slopes on a Bark frequency scale */
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

static void linear_curve(float *c){
  int i;  
  for(i=0;i<EHMER_MAX;i++)
    if(c[i]<=-200.)
      c[i]=0.;
    else
      c[i]=fromdB(c[i]);
}

static void interp_curve(float *c,float *c1,float *c2,float del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.-del);
}

static void setup_curve(float **c,
			int band,
			float *curveatt_dB){
  int i,j;
  float ath[EHMER_MAX];
  float tempc[P_LEVELS][EHMER_MAX];

  memcpy(c[0],c[4],sizeof(float)*EHMER_MAX);
  memcpy(c[2],c[4],sizeof(float)*EHMER_MAX);

  /* we add back in the ATH to avoid low level curves falling off to
     -infinity and unneccessarily cutting off high level curves in the
     curve limiting (last step).  But again, remember... a half-band's
     settings must be valid over the whole band, and it's better to
     mask too little than too much, so be pessimal. */

  for(i=0;i<EHMER_MAX;i++){
    float oc_min=band*.5-1+(i-EHMER_OFFSET)*.125;
    float oc_max=band*.5-1+(i-EHMER_OFFSET+1)*.125;
    float bark=toBARK(fromOC(oc_min));
    int ibark=floor(bark);
    float del=bark-ibark;
    float ath_min,ath_max;

    if(ibark<26)
      ath_min=ATH_Bark_dB[ibark]*(1.-del)+ATH_Bark_dB[ibark+1]*del;
    else
      ath_min=200.;

    bark=toBARK(fromOC(oc_max));
    ibark=floor(bark);
    del=bark-ibark;

    if(ibark<26)
      ath_max=ATH_Bark_dB[ibark]*(1.-del)+ATH_Bark_dB[ibark+1]*del;
    else
      ath_max=200.;

    ath[i]=min(ath_min,ath_max);
  }

  /* The c array is comes in as dB curves at 20 40 60 80 100 dB.
     interpolate intermediate dB curves */
  for(i=1;i<P_LEVELS;i+=2){
    interp_curve(c[i],c[i-1],c[i+1],.5);
  }

  /* normalize curves so the driving amplitude is 0dB */
  /* make temp curves with the ATH overlayed */
  for(i=0;i<P_LEVELS;i++){
    attenuate_curve(c[i],curveatt_dB[i]);
    memcpy(tempc[i],ath,EHMER_MAX*sizeof(float));
    attenuate_curve(tempc[i],-i*10.);
    max_curve(tempc[i],c[i]);
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
    min_curve(c[j],tempc[j]);
  }

  /* take things out of dB domain into linear amplitude */
  for(i=0;i<P_LEVELS;i++)
    linear_curve(c[i]);

}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate){
  long i,j;
  memset(p,0,sizeof(vorbis_look_psy));
  p->ath=_ogg_malloc(n*sizeof(float));
  p->octave=_ogg_malloc(n*sizeof(int));
  p->bark=_ogg_malloc(n*sizeof(float));
  p->vi=vi;
  p->n=n;

  /* set up the lookups for a given blocksize and sample rate */
  /* Vorbis max sample rate is limited by 26 Bark (54kHz) */
  set_curve(ATH_Bark_dB, p->ath,n,rate);
  for(i=0;i<n;i++)
    p->ath[i]=fromdB(p->ath[i]);
  for(i=0;i<n;i++)
    p->bark[i]=toBARK(rate/(2*n)*i); 

  for(i=0;i<n;i++){
    int oc=toOC((i+.5)*rate/(2*n))*2.+2; /* half octaves, actually */
    if(oc<0)oc=0;
    if(oc>=P_BANDS)oc=P_BANDS-1;
    p->octave[i]=oc;
  }  

  p->tonecurves=_ogg_malloc(P_BANDS*sizeof(float **));
  p->noiseatt=_ogg_malloc(P_BANDS*sizeof(float **));
  p->peakatt=_ogg_malloc(P_BANDS*sizeof(float *));
  for(i=0;i<P_BANDS;i++){
    p->tonecurves[i]=_ogg_malloc(P_LEVELS*sizeof(float *));
    p->noiseatt[i]=_ogg_malloc(P_LEVELS*sizeof(float));
    p->peakatt[i]=_ogg_malloc(P_LEVELS*sizeof(float));
  }

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++){
      p->tonecurves[i][j]=_ogg_malloc(EHMER_MAX*sizeof(float));
    }

  /* OK, yeah, this was a silly way to do it */
  memcpy(p->tonecurves[0][4],tone_125_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][6],tone_125_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][8],tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[0][10],tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[2][4],tone_125_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][6],tone_125_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][8],tone_125_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[2][10],tone_125_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[4][4],tone_250_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][6],tone_250_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][8],tone_250_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[4][10],tone_250_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[6][4],tone_500_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][6],tone_500_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][8],tone_500_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[6][10],tone_500_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[8][4],tone_1000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][6],tone_1000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][8],tone_1000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[8][10],tone_1000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[10][4],tone_2000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][6],tone_2000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][8],tone_2000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[10][10],tone_2000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[12][4],tone_4000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][6],tone_4000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][8],tone_4000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[12][10],tone_4000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[14][4],tone_8000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][6],tone_8000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][8],tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[14][10],tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);

  memcpy(p->tonecurves[16][4],tone_8000_40dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][6],tone_8000_60dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][8],tone_8000_80dB_SL,sizeof(float)*EHMER_MAX);
  memcpy(p->tonecurves[16][10],tone_8000_100dB_SL,sizeof(float)*EHMER_MAX);

  /* interpolate curves between */
  for(i=1;i<P_BANDS;i+=2)
    for(j=4;j<P_LEVELS;j+=2){
      memcpy(p->tonecurves[i][j],p->tonecurves[i-1][j],EHMER_MAX*sizeof(float));
      /*interp_curve(p->tonecurves[i][j],
		   p->tonecurves[i-1][j],
		   p->tonecurves[i+1][j],.5);*/
      min_curve(p->tonecurves[i][j],p->tonecurves[i+1][j]);
      /*min_curve(p->tonecurves[i][j],p->tonecurves[i-1][j]);*/
    }

  /*for(i=0;i<P_BANDS-1;i++)
    for(j=4;j<P_LEVELS;j+=2)
    min_curve(p->tonecurves[i][j],p->tonecurves[i+1][j]);*/

  /* set up the final curves */
  for(i=0;i<P_BANDS;i++)
    setup_curve(p->tonecurves[i],i,vi->toneatt[i]);

  /* set up attenuation levels */
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++){
      p->peakatt[i][j]=fromdB(p->vi->peakatt[i][j]);
      p->noiseatt[i][j]=fromdB(p->vi->noiseatt[i][j]);
    }

}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  _ogg_free(p->tonecurves[i][j]);
	}
	_ogg_free(p->noiseatt[i]);
	_ogg_free(p->tonecurves[i]);
	_ogg_free(p->peakatt[i]);
      }
      _ogg_free(p->tonecurves);
      _ogg_free(p->noiseatt);
      _ogg_free(p->peakatt);
    }
    memset(p,0,sizeof(vorbis_look_psy));
  }
}

static void compute_decay_fixed(vorbis_look_psy *p,float *f, float *decay, int n){
  /* handle decay */
  int i;
  float decscale=fromdB(p->vi->decay_coeff*n); 
  float attscale=1./fromdB(p->vi->attack_coeff); 

  for(i=10;i<n;i++){
    float pre=decay[i];
    if(decay[i]){
      float val=decay[i]*decscale;
      float att=fabs(f[i]/val);

      if(att>attscale)
	decay[i]=fabs(f[i]/attscale);
      else
	decay[i]=val;
    }else{
      decay[i]=fabs(f[i]/attscale);
    }
    if(pre>f[i])f[i]=pre;
  }
}

static long _eights[EHMER_MAX+1]={
  981,1069,1166,1272,
  1387,1512,1649,1798,
  1961,2139,2332,2543,
  2774,3025,3298,3597,
  3922,4277,4664,5087,
  5547,6049,6597,7194,
  7845,8555,9329,10173,
  11094,12098,13193,14387,
  15689,17109,18658,20347,
  22188,24196,26386,28774,
  31379,34219,37316,40693,
  44376,48393,52772,57549,
  62757,68437,74631,81386,
  88752,96785,105545,115097,
  125515};

static int seed_curve(float *flr,
		      float **curves,
		       float amp,float specmax,
		       int x,int n,float specatt,
		       int maxEH){
  int i;
  float *curve;

  /* make this attenuation adjustable */
  int choice=(int)((todB(amp)-specmax+specatt)/10.+.5);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);

  for(i=maxEH;i>=0;i--)
    if(((x*_eights[i])>>12)<n)break;
  maxEH=i;
  curve=curves[choice];

  for(;i>=0;i--)
    if(curve[i]>0.)break;
  
  for(;i>=0;i--){
    float lin=curve[i];
    if(lin>0.){
      float *fp=flr+((x*_eights[i])>>12);
      lin*=amp;	
      if(*fp<lin)*fp=lin;
    }else break;
  }    
  return(maxEH);
}

static void seed_peak(float *flr,
		      float *att,
		      float amp,float specmax,
		      int x,int n,float specatt){
  int prevx=(x*_eights[16])>>12;

  /* make this attenuation adjustable */
  int choice=rint((todB(amp)-specmax+specatt)/10.+.5);
  if(choice<0)choice=0;
  if(choice>=P_LEVELS)choice=P_LEVELS-1;

  if(prevx<n){
    float lin=att[choice];
    if(lin){
      lin*=amp;	
      if(flr[prevx]<lin)flr[prevx]=lin;
    }
  }
}

static void seed_generic(vorbis_look_psy *p,
			 float ***curves,
			 float *f, 
			 float *flr,
			 float *seeds,
			 float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  int maxEH=EHMER_MAX-1;

  /* prime the working vector with peak values */
  /* Use the 125 Hz curve up to 125 Hz and 8kHz curve after 8kHz. */
  for(i=0;i<n;i++)
    if(f[i]>flr[i])
      maxEH=seed_curve(seeds,curves[p->octave[i]],
		       f[i],specmax,i,n,vi->max_curve_dB,maxEH);
}

static void seed_att(vorbis_look_psy *p,
		     float **att,
		     float *f, 
		     float *flr,
		     float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  
  for(i=0;i<n;i++)
    if(f[i]>flr[i])
      seed_peak(flr,att[p->octave[i]],f[i],
		specmax,i,n,vi->max_curve_dB);
}

static void seed_point(vorbis_look_psy *p,
		     float **att,
		     float *f, 
		     float *flr,
		     float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  
  for(i=0;i<n;i++){
    /* make this attenuation adjustable */
    int choice=rint((todB(f[i])-specmax+vi->max_curve_dB)/10.+.5);
    float lin;
    if(choice<0)choice=0;
    if(choice>=P_LEVELS)choice=P_LEVELS-1;
    lin=att[p->octave[i]][choice]*f[i];
    if(flr[i]<lin)flr[i]=lin;
  }
}

/* bleaugh, this is more complicated than it needs to be */
static void max_seeds(vorbis_look_psy *p,float *seeds,float *flr){
  long n=p->n,i,j;
  long *posstack=alloca(n*sizeof(long));
  float *ampstack=alloca(n*sizeof(float));
  long stack=0;

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
	  if(i<posstack[stack-1]*1.0905077080){
	    if(stack>1 && ampstack[stack-1]<ampstack[stack-2] &&
	       i<posstack[stack-2]*1.0905077080){
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
  {
    long pos=0;
    for(i=0;i<stack;i++){
      long endpos;
      if(i<stack-1 && ampstack[i+1]>ampstack[i]){
	endpos=posstack[i+1];
      }else{
	endpos=posstack[i]*1.0905077080+1; /* +1 is important, else bin 0 is
                                       discarded in short frames */
      }
      if(endpos>n)endpos=n;
      for(j=pos;j<endpos;j++)
	if(flr[j]<ampstack[i])
	  flr[j]=ampstack[i];
      pos=endpos;
    }
  }   

  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */
}

static void bark_noise(long n,float *b,float *f,float *noise){
  long i=1,lo=0,hi=2;
  float acc=0.,val,del=0.;

  float *norm=alloca(n*sizeof(float));

  memset(noise,0,n*sizeof(float));
  memset(norm,0,n*sizeof(float));

  while(hi<n){
    val=todB_nn(f[i]*f[i])+400.;
    del=1./(i-lo);
    noise[lo]+=val*del;
    noise[i]-=val*del;
    norm[lo]+=del;
    norm[i]-=del;
 
    del=1./(hi-i);
    noise[i]-=val*del;
    noise[hi]+=val*del;
    norm[hi]+=del;
    norm[i]-=del;
    

    i++;
    for(;hi<n && b[hi]-.3<b[i];hi++);
    for(;lo<i-1 && b[lo]+.3<b[i];lo++);
    if(i==hi)hi++;
  }

  {
    long ilo=i-lo;
    long hii=hi-i;

    for(;i<n;i++){
      val=todB_nn(f[i]*f[i])+400.;
      del=1./(hii);
      noise[i]-=val*del;
      norm[i]-=del;
     
      del=1./(ilo);
      noise[i-ilo]+=val*del;
      noise[i]-=val*del;      
      norm[i-ilo]+=del;
      norm[i]-=del;      
    }
    for(i=1,lo=n-ilo;lo<n;lo++,i++){
      val=todB_nn(f[n-i]*f[n-i])+400.;
      del=1./ilo;
      noise[lo]+=val*del;
      norm[lo]+=del;
    }
  }


  acc=0;
  val=0;

  for(i=0;i<n;i++){
    val+=norm[i];
    norm[i]=val;
    acc+=noise[i];
    noise[i]=acc;
  }

  val=0;
  acc=0;
  for(i=0;i<n;i++){
    val+=norm[i];
    acc+=noise[i];
    if(val==0){
      noise[i]=0.;
      norm[i]=0;
    }else{
      float v=acc/val-400;
      noise[i]=sqrt(fromdB(v));
    }
  }
}

void _vp_compute_mask(vorbis_look_psy *p,float *f, 
		      float *flr, 
		      float *decay){
  float *smooth=alloca(sizeof(float)*p->n);
  int i,n=p->n;
  float specmax=0.;
  static int seq=0;

  float *seed=alloca(sizeof(float)*p->n);
  float *seed2=alloca(sizeof(float)*p->n);

  _analysis_output("mdct",seq,f,n,1,1);
  memset(flr,0,n*sizeof(float));

  /* noise masking */
  if(p->vi->noisemaskp){
    memset(seed,0,n*sizeof(float));
    bark_noise(n,p->bark,f,seed);
    seed_point(p,p->noiseatt,seed,flr,specmax);

  }

  /* smooth the data is that's called for ********************************/
  for(i=0;i<n;i++)smooth[i]=fabs(f[i]);
  if(p->vi->smoothp){
    /* compute power^.5 of three neighboring bins to smooth for peaks
       that get split twixt bins/peaks that nail the bin.  This evens
       out treatment as we're not doing additive masking any longer. */
    float acc=smooth[0]*smooth[0]+smooth[1]*smooth[1];
    float prev=smooth[0];

    smooth[0]=sqrt(acc);
    for(i=1;i<n-1;i++){
      float this=smooth[i];
      acc+=smooth[i+1]*smooth[i+1];
      if(acc<0)acc=0; /* it can happen due to finite precision */
      smooth[i]=sqrt(acc);
      acc-=prev*prev;
      prev=this;
    }
    if(acc<0)acc=0; /* in case it happens on the final iteration */
    smooth[n-1]=sqrt(acc);
  }

  _analysis_output("smooth",seq,smooth,n,1,1);

  /* find the highest peak so we know the limits *************************/
  for(i=0;i<n;i++){
    if(smooth[i]>specmax)specmax=smooth[i];
  }
  specmax=todB(specmax);

  /* set the ATH (floating below specmax by a specified att) */
  if(p->vi->athp){
    float att=specmax+p->vi->ath_adjatt;
    if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;
    att=fromdB(att);

    for(i=0;i<n;i++){
      float av=p->ath[i]*att;
      if(av>flr[i])flr[i]=av;
    }
  }

  _analysis_output("ath",seq,flr,n,1,1);

  /* peak attenuation ******/
  if(p->vi->peakattp){
    memset(seed,0,n*sizeof(float));
    seed_att(p,p->peakatt,smooth,seed,specmax);
    max_seeds(p,seed,flr);
  }

  /* tone masking */
  if(p->vi->tonemaskp){
    memset(seed,0,n*sizeof(float));
    memset(seed2,0,n*sizeof(float));

    seed_generic(p,p->tonecurves,smooth,flr,seed2,specmax);
    max_seeds(p,seed2,seed2);

    for(i=0;i<n;i++)if(seed2[i]<flr[i])seed2[i]=flr[i];
    for(i=0;i<n;i++)if(seed2[i]<decay[i])seed2[i]=decay[i];

    seed_generic(p,p->tonecurves,smooth,seed2,seed,specmax);
    max_seeds(p,seed,seed);
    
    if(p->vi->decayp)
      compute_decay_fixed(p,seed,decay,n);
    
    for(i=0;i<n;i++)if(flr[i]<seed[i])flr[i]=seed[i];
    
  }

  _analysis_output("final",seq,flr,n,1,1);

  /* doing this here is clean, but we need to find a faster way to do
     it than to just tack it on */

  for(i=0;i<n;i++)if(2.*f[i]>flr[i] || -2.*f[i]>flr[i])break;
  if(i==n)memset(flr,0,sizeof(float)*n);

  seq++;
}


/* this applies the floor and (optionally) tries to preserve noise
   energy in low resolution portions of the spectrum */
/* f and flr are *linear* scale, not dB */
void _vp_apply_floor(vorbis_look_psy *p,float *f, float *flr){
  float *work=alloca(p->n*sizeof(float));
  int j;

  /* subtract the floor */
  for(j=0;j<p->n;j++){
    if(flr[j]<=0)
      work[j]=0.;
    else
      work[j]=f[j]/flr[j];
  }

  memcpy(f,work,p->n*sizeof(float));
}


