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
  last modification date: Aug 02 1999

 ********************************************************************/

/* Some of these routines (autocorrelator, LPC coefficient estimator)
   are directly derived from and/or modified from code written by
   Jutta Degener and Carsten Bormann; thus we include their copyright
   below.  The entirety of this file is freely redistributable on the
   condition that both of these copyright notices are preserved
   without modification.  */

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
#include <string.h>
#include <math.h>
#include "smallft.h"
#include "lpc.h"

/* This is pared down for Vorbis, where we only use LPC to encode
   spectral envelope curves.  Thus we only are interested in
   generating the coefficients and recovering the curve from the
   coefficients.  Autocorrelation LPC coeff generation algorithm
   invented by N. Levinson in 1947, modified by J. Durbin in 1959. */

/*  Input : n element envelope curve
    Output: m lpc coefficients, excitation energy */

double vorbis_curve_to_lpc(double *curve,int n,double *lpc,int m){
  double aut[m+1],work[n+n],error;
  int i,j;

  /* input is a real curve. make it complex-real */
  for(i=0;i<n;i++){
    work[i*2]=curve[i];
    work[i*2+1]=0;
  }

  n*=2;
  fft_backward(n,work,NULL,NULL);

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
   results from direct calculation are cleaner and faster, however */

static double vorbis_lpc_magnitude(double w,double *lpc, int m){
  int k;
  double real=1,imag=0;
  for(k=0;k<m;k++){
    real+=lpc[k]*cos((k+1)*w);
    imag+=lpc[k]*sin((k+1)*w);
  }  
  return(1/sqrt(real*real+imag*imag));
}

/* generate the whole freq response curve on an LPC IIR filter */

void vorbis_lpc_to_curve(double *curve,int n,double *lpc, double amp,int m){
  int i;
  double w=1./n*M_PI;
  for(i=0;i<n;i++)
    curve[i]=vorbis_lpc_magnitude(i*w,lpc,m)*amp;
}

/* find frequency response of LPC filter only at nonsero residue
   points and apply the envelope to the residue */

void vorbis_lpc_apply(double *residue,int n,double *lpc, double amp,int m){
  int i;
  double w=1./n*M_PI;
  for(i=0;i<n;i++)
    if(residue[i])
      residue[i]*=vorbis_lpc_magnitude(i*w,lpc,m)*amp;
}


