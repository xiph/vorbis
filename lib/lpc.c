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

  function: LPC low level routines
  author: Monty <monty@xiph.org>
  modifications by: Monty
  last modification date: Aug 22 1999

 ********************************************************************/

/* Some of these routines (autocorrelator, LPC coefficient estimator)
   are derived from code written by Jutta Degener and Carsten Bormann;
   thus we include their copyright below.  The entirety of this file
   is freely redistributable on the condition that both of these
   copyright notices are preserved without modification.  */

/* Preserved Copyright: *********************************************/

/* Copyright 1992, 1993, 1994 by Jutta Degener and Carsten Bormann,
Technische Universita"t Berlin

Any use of this software is permitted provided that this notice is not
removed and that neither the authors nor the Technische Universita"t
Berlin are deemed to have made any representations as to the
suitability of this software for any purpose nor are held responsible
for any defects of this software. THERE IS ABSOLUTELY NO WARRANTY FOR
THIS SOFTWARE.

As a matter of courtesy, the authors request to be informed about uses
this software has found, about bugs in this software, and about any
improvements that may be of general interest.

Berlin, 28.11.1994
Jutta Degener
Carsten Bormann

*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "smallft.h"
#include "lpc.h"
#include "xlogmap.h"

/* This is pared down for Vorbis where we only use LPC to encode
   spectral envelope curves.  Thus we only are interested in
   generating the coefficients and recovering the curve from the
   coefficients.  Autocorrelation LPC coeff generation algorithm
   invented by N. Levinson in 1947, modified by J. Durbin in 1959. */

/*  Input : n element envelope curve
    Output: m lpc coefficients, excitation energy */

double memcof(double *data, int n, int m, double *d){
  int k,j,i;
  double p=0.,wk1[n],wk2[n],wkm[m],xms;
  
  memset(wk1,0,sizeof(wk1));
  memset(wk2,0,sizeof(wk2));
  memset(wkm,0,sizeof(wkm));
    
  for (j=0;j<n;j++) p += data[j]*data[j];
  xms=p/n;

  wk1[0]=data[0];
  wk2[n-2]=data[n-1];

  for (j=2;j<=n-1;j++) {
    wk1[j-1]=data[j-1];
    wk2[j-2]=data[j-1];
  }

  for (k=1;k<=m;k++) {
    double num=0.,denom=0.;
    for (j=1;j<=(n-k);j++) {

      num += wk1[j-1]*wk2[j-1];
      denom += wk1[j-1]*wk1[j-1] + wk2[j-1]*wk2[j-1];

    }

    d[k-1]=2.0*num/denom;
    xms *= (1.0-d[k-1]*d[k-1]);

    for (i=1;i<=(k-1);i++)
      d[i-1]=wkm[i-1]-d[k-1]*wkm[k-i-1];

    if (k == m) return xms;
   
    for (i=1;i<=k;i++) wkm[i-1]=d[i-1];
    for (j=1;j<=(n-k-1);j++) {

      wk1[j-1] -= wkm[k-1]*wk2[j-1];
      wk2[j-1]=wk2[j]-wkm[k-1]*wk1[j];

    }
  }
}

static double vorbis_gen_lpc(double *curve,int n,double *lpc,int m){
  double aut[m+1],work[n+n],error;
  drft_lookup dl;
  int i,j;

  /* input is a real curve. make it complex-real */
  for(i=0;i<n;i++){
    work[i*2]=curve[i];
    work[i*2+1]=0;
  }

  n*=2;
  drft_init(&dl,n);
  drft_backward(&dl,work);
  drft_clear(&dl);

  /* The autocorrelation will not be circular.  Shift, else we lose
     most of the power in the edges. */
  
  for(i=0,j=n/2;i<n/2;){
    double temp=work[i];
    work[i++]=work[j];
    work[j++]=temp;
  }

  /* autocorrelation, p+1 lag coefficients */

  j=m+1;
  while(j--){
    double d=0;
    for(i=j;i<n;i++)d+=work[i]*work[i-j];
    aut[j]=d;
  }

  /* Generate lpc coefficients from autocorr values */

  error=aut[0];
  if(error==0){
    memset(lpc,0,m*sizeof(double));
    return 0;
  }
  
  for(i=0;i<m;i++){
    double r=-aut[i+1];

    /* Sum up this iteration's reflection coefficient; note that in
       Vorbis we don't save it.  If anyone wants to recycle this code
       and needs reflection coefficients, save the results of 'r' from
       each iteration. */

    for(j=0;j<i;j++)r-=lpc[j]*aut[i-j];
    r/=error; 

    /* Update LPC coefficients and total error */

    lpc[i]=r;
    for(j=0;j<i/2;j++){
      double tmp=lpc[j];
      lpc[j]+=r*lpc[i-1-j];
      lpc[i-1-j]+=r*tmp;
    }
    if(i%2)lpc[j]+=lpc[j]*r;
    
    error*=1.0-r*r;
  }

  /* we need the error value to know how big an impulse to hit the
     filter with later */

  return error;
}

/* One can do this the long way by generating the transfer function in
   the time domain and taking the forward FFT of the result.  The
   results from direct calculation are cleaner and faster. If one
   looks at the below in the context of the calling function, there's
   lots of redundant trig, but at least it's clear */

static double vorbis_lpc_magnitude(double w,double *lpc, int m){
  int k;
  double real=1,imag=0;
  double wn=w;
  for(k=0;k<m;k++){
    real+=lpc[k]*cos(wn);
    imag+=lpc[k]*sin(wn);
    wn+=w;
  }  
  return(1./sqrt(real*real+imag*imag));
}

/* On top of this basic LPC infrastructure we introduce two modifications:

   1) Filter generation is limited in the resolution of features it
   can represent (this is more obvious when the filter is looked at as
   a set of LSP coefficients).  Human perception of the audio spectrum
   is logarithmic not only in amplitude, but also frequency.  Because
   the high frequency features we'll need to encode will be broader
   than the low frequency features, filter generation will be
   dominated by higher frequencies (when most of the energy is in the
   lowest frequencies, and greatest perceived resolution is in the
   midrange).  To avoid this effect, Vorbis encodes the frequency
   spectrum with a biased log frequency scale. The intent is to
   roughly equalize the sizes of the octaves (see xlogmap.h)

   2) When we change the frequency scale, we also change the
   (apparent) relative energies of the bands; that is, on a log scale
   covering 5 octaves, the highest octave goes from being represented
   in half the bins, to only 1/32 of the bins.  If the amplitudes
   remain the same, we have divided the energy represented by the
   highest octave by 16 (as far as Levinson-Durbin is concerned).
   This will seriously skew filter generation, which bases calculation
   on the mean square error with respect to energy.  Thus, Vorbis
   normalizes the amplitudes of the log spectrum frequencies to keep
   the relative octave energies correct. */

/* n == size of vector to be used for filter, m == order of filter,
   oct == octaves in normalized scale, encode_p == encode (1) or
   decode (0) */

double lpc_init(lpc_lookup *l,int n, int m, int oct, int encode_p){
  double bias=LOG_BIAS(n,oct);
  double scale=(float)n/(float)oct; /* where n==mapped */    
  int i;

  l->n=n;
  l->m=m;
  l->escale=malloc(n*sizeof(double));
  l->dscale=malloc(n*sizeof(double));
  l->norm=malloc(n*sizeof(double));

  for(i=0;i<n;i++){
    /* how much 'real estate' in the log domain does the bin in the
       linear domain represent? */
    double logA=LOG_X(i-.5,bias);
    double logB=LOG_X(i+.5,bias);
    l->norm[i]=logB-logA;  /* this much */
  }

  /* the scale is encode/decode specific for algebraic simplicity */

  if(encode_p){
    /* encode */

    for(i=0;i<n;i++)
      l->escale[i]=LINEAR_X(i/scale,bias);
    
  }
  /* decode; encode may use this too */
  
  {
    double w=1./oct*M_PI;
    for(i=0;i<n;i++)
      l->dscale[i]=LOG_X(i,bias)*w;   
  }
}

void lpc_clear(lpc_lookup *l){
  if(l){
    if(l->escale)free(l->escale);
    free(l->dscale);
    free(l->norm);
  }
}


/* less efficient than the decode side (written for clarity).  We're
   not bottlenecked here anyway */

double vorbis_curve_to_lpc(double *curve,double *lpc,lpc_lookup *l){
  /* map the input curve to a log curve for encoding */

  /* for clarity, mapped and n are both represented although setting
     'em equal is a decent rule of thumb. */
  
  int n=l->n;
  int m=l->m;
  int mapped=n;
  double work[mapped];
  int i;

  /* fairly correct for low frequencies, naieve for high frequencies
     (suffers from undersampling) */

  for(i=0;i<mapped;i++){
    double lin=l->escale[i];
    int a=floor(lin);
    int b=ceil(lin);
    double del=lin-floor(lin);

    work[i]=(curve[a]/l->norm[a]*(1.-del)+
	     curve[b]/l->norm[b]*del);      

  }

  memcpy(curve,work,sizeof(work));

  return vorbis_gen_lpc(work,mapped,lpc,l->m);
}

/* generate the whole freq response curve on an LPC IIR filter */

void vorbis_lpc_to_curve(double *curve,double *lpc,double amp,lpc_lookup *l){
  int i;
  for(i=0;i<l->n;i++)
    curve[i]=vorbis_lpc_magnitude(l->dscale[i],lpc,l->m)*amp*l->norm[i];
}

/* find frequency response of LPC filter only at nonsero residue
   points and apply the envelope to the residue */

void vorbis_lpc_apply(double *residue,double *lpc,double amp,lpc_lookup *l){
  int i;
  for(i=0;i<l->n;i++)
    if(residue[i])
      residue[i]*=vorbis_lpc_magnitude(l->dscale[i],lpc,l->m)*amp*l->norm[i];
}


