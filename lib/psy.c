/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.23 2000/06/19 10:05:57 xiphmont Exp $

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

/* Why Bark scale for encoding but not masking? Because masking has a
   strong harmonic dependancy */

/* the beginnings of real psychoacoustic infrastructure.  This is
   still not tightly tuned */
void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    free(i);
  }
}

/* Set up decibel threshhold slopes on a Bark frequency scale */
/* the only bit left on a Bark scale.  No reason to change it right now */
static void set_curve(double *ref,double *c,int n, double crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK(i+1)*2*n/crate);
    double base=ref[i];
    if(j<endpos){
      double delta=(ref[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
	c[j]=base;
	base+=delta;
      }
    }
  }
}

static void min_curve(double *c,
		       double *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(double *c,
		       double *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(double *c,double att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

static void linear_curve(double *c){
  int i;  
  for(i=0;i<EHMER_MAX;i++)
    if(c[i]<=-900.)
      c[i]=0.;
    else
      c[i]=fromdB(c[i]);
}

static void interp_curve_dB(double *c,double *c1,double *c2,double del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=fromdB(todB(c2[i])*del+todB(c1[i])*(1.-del));
}

static void interp_curve(double *c,double *c1,double *c2,double del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.-del);
}

static void setup_curve(double **c,
			int oc,
			double *curveatt_dB){
  int i,j;
  double tempc[9][EHMER_MAX];
  double ath[EHMER_MAX];

  for(i=0;i<EHMER_MAX;i++){
    double bark=toBARK(fromOC(oc*.5+(i-EHMER_OFFSET)*.125));
    int ibark=floor(bark);
    double del=bark-ibark;
    if(ibark<26)
      ath[i]=ATH_Bark_dB[ibark]*(1.-del)+ATH_Bark_dB[ibark+1]*del;
    else
      ath[i]=200;
  }

  memcpy(c[0],c[2],sizeof(double)*EHMER_MAX);

  /* the temp curves are a bit roundabout, but this is only in
     init. */
  for(i=0;i<5;i++){
    memcpy(tempc[i*2],c[i*2],sizeof(double)*EHMER_MAX);
    attenuate_curve(tempc[i*2],curveatt_dB[i]+(i+1)*20);
    max_curve(tempc[i*2],ath);
    attenuate_curve(tempc[i*2],-(i+1)*20);
  }

  /* normalize them so the driving amplitude is 0dB */
  for(i=0;i<5;i++){
    attenuate_curve(c[i*2],curveatt_dB[i]);
  }

  /* The c array is comes in as dB curves at 20 40 60 80 100 dB.
     interpolate intermediate dB curves */
  for(i=0;i<7;i+=2){
    interp_curve(c[i+1],c[i],c[i+2],.5);
    interp_curve(tempc[i+1],tempc[i],tempc[i+2],.5);
  }

  /* take things out of dB domain into linear amplitude */
  for(i=0;i<9;i++)
    linear_curve(c[i]);
  for(i=0;i<9;i++)
    linear_curve(tempc[i]);
      
  /* Now limit the louder curves.

     the idea is this: We don't know what the playback attenuation
     will be; 0dB SL moves every time the user twiddles the volume
     knob. So that means we have to use a single 'most pessimal' curve
     for all masking amplitudes, right?  Wrong.  The *loudest* sound
     can be in (we assume) a range of ...+100dB] SL.  However, sounds
     20dB down will be in a range ...+80], 40dB down is from ...+60],
     etc... */

  for(i=8;i>=0;i--){
    for(j=0;j<i;j++)
      min_curve(c[i],tempc[j]);
  }
}


void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate){
  long i,j;
  double rate2=rate/2.;
  memset(p,0,sizeof(vorbis_look_psy));
  p->ath=malloc(n*sizeof(double));
  p->octave=malloc(n*sizeof(int));
  p->vi=vi;
  p->n=n;

  /* set up the lookups for a given blocksize and sample rate */
  /* Vorbis max sample rate is limited by 26 Bark (54kHz) */
  set_curve(ATH_Bark_dB, p->ath,n,rate);
  for(i=0;i<n;i++)
    p->ath[i]=fromdB(p->ath[i]+vi->ath_att);

  for(i=0;i<n;i++){
    int oc=rint(toOC((i+.5)*rate2/n)*2.);
    if(oc<0)oc=0;
    if(oc>12)oc=12;
    p->octave[i]=oc;
  }  

  p->tonecurves=malloc(13*sizeof(double **));
  p->noisecurves=malloc(13*sizeof(double **));
  p->peakatt=malloc(7*sizeof(double *));
  for(i=0;i<13;i++){
    p->tonecurves[i]=malloc(9*sizeof(double *));
    p->noisecurves[i]=malloc(9*sizeof(double *));
  }
  for(i=0;i<7;i++)
    p->peakatt[i]=malloc(5*sizeof(double));

  for(i=0;i<13;i++)
    for(j=0;j<9;j++){
      p->tonecurves[i][j]=malloc(EHMER_MAX*sizeof(double));
      p->noisecurves[i][j]=malloc(EHMER_MAX*sizeof(double));
    }

  /* OK, yeah, this was a silly way to do it */
  memcpy(p->tonecurves[0][2],tone_125_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[0][4],tone_125_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[0][6],tone_125_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[0][8],tone_125_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[2][2],tone_250_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[2][4],tone_250_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[2][6],tone_250_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[2][8],tone_250_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[4][2],tone_500_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[4][4],tone_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[4][6],tone_500_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[4][8],tone_500_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[6][2],tone_1000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[6][4],tone_1000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[6][6],tone_1000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[6][8],tone_1000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[8][2],tone_2000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[8][4],tone_2000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[8][6],tone_2000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[8][8],tone_2000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[10][2],tone_4000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[10][4],tone_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[10][6],tone_4000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[10][8],tone_4000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->tonecurves[12][2],tone_4000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[12][4],tone_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[12][6],tone_8000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->tonecurves[12][8],tone_8000_100dB_SL,sizeof(double)*EHMER_MAX);


  memcpy(p->noisecurves[0][2],noise_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[0][4],noise_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[0][6],noise_500_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[0][8],noise_500_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->noisecurves[2][2],noise_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[2][4],noise_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[2][6],noise_500_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[2][8],noise_500_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->noisecurves[4][2],noise_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[4][4],noise_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[4][6],noise_500_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[4][8],noise_500_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->noisecurves[6][2],noise_1000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[6][4],noise_1000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[6][6],noise_1000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[6][8],noise_1000_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->noisecurves[8][2],noise_2000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[8][4],noise_2000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[8][6],noise_2000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[8][8],noise_2000_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->noisecurves[10][2],noise_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[10][4],noise_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[10][6],noise_4000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[10][8],noise_4000_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->noisecurves[12][2],noise_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[12][4],noise_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[12][6],noise_4000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->noisecurves[12][8],noise_4000_80dB_SL,sizeof(double)*EHMER_MAX);

  setup_curve(p->tonecurves[0],0,vi->toneatt_125Hz);
  setup_curve(p->tonecurves[2],2,vi->toneatt_250Hz);
  setup_curve(p->tonecurves[4],4,vi->toneatt_500Hz);
  setup_curve(p->tonecurves[6],6,vi->toneatt_1000Hz);
  setup_curve(p->tonecurves[8],8,vi->toneatt_2000Hz);
  setup_curve(p->tonecurves[10],10,vi->toneatt_4000Hz);
  setup_curve(p->tonecurves[12],12,vi->toneatt_8000Hz);

  setup_curve(p->noisecurves[0],0,vi->noiseatt_125Hz);
  setup_curve(p->noisecurves[2],2,vi->noiseatt_250Hz);
  setup_curve(p->noisecurves[4],4,vi->noiseatt_500Hz);
  setup_curve(p->noisecurves[6],6,vi->noiseatt_1000Hz);
  setup_curve(p->noisecurves[8],8,vi->noiseatt_2000Hz);
  setup_curve(p->noisecurves[10],10,vi->noiseatt_4000Hz);
  setup_curve(p->noisecurves[12],12,vi->noiseatt_8000Hz);

  for(i=1;i<13;i+=2)
    for(j=0;j<9;j++){
      interp_curve_dB(p->tonecurves[i][j],
		      p->tonecurves[i-1][j],
		      p->tonecurves[i+1][j],.5);
      interp_curve_dB(p->noisecurves[i][j],
		      p->noisecurves[i-1][j],
		      p->noisecurves[i+1][j],.5);
    }
  for(i=0;i<5;i++){
    p->peakatt[0][i]=fromdB(p->vi->peakatt_125Hz[i]);
    p->peakatt[1][i]=fromdB(p->vi->peakatt_250Hz[i]);
    p->peakatt[2][i]=fromdB(p->vi->peakatt_500Hz[i]);
    p->peakatt[3][i]=fromdB(p->vi->peakatt_1000Hz[i]);
    p->peakatt[4][i]=fromdB(p->vi->peakatt_2000Hz[i]);
    p->peakatt[5][i]=fromdB(p->vi->peakatt_4000Hz[i]);
    p->peakatt[6][i]=fromdB(p->vi->peakatt_8000Hz[i]);
  }
}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)free(p->ath);
    if(p->octave)free(p->octave);
    if(p->noisecurves){
      for(i=0;i<13;i++){
	for(j=0;j<9;j++){
	  free(p->tonecurves[i][j]);
	  free(p->noisecurves[i][j]);
	}
	free(p->noisecurves[i]);
	free(p->tonecurves[i]);
      }
      for(i=0;i<7;i++)
	free(p->peakatt[i]);
      free(p->tonecurves);
      free(p->noisecurves);
      free(p->peakatt);
    }
    memset(p,0,sizeof(vorbis_look_psy));
  }
}

static void compute_decay(vorbis_look_psy *p,double *f, double *decay, int n){
  /* handle decay */
  int i;
  double decscale=1.-pow(p->vi->decay_coeff,n); 
  double attscale=1.-pow(p->vi->attack_coeff,n); 
  for(i=0;i<n;i++){
    double del=f[i]-decay[i];
    if(del>0)
      /* add energy */
      decay[i]+=del*attscale;
    else
      /* remove energy */
      decay[i]+=del*decscale;
    if(decay[i]>f[i])f[i]=decay[i];
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

static int seed_curve(double *flr,
		       double **curves,
		       double amp,double specmax,
		       int x,int n,double specatt,
		       int maxEH){
  int i;

  /* make this attenuation adjustable */
  int choice=(int)((todB(amp)-specmax+specatt)/10.-1.5);
  choice=max(choice,0);
  choice=min(choice,8);

  for(i=maxEH;i>=0;i--)
    if(((x*_eights[i])>>12)<n)break;
  maxEH=i;

  for(;i>=0;i--)
    if(curves[choice][i]>0.)break;
  
  for(;i>=0;i--){
    double lin=curves[choice][i];
    if(lin>0.){
      double *fp=flr+((x*_eights[i])>>12);
      lin*=amp;	
      if(*fp<lin)*fp=lin;
    }else break;
  }    
  return(maxEH);
}

static void seed_peak(double *flr,
		      double *att,
		      double amp,double specmax,
		      int x,int n,double specatt){
  int prevx=(x*_eights[16])>>12;
  int nextx=(x*_eights[17])>>12;

  /* make this attenuation adjustable */
  int choice=rint((todB(amp)-specmax+specatt)/20.)-1;
  if(choice<0)choice=0;
  if(choice>4)choice=4;

  if(prevx<n){
    double lin=att[choice];
    if(lin){
      lin*=amp;	
      if(prevx<0){
	if(nextx>=0){
	  if(flr[0]<lin)flr[0]=lin;
	}
      }else{
	if(flr[prevx]<lin)flr[prevx]=lin;
      }
    }
  }
}

static void seed_generic(vorbis_look_psy *p,
			 double ***curves,
			 double *f, 
			 double *flr,
			 double specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  int maxEH=EHMER_MAX-1;

  /* prime the working vector with peak values */
  /* Use the 125 Hz curve up to 125 Hz and 8kHz curve after 8kHz. */
  for(i=0;i<n;i++)
    if(f[i]>flr[i])
      maxEH=seed_curve(flr,curves[p->octave[i]],f[i],
		       specmax,i,n,vi->max_curve_dB,maxEH);
}

static void seed_att(vorbis_look_psy *p,
		     double *f, 
		     double *flr,
		     double specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  
  for(i=0;i<n;i++)
    if(f[i]>flr[i])
      seed_peak(flr,p->peakatt[(p->octave[i]+1)>>1],f[i],
		specmax,i,n,vi->max_curve_dB);
}

/* bleaugh, this is more complicated than it needs to be */
static void max_seeds(vorbis_look_psy *p,double *flr){
  long n=p->n,i,j;
  long *posstack=alloca(n*sizeof(long));
  double *ampstack=alloca(n*sizeof(double));
  long stack=0;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=flr[i];
    }else{
      while(1){
	if(flr[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=flr[i];
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
	  ampstack[stack++]=flr[i];
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
      for(j=pos;j<endpos;j++)flr[j]=ampstack[i];
      pos=endpos;
    }
  }   

  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */
}

#define noiseBIAS 2
static void quarter_octave_noise(long n,double *f,double *noise){
  long i;
  long lo=0,hi=0;
  double acc=0.;

  for(i=0;i<n;i++){
    /* not exactly correct, (the center frequency should be centered
       on a *log* scale), but not worth quibbling */
    long newhi=((i*_eights[17])>>12)+noiseBIAS;
    long newlo=((i*_eights[15])>>12)-noiseBIAS;
    if(newhi>n)newhi=n;

    for(;lo<newlo;lo++)
      acc-=todB(f[lo]); /* yeah, this ain't RMS */
    for(;hi<newhi;hi++)
      acc+=todB(f[hi]);
    noise[i]=fromdB(acc/(hi-lo));
  }
}

/* stability doesn't matter */
static int comp(const void *a,const void *b){
  if(fabs(**(double **)a)<fabs(**(double **)b))
    return(1);
  else
    return(-1);
}

/* move ath and absolute masking to 'apply_floor' to avoid confusion
   with noise fitting and a floor that warbles due to bad LPC fit */
static int frameno=-1;
void _vp_compute_mask(vorbis_look_psy *p,double *f, 
		      double *flr, 
		      double *decay){
  double *work=alloca(sizeof(double)*p->n);
  double *work2=alloca(sizeof(double)*p->n);
  int i,n=p->n;
  double specmax=0.;

  frameno++;
  memset(flr,0,n*sizeof(double));

  for(i=0;i<n;i++)work[i]=fabs(f[i]);
  
  /* find the highest peak so we know the limits */
  for(i=0;i<n;i++){
    if(work[i]>specmax)specmax=work[i];
  }
  specmax=todB(specmax);
  
  /* don't use the smoothed data for noise */
  if(p->vi->noisemaskp){
    quarter_octave_noise(p->n,f,work2);
    seed_generic(p,p->noisecurves,work2,flr,specmax);
  }
  
  /* ... or peak att */
  if(p->vi->peakattp)
    seed_att(p,work,flr,specmax);
  
  if(p->vi->smoothp){
    /* compute power^.5 of three neighboring bins to smooth for peaks
       that get split twixt bins/peaks that nail the bin.  This evens
       out treatment as we're not doing additive masking any longer. */
    double acc=work[0]*work[0]+work[1]*work[1];
    double prev=work[0];

    work[0]=sqrt(acc);
    for(i=1;i<n-1;i++){
      double this=work[i];
      acc+=work[i+1]*work[i+1];
      work[i]=sqrt(acc);
      acc-=prev*prev;
      prev=this;
    }
    work[n-1]=sqrt(acc);
  }
  

  /* seed the tone masking */
  if(p->vi->tonemaskp){
    if(p->vi->decayp){
      
      memset(work2,0,n*sizeof(double));
      seed_generic(p,p->tonecurves,work,work2,specmax);
      
      /* chase the seeds */
      max_seeds(p,flr);
      max_seeds(p,work2);
    
      /* compute, update and apply decay accumulator */
      compute_decay(p,work2,decay,n);
      for(i=0;i<n;i++)if(flr[i]<work2[i])flr[i]=work2[i];
      
    }else{
      
      seed_generic(p,p->tonecurves,work,flr,specmax);
      
      /* chase the seeds */
      max_seeds(p,flr);
    }
  }else{
    max_seeds(p,flr);
  }
}


/* this applies the floor and (optionally) tries to preserve noise
   energy in low resolution portions of the spectrum */
/* f and flr are *linear* scale, not dB */
void _vp_apply_floor(vorbis_look_psy *p,double *f, double *flr){
  double *work=alloca(p->n*sizeof(double));
  double thresh=fromdB(p->vi->noisefit_threshdB);
  int i,j,addcount=0;
  thresh*=thresh;

  /* subtract the floor */
  for(j=0;j<p->n;j++){
    if(flr[j]<=0)
      work[j]=0.;
    else
      work[j]=f[j]/flr[j];
  }

  /* mask off the ATH.  This should be floating below specmax too, but
     for now, 0dB is fixed... */
  if(p->vi->athp)
    for(j=0;j<p->n;j++)
      if(fabs(f[j])<p->ath[j]){
	/* zeroes can cause rounding stability issues */
	if(f[j]>0)
	  work[j]=.1;
	else
	  if(f[j]<0)
	    work[j]=-.1;
      }

  /* look at spectral energy levels.  Noise is noise; sensation level
     is important */
  if(p->vi->noisefitp){
    double **index=alloca(p->vi->noisefit_subblock*sizeof(double *));

    /* we're looking for zero values that we want to reinstate (to
       floor level) in order to raise the SL noise level back closer
       to original.  Desired result; the SL of each block being as
       close to (but still less than) the original as possible.  Don't
       bother if the net result is a change of less than
       p->vi->noisefit_thresh dB */
    for(i=0;i<p->n;){
      double original_SL=0.;
      double current_SL=0.;
      int z=0;

      /* compute current SL */
      for(j=0;j<p->vi->noisefit_subblock && i<p->n;j++,i++){
	double y=(f[i]*f[i]);
	original_SL+=y;
	if(work[i]){
	  current_SL+=y;
	}else{
	  if(p->vi->athp){
	    if(fabs(f[j])>=p->ath[j])index[z++]=f+i;
	  }else
	    index[z++]=f+i;
	}	
      }

      /* sort the values below mask; add back the largest first, stop
         when we violate the desired result above (which may be
         immediately) */
      if(z && current_SL*thresh<original_SL){
	qsort(index,z,sizeof(double *),&comp);
	
	for(j=0;j<z;j++){
	  int p=index[j]-f;
	  double val=flr[p]*flr[p]+current_SL;
	  
	  if(val<original_SL){
	    addcount++;
	    if(f[p]>0)
	      work[p]=1;
	    else
	      work[p]=-1;
	    current_SL=val;
	  }else
	    break;
	}
      }
    }
  }

  memcpy(f,work,p->n*sizeof(double));
}


