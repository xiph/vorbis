/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: random psychoacoustics (not including preecho)
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 15 1999

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "codec.h"
#include "psy.h"
#include "lpc.h"
#include "smallft.h"
#include "xlogmap.h"

/* Set up decibel threshhold 'curves'.  Actually, just set a level at
   log frequency intervals, interpolate, and call it a curve. */

static void set_curve(double *points,
		      double *ref,int rn,double rrate,
		      double *c,int n, double crate){
  int i,j=0;

  for(i=0;i<rn-1;i++){
    int endpos=points[i+1]*n*rrate/crate;
    double base=ref[i];
    double delta=(ref[i+1]-base)/(endpos-j);
    for(;j<endpos && j<n;j++){
      c[j]=base;
      base+=delta;
    }
  }
}

void _vp_psy_init(psy_lookup *p,vorbis_info *vi,int n){
  memset(p,0,sizeof(psy_lookup));
  p->noisethresh=malloc(n*sizeof(double));
  p->maskthresh=malloc(n*sizeof(double));
  p->vi=vi;
  p->n=n;

  /* set up the curves for a given blocksize and sample rate */
  
  set_curve(vi->threshhold_points,
	    vi->noisethresh,THRESH_POINTS,48000,p->noisethresh,n,vi->rate);
  set_curve(vi->threshhold_points,
	    vi->maskthresh, THRESH_POINTS,48000,p->maskthresh, n,vi->rate);

#ifdef ANALYSIS
  {
    int j;
    FILE *out;
    char buffer[80];
    
    sprintf(buffer,"noise_threshhold_%d.m",n);
    out=fopen(buffer,"w+");
    for(j=0;j<n;j++)
      fprintf(out,"%g\n",p->noisethresh[j]);
    fclose(out);
    sprintf(buffer,"mask_threshhold_%d.m",n);
    out=fopen(buffer,"w+");
    for(j=0;j<n;j++)
      fprintf(out,"%g\n",p->maskthresh[j]);
    fclose(out);
  }
#endif

}

void _vp_psy_clear(psy_lookup *p){
  if(p){
    if(p->noisethresh)free(p->noisethresh);
    if(p->maskthresh)free(p->maskthresh);
    memset(p,0,sizeof(psy_lookup));
  }
}

/* Find the mean log energy of a given 'band'; used to evaluate tones
   against background noise */

/* This is faster than a real convolution, gives us roughly the log f
   scale we seek, and gives OK results.  So, that means it's a good
   hack */

void _vp_noise_floor(psy_lookup *p, double *f, double *m){
  int n=p->n;
  vorbis_info *vi=p->vi;
  
  long lo=0,hi=0;
  double acc=0.;
  double div=0.;
  int i,j;
  
  double bias=n*vi->noisebias;

  for(i=0;i<n;i++){
    long newlo=i*vi->lnoise-bias;
    long newhi=i*vi->hnoise+bias;
    double temp;
    
    if(newhi>n)newhi=n;
    if(newlo<0)newlo=0;
    
    for(j=hi;j<newhi;j++){
      acc+=todB(f[j]);
      div+=1.;
    }
    for(j=lo;j<newlo;j++){
      acc-=todB(f[j]);
      div-=1.;
    }

    hi=newhi;
    lo=newlo;

    /* attenuate by the noise threshhold curve */
    temp=fromdB(acc/div+p->noisethresh[i]);
    if(m[i]<temp)m[i]=temp;
  }
}

/* Masking curve: linear rolloff on a dB scale, adjusted by octave,
   attenuated by maskthresh */

void _vp_mask_floor(psy_lookup *p,double *f, double *m){
  int n=p->n;
  double hroll=p->vi->hroll;
  double lroll=p->vi->lroll;
  double ocSCALE=1./log(2);
  double curmask=-9.e40;
  double maskbias=n*p->vi->maskbias;
  double curoc=log(maskbias)*ocSCALE;
  long i;

  /* run mask forward then backward */
  for(i=0;i<n;i++){
    double newmask=todB(f[i])+p->maskthresh[i];
    double newoc=log(i+maskbias)*ocSCALE;
    double roll=curmask-(newoc-curoc)*hroll;
    double troll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    troll=fromdB(roll);
    if(m[i]<troll)m[i]=troll;
  }

  curmask=-9.e40;
  curoc=log(n+maskbias)*ocSCALE;
  for(i=n-1;i>=0;i--){
    double newmask=todB(f[i])+p->maskthresh[i];
    double newoc=log(i+maskbias)*ocSCALE;
    double roll=curmask-(curoc-newoc)*lroll;
    double troll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    troll=fromdB(roll);
    if(m[i]<troll)m[i]=troll;
  }
}

/* s must be padded at the end with m-1 zeroes */
static void time_convolve(double *s,double *r,int n,int m){
  int i;
  
  for(i=0;i<n;i++){
    int j;
    double acc=0;

    for(j=0;j<m;j++)
      acc+=s[i+j]*r[m-j-1];

    s[i]=acc;
  }
}

/*************************************************************************/
/* Continuous balance analysis/synthesis *********************************/


/* Compute the average continual spectral balance of the given vectors
   (in radians); encode into LPC coefficients */

double _vp_balance_compute(double *A, double *B, double *lpc,lpc_lookup *vb){
  /* correlate in time (the response function is small).  Log
     frequency scale, small mapping */

  int n=vb->n;
  int mapped=vb->ln;

  /* 256/15 are arbitrary but unimportant to decoding */
  int resp=15;
  int i;

  /* This is encode side. Don't think too hard about it */
  
  double workA[mapped+resp];
  double workB[mapped+resp];
  double p[mapped];
  double workC[resp];

  memset(workA,0,sizeof(workA));
  memset(workB,0,sizeof(workB));
  memset(workC,0,sizeof(workC));

  for(i=0;i<n;i++){
    int j=vb->bscale[i]+(resp>>1);
    double mag_sq=A[i]*A[i]+B[i]*B[i];
    double phi;

    if(B[i]==0)
      phi=M_PI/2.;
    else{
      phi=atan(A[i]/B[i]);
      if((A[i]<0 && B[i]>0)||
	 (A[i]>0 && B[i]<0)){
	/* rotate II and IV into the first quadrant.  III is already there */
	phi+=M_PI/2;
      }
    }

    workA[j]+=mag_sq*sin(phi);
    workB[j]+=mag_sq*cos(phi);
  }

  /* prepare convolution vector.  Play with a few different shapes */
  
  for(i=0;i<resp;i++){
    workC[i]=sin(M_PI*(i+1)/(float)(resp+1));
    workC[i]*=workC[i];
  }

  time_convolve(workA,workC,mapped,resp);
  time_convolve(workB,workC,mapped,resp);

  {
    double amp1;

    for(i=0;i<mapped;i++){
      p[i]=atan2(workA[i],workB[i]);
    }

    amp1=sqrt(vorbis_gen_lpc(p,lpc,vb));

    return(amp1);
  }

}

/*void _vp_balance_apply(double *A, double *B, double *lpc, double amp,
		     lpc_lookup *vb,int divp){
  int i;
  for(i=0;i<vb->n;i++){
    double mag=sqrt(A[i]*A[i]+B[i]*B[i]);
    double del=vorbis_lpc_magnitude(vb->dscale[i],lpc,vb->m)*amp;
    double phi=atan2(A[i],B[i]);

    if(divp)
      phi-=del;
    else
      phi+=del;

    A[i]=mag*sin(phi);
    B[i]=mag*cos(phi);
  }
  }*/
