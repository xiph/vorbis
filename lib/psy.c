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
 last mod: $Id: psy.c,v 1.16.2.1 2000/03/29 03:49:28 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "vorbis/codec.h"

#include "masking.h"
#include "psy.h"
#include "scales.h"

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

static void interp_curve(double *c,double *c1,double *c2,double del){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]=c2[i]*del+c1[i]*(1.-del);
}

extern void analysis(char *base,int i,double *v,int n,int bark,int dB);

static double c0[EHMER_MAX]; /* initialized to zero */
static void setup_curve(double **c,
			int oc,
			double dB20,
			double dB40, 
			double dB60, 
			double dB80,
			double dB100,
			double dB120){
  int i,j;
  double tempc[11][EHMER_MAX];
  double ath[EHMER_MAX];

  for(i=0;i<EHMER_MAX;i++){
    double bark=toBARK(fromOC(oc*.5+(i-EHMER_OFFSET)*.125));
    int ibark=floor(bark);
    double del=bark-ibark;
    if(ibark<26)
      ath[i]=ATH_Bark_dB[ibark]*(1.-del)+ATH_Bark_dB[ibark+1]*del;
    else
      ath[i]=200;
    fprintf(stderr,"%d.%g ",ibark,del);
  }
  fprintf(stderr,"\n");
  analysis("Cath",oc*100,ath,EHMER_MAX,0,0);

  memcpy(c[10],c[8],sizeof(double)*EHMER_MAX);
  memcpy(c[0],c[2],sizeof(double)*EHMER_MAX);

  for(i=0;i<11;i+=2)
    analysis("Cpre",oc*100+i,c[i],EHMER_MAX,0,0);

  for(i=0;i<11;i++){
    analysis("Cpre",oc*100+i,c[i],EHMER_MAX,0,0);
    memcpy(tempc[i],c[i],sizeof(double)*EHMER_MAX);
    max_curve(tempc[i],ath);
  }

  attenuate_curve(c[0],dB20);
  attenuate_curve(c[2],dB40);
  attenuate_curve(c[4],dB60);
  attenuate_curve(c[6],dB80);
  attenuate_curve(c[8],dB100);
  attenuate_curve(c[10],dB120);
  attenuate_curve(tempc[0],dB20);
  attenuate_curve(tempc[2],dB40);
  attenuate_curve(tempc[4],dB60);
  attenuate_curve(tempc[6],dB80);
  attenuate_curve(tempc[8],dB100);
  attenuate_curve(tempc[10],dB120);

  /* The c array is comes in as dB curves at 20, 40, 60 80 100 120 dB.
     interpolate intermediate dB curves */
  interp_curve(c[1],c[0],c[2],.5);
  interp_curve(c[3],c[2],c[4],.5);
  interp_curve(c[5],c[4],c[6],.5);
  interp_curve(c[7],c[6],c[8],.5);
  interp_curve(c[9],c[8],c[10],.5);
  interp_curve(tempc[1],tempc[0],tempc[2],.5);
  interp_curve(tempc[3],tempc[2],tempc[4],.5);
  interp_curve(tempc[5],tempc[4],tempc[6],.5);
  interp_curve(tempc[7],tempc[6],tempc[8],.5);
  interp_curve(tempc[9],tempc[8],tempc[10],.5);

  /* take things out of dB domain into linear amplitude */
  for(i=0;i<11;i++)
    linear_curve(c[i]);
  for(i=0;i<11;i++)
    linear_curve(tempc[i]);
      
  /* Now limit the louder curves.

     the idea is this: We don't know what the playback attenuation
     will be; 0dB moves every time the user twiddles the volume
     knob. So that means we have to use a single 'most pessimal' curve
     for all masking amplitudes, right?  Wrong.  The *loudest* sound
     can be in (we assume) a range of 0-120dB SL.  However, sounds
     20dB down will be in a range of 0-100, 40dB down is from 0-80,
     etc... */
  for(i=10;i>=0;i--){
    analysis("Craw",oc*100+i,c[i],EHMER_MAX,0,1);
    analysis("Ctemp",oc*100+i,tempc[i],EHMER_MAX,0,1);
    for(j=0;j<i;j++)
      min_curve(c[i],tempc[j]);
    analysis("C",oc*100+i,c[i],EHMER_MAX,0,1);
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
  set_curve(vi->ath, p->ath,n,rate);

  for(i=0;i<n;i++){
    double oc=toOC((i+.5)*rate2/n);
    int pre=fromOC(oc-.0625)/rate2*n;
    int post=fromOC(oc+.0625)/rate2*n+1;
    p->pre[i]=(pre<0?0:pre);
    p->octave[i]=oc;
    p->post[i]=(post<n?post:n-1);
  }  

  p->curves=malloc(11*sizeof(double **));
  for(i=0;i<11;i++)
    p->curves[i]=malloc(11*sizeof(double *));

  for(i=0;i<11;i++)
    for(j=0;j<11;j++)
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

  for(i=1;i<11;i+=2)
    for(j=0;j<11;j+=2)
      interp_curve(p->curves[i][j],p->curves[i-1][j],p->curves[i+1][j],.5);
  
  for(i=0;i<11;i++)
    setup_curve(p->curves[i],i,-35.,-40.,-60.,-80.,-100.,-105.);

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
	for(j=0;j<11;j++)
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

static double seed_peaks(double *floor,int *len,double **curve,
		       double amp,double specmax,
		       int *pre,int *post,
		       int x,int n,int addp){
  int i;
  int ix=x*_eights[0];
  int prevx=(ix<0?-1:pre[ix]);
  int nextx;
  double ret=0.;

  /* make this attenuation adjustable */
  int choice=rint((amp-specmax+100.)/10.)-2;
  if(choice<0)choice=0;
  if(choice>8)choice=8;
  amp=fromdB(amp);

  for(i=0;i<EHMER_MAX;i++){ 
    ix=x*_eights[i];
    nextx=(ix<n?post[ix]:n-1);
    if(addp){
      double lin=curve[choice][i]*amp;
      /* Currently uses a n+n = +3dB additivity; 
	 6dB may be more correct in most cases */
      lin*=lin;
      
      floor[prevx]+=lin;
      floor[nextx]-=lin;
      if(i==EHMER_OFFSET || prevx==x)ret+=lin;
      if(nextx==x)ret-=lin;
    }else{
      if(floor[prevx]<curve[choice][i]*amp){
	floor[prevx]=curve[choice][i]*amp;
	len[prevx]=nextx;
      }
      if(len[prevx]<nextx+1)len[prevx]=nextx;
    }
    prevx=nextx;
  }
  return(ret);
}

static void add_seeds(double *floor,int n){
  int i;
  double acc=0.;
  for(i=0;i<n;i++){
    acc+=floor[i];
    floor[i]=(acc<=0?-DYNAMIC_RANGE_dB:todB(sqrt(acc)));
  }
}

static void arbitrate_peak(double *floor,int *len,int to,
			   double val,int end,int n){
  if(to<end){
    if(len[to]==to || val>floor[to]){
      /* new maximum */
      arbitrate_peak(floor,len,end,floor[to],len[to],n);
      floor[to]=val;
      len[to]=end;
    }else
      arbitrate_peak(floor,len,len[to],val,end,n);
  }
}

static void max_seeds(double *floor,int *len,int n){
  int i;
  double acc=0.;
  int end=0;
  for(i=0;i<n;i++){
    if(i==end){
      acc=0.;
      end=0;
    }
    if(floor[i]>acc){
      /* new maximum */
      arbitrate_peak(floor,len,len[i],acc,end,n);
      acc=floor[i];
      end=len[i];
    }else{
      /* nope, but see if we're maximum later */
      arbitrate_peak(floor,len,end,floor[i],len[i],n);
    }
    floor[i]=(acc>0?todB(acc):-DYNAMIC_RANGE_dB);
  }
}

/* octave/dB SL scale for masking curves, Bark/dB SPL scale for ATH.
   Why Bark scale for encoding but not masking? Because masking has a
   strong harmonic dependancy */

extern int frameno;
void _vp_tone_tone_mask(vorbis_look_psy *p,double *f, double *flr, 
			int athp, int addp, int decayp, double *decay){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  double *acctemp=alloca(n*sizeof(double));
  double *work=alloca(n*sizeof(double));
  double *workdB=alloca(n*sizeof(double));
  /*double *max=alloca(n*sizeof(double));
    int    *maxlen=alloca(n*sizeof(int));*/
  double specmax=-DYNAMIC_RANGE_dB;
  double acc=0.;
  /*memset(maxlen,0,n*sizeof(int));*/
  
  for(i=0;i<n;i++)work[i]=fabs(f[i]);
  
  /* slight smoothing; remember that 1 + 1 = sqrt(2) :-) 
     for(i=n-1;i>0;i--)work[i]=hypot(work[i],work[i-1]);*/

  /* handle decay */
  if(decayp){
    double decscale=1.-pow(vi->decay_coeff,n); 
    double attscale=1.-pow(vi->attack_coeff,n); 
    for(i=0;i<n;i++){
      double del=fabs(f[i])-decay[i];
      if(del>0)
	/* add energy */
	decay[i]+=del*attscale;
      else
	/* remove energy */
	decay[i]+=del*decscale;
      if(decay[i]>fabs(work[i]))work[i]=decay[i];
    }
  }

  for(i=0;i<n;i++){
    workdB[i]=todB(work[i]);
    if(workdB[i]>specmax)specmax=workdB[i];
  }

  /* subtract the absolute threshhold of hearing curve so the Ehmer
     curves can be used on the data directly */
  for(i=0;i<n;i++){
    /*work[i]-=p->ath[i]; clearly incorrect; ehmer's early data is not
      on an ATH relative scale */
    /*max[i]= -DYNAMIC_RANGE_dB;*/
  }
  memset(flr,0,sizeof(double)*n);

  /* prime the working vector with peak values */
  /* Use the 250 Hz curve up to 250 Hz and 8kHz curve after 8kHz. */
  for(i=0;i<n;i++){
    acc+=flr[i];
    if(work[i]*work[i]>acc){
      int o=rint(p->octave[i]*2.);
      if(o<0)o=0;
      if(o>10)o=10;
      acc+=seed_peaks(flr,NULL,p->curves[o],workdB[i],
		      specmax,p->pre,p->post,i,n,1);
      /*seed_peaks(max,maxlen,p->curves[o],workdB[i],
	specmax,p->pre,p->post,i,n,0);*/
    }
    acctemp[i]=acc;
  }

  /* now, chase curves down from the peak seeds */
  add_seeds(flr,n);
  /*max_seeds(max,maxlen,n);*/
  

  analysis("Pwork",frameno,workdB,n,1,0);
  analysis("Padd",frameno,flr,n,1,0);
  analysis("Pacc",frameno,acctemp,n,1,1);
  /*  analysis("Pmax",frameno,max,n,1,0);*/

  for(i=0;i<n;i++)if(flr[i]<p->ath[i]-vi->master_att)flr[i]=p->ath[i]-vi->master_att; 
  for(i=0;i<n;i++)flr[i]+=6.;

}

