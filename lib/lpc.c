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

  function: LPC low level routines
  last mod: $Id: lpc.c,v 1.18 2000/02/23 09:24:28 xiphmont Exp $

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
#include <string.h>
#include <math.h>
#include "os.h"
#include "smallft.h"
#include "lpc.h"
#include "scales.h"
#include "misc.h"

/* Autocorrelation LPC coeff generation algorithm invented by
   N. Levinson in 1947, modified by J. Durbin in 1959. */

/* Input : n elements of time doamin data
   Output: m lpc coefficients, excitation energy */

double vorbis_lpc_from_data(double *data,double *lpc,int n,int m){
  double *aut=alloca(sizeof(double)*(m+1));
  double error;
  int i,j;

  /* autocorrelation, p+1 lag coefficients */

  j=m+1;
  while(j--){
    double d=0;
    for(i=j;i<n;i++)d+=data[i]*data[i-j];
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

/* Input : n element envelope spectral curve
   Output: m lpc coefficients, excitation energy */

double vorbis_lpc_from_spectrum(double *curve,double *lpc,lpc_lookup *l){
  int n=l->ln;
  int m=l->m;
  double *work=alloca(sizeof(double)*(n+n));
  double fscale=.5/n;
  int i,j;
  
  /* input is a real curve. make it complex-real */
  /* This mixes phase, but the LPC generation doesn't care. */
  for(i=0;i<n;i++){
    work[i*2]=curve[i]*fscale;
    work[i*2+1]=0;
  }
  
  n*=2;
  drft_backward(&l->fft,work);
  
  /* The autocorrelation will not be circular.  Shift, else we lose
     most of the power in the edges. */
  
  for(i=0,j=n/2;i<n/2;){
    double temp=work[i];
    work[i++]=work[j];
    work[j++]=temp;
  }
  
  return(vorbis_lpc_from_data(work,lpc,n,m));
}

/* initialize Bark scale and normalization lookups.  We could do this
   with static tables, but Vorbis allows a number of possible
   combinations, so it's best to do it computationally.

   The below is authoritative in terms of defining scale mapping.
   Note that the scale depends on the sampling rate as well as the
   linear block and mapping sizes */

void lpc_init(lpc_lookup *l,int n, long mapped, long rate, int m){
  int i;
  double scale;
  memset(l,0,sizeof(lpc_lookup));

  l->n=n;
  l->ln=mapped;
  l->m=m;

  l->linearmap=malloc(n*sizeof(int));
  l->barknorm=malloc(mapped*sizeof(double));

  /* we choose a scaling constant so that:
     floor(bark(rate/2-1)*C)=mapped-1
     floor(bark(rate/2)*C)=mapped */

  scale=mapped/toBARK(rate/2.);

  /* the mapping from a linear scale to a smaller bark scale is
     straightforward.  We do *not* make sure that the linear mapping
     does not skip bark-scale bins; the decoder simply skips them and
     the encoder may do what it wishes in filling them.  They're
     necessary in some mapping combinations to keep the scale spacing
     accurate */
  {
    int last=-1;
    for(i=0;i<n;i++){
      int val=floor( toBARK((rate/2.)/n*i) *scale); /* bark numbers
							    represent
							    band edges */
      if(val>=mapped)val=mapped; /* guard against the approximation */
      l->linearmap[i]=val;
      last=val;
    }
  }

  /* 'Normalization' is just making sure that power isn't lost in the
     log scale by virtue of compressing the scale in higher
     frequencies.  We figure the weight of bands in proportion to
     their linear/bark width ratio below, again, authoritatively.  We
     use computed width (not the number of actual bins above) for
     smoothness in the scale; they should agree closely */

  /* keep it 0. to 1., else the dynamic range starts spreading through
     all the squaring... */

  for(i=0;i<mapped;i++)
    l->barknorm[i]=(fromBARK((i+1)/scale)-fromBARK(i/scale));
  for(i=0;i<mapped;i++)
    l->barknorm[i]/=l->barknorm[mapped-1];

  /* we cheat decoding the LPC spectrum via FFTs */
  
  drft_init(&l->fft,mapped*2);

}

void lpc_clear(lpc_lookup *l){
  if(l){
    if(l->barknorm)free(l->barknorm);
    if(l->linearmap)free(l->linearmap);
    drft_clear(&l->fft);
  }
}


/* less efficient than the decode side (written for clarity).  We're
   not bottlenecked here anyway */
static int frameno=-1;

double vorbis_curve_to_lpc(double *curve,double *lpc,lpc_lookup *l){
  /* map the input curve to a bark-scale curve for encoding */
  
  int mapped=l->ln;
  double *work=alloca(sizeof(double)*mapped);
  int i,j,last=0;

  frameno++;
  _analysis_output("lpc_pre",frameno,curve,l->n);

  memset(work,0,sizeof(double)*mapped);

  /* Only the decode side is behavior-specced; for now in the encoder,
     we select the maximum value of each band as representative (this
     helps make sure peaks don't go out of range.  In error terms,
     selecting min would make more sense, but the codebook is trained
     numerically, so we don't actually lose.  We'd still want to
     use the original curve for error and noise estimation */

  for(i=0;i<l->n;i++){
    int bark=l->linearmap[i];
    if(work[bark]<curve[i])work[bark]=curve[i];
    if(bark>last+1){
      /* If the bark scale is climbing rapidly, some bins may end up
         going unused.  This isn't a waste actually; it keeps the
         scale resolution even so that the LPC generator has an easy
         time.  However, if we leave the bins empty we lose energy.
         So, fill 'em in.  The decoder does not do anything with  he
         unused bins, so we can fill them anyway we like to end up
         with a better spectral curve */

      /* we'll always have a bin zero, so we don't need to guard init */
      long span=bark-last;
      for(j=1;j<span;j++){
	double del=(double)j/span;
	work[j+last]=work[bark]*del+work[last]*(1.-del);
      }
    }
    last=bark;
  }
  _analysis_output("lpc_prelog",frameno,work,l->ln);
  for(i=0;i<mapped;i++)work[i]*=l->barknorm[i];
  _analysis_output("lpc_prelognorm",frameno,work,l->ln);

  return vorbis_lpc_from_spectrum(work,lpc,l);
}


/* One can do this the long way by generating the transfer function in
   the time domain and taking the forward FFT of the result.  The
   results from direct calculation are cleaner and faster. 

   This version does a linear curve generation and then later
   interpolates the log curve from the linear curve.  */

void _vlpc_de_helper(double *curve,double *lpc,double amp,
			    lpc_lookup *l){
  int i;
  memset(curve,0,sizeof(double)*l->ln*2);
  if(amp==0)return;

  for(i=0;i<l->m;i++){
    curve[i*2+1]=lpc[i]/(4*amp);
    curve[i*2+2]=-lpc[i]/(4*amp);
  }

  drft_backward(&l->fft,curve); /* reappropriated ;-) */

  {
    int l2=l->ln*2;
    double unit=1./amp;
    curve[0]=(1./(curve[0]*2+unit));
    for(i=1;i<l->ln;i++){
      double real=(curve[i]+curve[l2-i]);
      double imag=(curve[i]-curve[l2-i]);
      curve[i]=(1./hypot(real+unit,imag));
    }
  }
}  

/* generate the whole freq response curve of an LPC IIR filter */

void vorbis_lpc_to_curve(double *curve,double *lpc,double amp,lpc_lookup *l){
  double *lcurve=alloca(sizeof(double)*(l->ln*2));
  int i;

  if(amp==0){
    memset(curve,0,sizeof(double)*l->n);
    return;
  }
  _vlpc_de_helper(lcurve,lpc,amp,l);
  _analysis_output("lpc_lognorm",frameno,lcurve,l->ln);

  for(i=0;i<l->ln;i++)lcurve[i]/=l->barknorm[i];
  _analysis_output("lpc_log",frameno,lcurve,l->ln);
  for(i=0;i<l->n;i++)curve[i]=lcurve[l->linearmap[i]];
  _analysis_output("lpc",frameno,curve,l->n);

}

/* subtract or add an lpc filter to data.  Vorbis doesn't actually use this. */

void vorbis_lpc_residue(double *coeff,double *prime,int m,
			double *data,long n){

  /* in: coeff[0...m-1] LPC coefficients 
         prime[0...m-1] initial values 
         data[0...n-1] data samples 
    out: data[0...n-1] residuals from LPC prediction */

  long i,j;
  double *work=alloca(sizeof(double)*(m+n));
  double y;

  if(!prime)
    for(i=0;i<m;i++)
      work[i]=0;
  else
    for(i=0;i<m;i++)
      work[i]=prime[i];

  for(i=0;i<n;i++){
    y=0;
    for(j=0;j<m;j++)
      y-=work[i+j]*coeff[m-j-1];
    
    work[i+m]=data[i];
    data[i]-=y;
  }
}


void vorbis_lpc_predict(double *coeff,double *prime,int m,
                     double *data,long n){

  /* in: coeff[0...m-1] LPC coefficients 
         prime[0...m-1] initial values (allocated size of n+m-1)
         data[0...n-1] residuals from LPC prediction   
    out: data[0...n-1] data samples */

  long i,j,o,p;
  double y;
  double *work=alloca(sizeof(double)*(m+n));

  if(!prime)
    for(i=0;i<m;i++)
      work[i]=0.;
  else
    for(i=0;i<m;i++)
      work[i]=prime[i];

  for(i=0;i<n;i++){
    y=data[i];
    o=i;
    p=m;
    for(j=0;j<m;j++)
      y-=work[o++]*coeff[--p];
    
    data[i]=work[o]=y;
  }
}

