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
 last modification date: Aug 26 1999

 ********************************************************************/

#include <math.h>
#include <string.h>
#include "stdio.h"
#include "codec.h"
#include "psy.h"
#include "lpc.h"
#include "smallft.h"
#include "xlogmap.h"

#define NOISEdB -6

#define MASKdB  20
#define HROLL   60
#define LROLL   90
#define MASKBIAS  10

#define LNOISE  .95
#define HNOISE  1.01
#define NOISEBIAS  20

/* Find the mean log energy of a given 'band'; used to evaluate tones
   against background noise */

/* This is faster than a real convolution, gives us roughly the log f
   scale we seek, and gives OK results.  So, that means it's a good
   hack */

/* To add: f scale noise attenuation curve */

void _vp_noise_floor(double *f, double *m,int n){
  long lo=0,hi=0;
  double acc=0,div=0;
  int i,j;

  for(i=100;i<n;i++){
    long newlo=i*LNOISE-NOISEBIAS;
    long newhi=i*HNOISE+NOISEBIAS;
    double temp;
    
    if(newhi>n)newhi=n;
    if(newlo<0)newlo=0;

    for(j=hi;j<newhi;j++){
      acc+=todB(f[j]);
      div++;
    }
    for(j=lo;j<newlo;j++){
      acc-=todB(f[j]);
      div--;
    }

    hi=newhi;
    lo=newlo;

    temp=fromdB(acc/div+NOISEdB); /* The NOISEdB constant should be an
				     attenuation curve */
    if(m[i]<temp)m[i]=temp;
  }
}

/* figure the masking curve.  linear rolloff on a dB scale, adjusted
   by octave */
void _vp_mask_floor(double *f, double *m,int n){
  double ocSCALE=1./log(2);
  double curmask=-9.e40;
  double curoc=log(MASKBIAS)*ocSCALE;
  long i;

  /* run mask forward then backward */
  for(i=0;i<n;i++){
    double newmask=todB(f[i])-MASKdB;
    double newoc=log(i+MASKBIAS)*ocSCALE;
    double roll=curmask-(newoc-curoc)*HROLL;
    double lroll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    lroll=fromdB(roll);
    if(m[i]<lroll)m[i]=lroll;
  }

  curmask=-9.e40;
  curoc=log(n+MASKBIAS)*ocSCALE;
  for(i=n-1;i>=0;i--){
    double newmask=todB(f[i])-MASKdB;
    double newoc=log(i+MASKBIAS)*ocSCALE;
    double roll=curmask-(curoc-newoc)*LROLL;
    double lroll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    lroll=fromdB(roll);
    if(m[i]<lroll)m[i]=lroll;
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
