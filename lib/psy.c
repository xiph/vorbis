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
 last mod: $Id: psy.c,v 1.16.2.2.2.9 2000/04/21 16:35:39 xiphmont Exp $

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

/* the beginnings of real psychoacoustic infrastructure.  This is
   still not tightly tuned */

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    free(i);
  }
}

/* Set up decibel threshhold slopes on a Bark frequency scale */
static void set_curve(double *ref,double *c,int n, double crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK(i+1)*2*n/crate);
    double base=ref[i];
    double delta=(ref[i+1]-base)/(endpos-j);
    for(;j<endpos && j<n;j++){
      c[j]=base;
      base+=delta;
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
			double *curveatt_dB,
			double peaklowrolloff,
			double peakhighrolloff){
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

  for(i=0;i<9;i++){
    memcpy(tempc[i],c[i],sizeof(double)*EHMER_MAX);
    max_curve(tempc[i],ath);
  }

  /* normalize them so the driving amplitude is 0dB */
  for(i=0;i<5;i++){
    attenuate_curve(c[i*2],curveatt_dB[i]);
    attenuate_curve(tempc[i*2],curveatt_dB[i]);
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
  p->pre=malloc(n*sizeof(int));
  p->octave=malloc(n*sizeof(double));
  p->post=malloc(n*sizeof(int));
  p->curves=malloc(11*sizeof(double));
  p->vi=vi;
  p->n=n;

  /* set up the lookups for a given blocksize and sample rate */
  /* Vorbis max sample rate is limited by 26 Bark (54kHz) */
  set_curve(ATH_Bark_dB, p->ath,n,rate);
  for(i=0;i<n;i++)
    p->ath[i]=fromdB(p->ath[i]+vi->ath_att);

  for(i=0;i<n;i++){
    double oc=toOC((i+.5)*rate2/n);
    int pre=fromOC(oc-.0625)/rate2*n;
    int post=fromOC(oc+.0625)/rate2*n;
    p->pre[i]=(pre<0?0:pre);
    p->octave[i]=oc;
    p->post[i]=(post<0?0:post);
  }  

  p->curves=malloc(11*sizeof(double **));
  for(i=0;i<11;i++)
    p->curves[i]=malloc(9*sizeof(double *));

  for(i=0;i<11;i++)
    for(j=0;j<9;j++)
      p->curves[i][j]=malloc(EHMER_MAX*sizeof(double));

  memcpy(p->curves[0][2],tone_250_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[0][4],tone_250_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[0][6],tone_250_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[0][8],tone_250_80dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->curves[2][2],tone_500_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[2][4],tone_500_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[2][6],tone_500_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[2][8],tone_500_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->curves[4][2],tone_1000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[4][4],tone_1000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[4][6],tone_1000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[4][8],tone_1000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->curves[6][2],tone_2000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[6][4],tone_2000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[6][6],tone_2000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[6][8],tone_2000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->curves[8][2],tone_4000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[8][4],tone_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[8][6],tone_4000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[8][8],tone_4000_100dB_SL,sizeof(double)*EHMER_MAX);

  memcpy(p->curves[10][2],tone_4000_40dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[10][4],tone_4000_60dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[10][6],tone_4000_80dB_SL,sizeof(double)*EHMER_MAX);
  memcpy(p->curves[10][8],tone_8000_100dB_SL,sizeof(double)*EHMER_MAX);

  setup_curve(p->curves[0],0,vi->curveatt_250Hz,vi->peakpre,vi->peakpost);
  setup_curve(p->curves[2],2,vi->curveatt_500Hz,vi->peakpre,vi->peakpost);
  setup_curve(p->curves[4],4,vi->curveatt_1000Hz,vi->peakpre,vi->peakpost);
  setup_curve(p->curves[6],6,vi->curveatt_2000Hz,vi->peakpre,vi->peakpost);
  setup_curve(p->curves[8],8,vi->curveatt_4000Hz,vi->peakpre,vi->peakpost);
  setup_curve(p->curves[10],10,vi->curveatt_8000Hz,vi->peakpre,vi->peakpost);

  for(i=1;i<11;i+=2)
    for(j=0;j<9;j++)
      interp_curve_dB(p->curves[i][j],p->curves[i-1][j],p->curves[i+1][j],.5);

}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)free(p->ath);
    if(p->octave)free(p->octave);
    if(p->pre)free(p->pre);
    if(p->post)free(p->post);
    if(p->curves){
      for(i=0;i<11;i++){
	for(j=0;j<9;j++)
	  free(p->curves[i][j]);
	free(p->curves[i]);
      }
      free(p->curves);
    }
    memset(p,0,sizeof(vorbis_look_psy));
  }
}

static double _eights[EHMER_MAX]={
  .2500000000000000000,.2726269331663144148,
  .2973017787506802667,.3242098886627524165,
  .3535533905932737622,.3855527063519852059,
  .4204482076268572715,.4585020216023356159,
  .5000000000000000000,.5452538663326288296,
  .5946035575013605334,.6484197773255048330,
  .7071067811865475244,.7711054127039704118,
  .8408964152537145430,.9170040432046712317,
  1.000000000000000000,1.090507732665257659,
  1.189207115002721066,1.296839554651009665,
  1.414213562373095048,1.542210825407940823,
  1.681792830507429085,1.834008086409342463,
  2.000000000000000000,2.181015465330515318,
  2.378414230005442133,2.593679109302019331,
  2.828427124746190097,3.084421650815881646,
  3.363585661014858171,3.668016172818684926,
  4.000000000000000000,4.362030930661030635,
  4.756828460010884265,5.187358218604038662,
  5.656854249492380193,6.168843301631763292,
  6.727171322029716341,7.336032345637369851,
  8.000000000000000000,8.724061861322061270,
  9.513656920021768529,10.37471643720807732,
  11.31370849898476038,12.33768660326352658,
  13.45434264405943268,14.67206469127473970,
  16.00000000000000000,17.44812372264412253,
  19.02731384004353705,20.74943287441615464,
  22.62741699796952076,24.67537320652705316,
  26.90868528811886536,29.34412938254947939};

static double seed_peaks(double *floor,double **curve,
		       double amp,double specmax,
		       int *pre,int *post,
		       int x,int n,double specatt){
  int i;
  int ix=x*_eights[0];
  int prevx=pre[ix];
  int nextx;
  double ret=0.;

  /* make this attenuation adjustable */
  int choice=rint((todB(amp)-specmax+specatt)/10.)-2;
  if(choice<0)choice=0;
  if(choice>8)choice=8;

  for(i=0;i<EHMER_MAX;i++){
    if(prevx<n){
      double lin=curve[choice][i];
      ix=x*_eights[i];
      nextx=(ix<n?post[ix]:n);
      if(lin){
	/* Currently uses a n+n = +3dB additivity */
	lin*=amp;
	lin*=lin;
	
	floor[prevx]+=lin;
	if(nextx==prevx){
	  if(nextx+1<n)floor[nextx+1]-=lin;
	}else{
	  if(nextx<n)floor[nextx]-=lin;
	}
	if(i==EHMER_OFFSET || prevx==x)ret+=lin;
	if(nextx==x)ret-=lin;
      }
      prevx=nextx;
    }
  }
  return(ret);
}

static void add_seeds(double *floor,int n){
  int i;
  double acc=0.;
  for(i=0;i<n;i++){
    acc+=floor[i];
    floor[i]=(acc<=0.?0.:sqrt(acc));
  }
}

/* Why Bark scale for encoding but not masking? Because masking has a
   strong harmonic dependancy */
static void _vp_tone_tone_iter(vorbis_look_psy *p,
			       double *f, 
			       double *flr, double *mask,
			       double *decay){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  double *work=alloca(n*sizeof(double));
  double specmax=0;
  double acc=0.;

  memcpy(work,f,p->n*sizeof(double));
  
  /* handle decay */
  if(vi->decayp && decay){
    double decscale=1.-pow(vi->decay_coeff,n); 
    double attscale=1.-pow(vi->attack_coeff,n); 
    for(i=0;i<n;i++){
      double del=work[i]-decay[i];
      if(del>0)
	/* add energy */
	decay[i]+=del*attscale;
      else
	/* remove energy */
	decay[i]+=del*decscale;
      if(decay[i]>work[i])work[i]=decay[i];
    }
  }

  for(i=0;i<n;i++){
    if(work[i]>specmax)specmax=work[i];
  }

  specmax=todB(specmax);
  memset(flr,0,sizeof(double)*n);

  /* prime the working vector with peak values */
  /* Use the 250 Hz curve up to 250 Hz and 8kHz curve after 8kHz. */
  for(i=0;i<n;i++){
    acc+=flr[i]; /* XXX acc is behaving incorrectly.  Check it */
    if(work[i]*work[i]>acc){
      int o=rint(p->octave[i]*2.);
      if(o<0)o=0;
      if(o>10)o=10;

      acc+=seed_peaks(flr,p->curves[o],work[i],
		      specmax,p->pre,p->post,i,n,vi->max_curve_dB);
    }
  }

  /* chase curves down from the peak seeds */
  add_seeds(flr,n);
  
  memcpy(mask,flr,n*sizeof(double));

  /* mask off the ATH */
  if(vi->athp)
    for(i=0;i<n;i++)
      if(mask[i]<p->ath[i])
	mask[i]=p->ath[i];

  /* do the peak att with rolloff hack. But I don't know which is
     better... doing it to mask or floor.  We'll assume mask right now */
  if(vi->peakattp){
    double curmask;

    /* seed peaks for rolloff first so we only do it once */
    for(i=0;i<n;i++){
      if(work[i]>mask[i]){
	double val=todB(work[i]);
	int o=p->octave[i];
	int choice=rint((val-specmax+vi->max_curve_dB)/20.)-1;
	if(o<0)o=0;
	if(o>5)o=5;
	if(choice<0)choice=0;
	if(choice>4)choice=4;
	work[i]=fromdB(val+vi->peakatt[o][choice]);
      }else{
	work[i]=0;
      }
    }

    /* chase down from peaks forward */
    curmask=0.;
    for(i=0;i<n-1;i++){
      if(work[i]>curmask)curmask=work[i];
      if(curmask>mask[i]){
	mask[i]=curmask;
	/* roll off the curmask */
	curmask*=fromdB(vi->peakpost*(p->octave[i+1]-p->octave[i]));
      }else{
	curmask=0;
      }
    }
    /* chase down from peaks backward */
    curmask=0.;
    for(i=n-1;i>0;i--){
      if(work[i]>curmask)curmask=work[i];
      if(curmask>=mask[i]){
	mask[i]=curmask;
	/* roll off the curmask */
	curmask*=fromdB(vi->peakpre*(p->octave[i]-p->octave[i-1]));
      }else{
	curmask=0;
      }
    }
  }
}

/* stability doesn't matter */
static int comp(const void *a,const void *b){
  if(fabs(**(double **)a)<fabs(**(double **)b))
    return(1);
  else
    return(-1);
}

/* this applies the floor and (optionally) tries to preserve noise
   energy in low resolution portions of the spectrum */
/* f and flr are *linear* scale, not dB */
void _vp_apply_floor(vorbis_look_psy *p,double *f, 
		      double *flr,double *mask){
  double *work=alloca(p->n*sizeof(double));
  double thresh=fromdB(p->vi->noisefit_threshdB);
  int i,j,addcount=0;
  thresh*=thresh;

  /* subtract the floor */
  for(j=0;j<p->n;j++){
    if(flr[j]<=0)
      work[j]=0.;
    else{
      if(fabs(f[j])<mask[j] || fabs(f[j])<flr[j]){
 	work[j]=0;
      }else{
	double val=f[j]/flr[j];
	work[j]=val;
      }
    }
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

	  /* yes, I mean *mask* here, not floor.  This should only be
             being applied in areas where noise dominates masking;
             otherwise we don't want to be adding tones back. */
	  double val=mask[p]*mask[p]+current_SL;
	  
	  if(val<original_SL){
	    addcount++;
	    if(f[p]>0)
	      work[p]=mask[p]/flr[p];
	    else
	      work[p]=-mask[p]/flr[p];
	    current_SL=val;
	  }else
	    break;
	}
      }
    }
  }
  memcpy(f,work,p->n*sizeof(double));
}

void _vp_tone_tone_mask(vorbis_look_psy *p,double *f, 
			double *flr, double *mask,
			double *decay){
  double *iter=alloca(sizeof(double)*p->n);
  int i,j,n=p->n;

  for(i=0;i<n;i++)iter[i]=fabs(f[i]);  

  /* perform iterative tone-tone masking */

  for(i=0;i<p->vi->curve_fit_iterations;i++){
    if(i==0)
      _vp_tone_tone_iter(p,iter,flr,mask,decay);
    else{
      _vp_tone_tone_iter(p,iter,iter,mask,decay);
      for(j=0;j<n;j++)
	flr[j]=(flr[j]+iter[j])/2.;
    }
    if(i!=p->vi->curve_fit_iterations-1){
      for(j=0;j<n;j++)
	if(fabs(f[j])<mask[j] && fabs(f[j])<flr[j])
	  iter[j]=0.;
	else
	  iter[j]=fabs(f[j]);
    }
  }
}

