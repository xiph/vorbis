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

 function: PCM data envelope analysis and manipulation
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Jun 17 1999

 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

static typedef struct {
  int divisor;
  double *window;
} envelope_lookup;

static double oPI  = 3.14159265358979323846;

envelope_lookup *init_envelope(int length,int divleng){
  envelope_lookup *ret=malloc(sizeof(envelope_lookup));
  int i;

  ret->length=divleng;
  ret->window=malloc(divleng*sizeof(double)*2);

  /* We just use a straight sin^2(x) window for this */
  for(i=0;i<divleng*2;i++){
    double temp=sin((i+.5)/divleng*oPI);
    ret->window[i]=temp*temp;
  }
}

/* right now, we do things simple and dirty.  Should this prove
   inadequate, then we'll think of something different.  The details
   of the encoding format do not depend on the exact behavior, only
   the format of the bits that come out.

   Using residual from an LPC whitening filter to judge envelope
   energy would probably yield cleaner results, but that's slow.
   Let's see if simple delta analysis gives us acceptible results.  */

int analyze_envelope0(double *vector, envelope_lookup *init, int n,
		       double *deltas){

  int divisor=init->length;
  int divs=n/divisor-1;
  double *win=init->window;
  int i,j,count=0;

  double max,spanlo,spanhi;
  
  /* initial and final blocks are special cases. Eg:
     ______________                  
                   \        
     |_______|______\|_______|_______|

                ___________             
               /           \
     |_______|/______|______\|_______|

                        _____________
                       /    
     |_______|_______|/______|_______|
 
     as we go block by block, we watch the collective metrics span. If we 
     span the threshhold (assuming the threshhold is active), we use an 
     abbreviated vector */
  
  /* initial frame */
  max=0;
  for(i=1;i<divisor;i++){
    double temp=abs(vector[i-1]-vector[i]);
    if(max<temp)max=temp;
  }
  for(;i<divisor*2;i++){
    double temp=abs(win[i-1]*vector[i-1]-win[i]*vector[i]);
    if(max<temp)max=temp;
  }
  spanlo=spanhi=deltas[count++]=max;

  /* mid frames */
  for(j=divisor;j<n-divisor*2;j+=divisor){
    max=0;
    for(i=1;i<divisor*2;i++){
      double temp=abs(win[i-1]*vector[j+i-1]-win[i]*vector[j+i]);
      if(max<temp)max=temp;
    }
    deltas[count++]=max;
    if(max<spanlo)spanlo=max;
    if(max>spanhi)spanhi=max;
    if(threshhold>1 && spanlo*threshhold<spanhi)
      abbrevflag=1;
    if(abbrevflag && j>n0-divisor/2)break;
  }

  /* last frame */
  if(!abbrevflag){
    max=0;
    for(i=1;i<divisor;i++){
      double temp=abs(win[i-1]*vector[j+i-1]-win[i]*vector[j+i]);
      if(max<temp)max=temp;
    }
    for(;i<divisor*2;i++){
      double temp=abs(vector[j+i-1]-vector[j+i]);
      if(max<temp)max=temp;
    }
    deltas[count++]=max;
    if(max<spanlo)spanlo=max;
    if(max>spanhi)spanhi=max;
    if(threshhold>1 && spanlo*threshhold<spanhi)
      abbrevflag=1;
  }

  if(abbrevflag)return(n0);
  return(n);
}  

/* also decide if we're going with a full sized or abbreviated
   vector. Some encoding tactics might want to use envelope massaging
   fully and discard abbreviated vectors entriely.  We make that
   decision here */

int analyze_envelope1(envelope_lookup *init,int n,
		      double triggerthresh,double spanthresh,
		      double *deltas){

  /* Look at the delta values; decide if we need to do any envelope
     manipulation at all on this vector; if so, choose the
     multipliers and placeholders.

     '0' is a placeholder.  Other values specify a
     multiplier/divisor. Multipliers are used by the decoder, divisors
     in the encoder.  The mapped m/d value for each segment is
     2^(n-1).  Placeholders (zeros) take on the value of the last
     non-zero multiplier/divisor.  When the placeholder is not
     preceeded by a non-placeholder value in the current vector, it
     assumes the value of the *next* non-zero value.  In this way, the
     vector manipulation is local to the current vector and does not
     rely on preceeding vectors.

  */
  
  /* scan forward with sliding windows; we start manipulating envelopes
     when the collective deltas span over a threshhold. If in fact we
     begin manipulating, we can manage on a finer scale than the
     original threshhold. first look for the larger threshhold and if
     we span it, manipulate the vector to hold within the smaller span
     threshhold. */

  /* scan for the trigger */

  int divisor=init->length;
  int divs=n/divisor-1;
  int i,triggerflag=0;
  double spanlo,spanhi;
  
  spanlo=spanhi=deltas[0];

  for(i=1;i<divs;i++){
    double max=deltas[i];
    if(max<spanlo)spanlo=max;
    if(max>spanhi)spanhi=max;
    if(spanlo*triggerthresh<spanhi){
      triggerflag=1;
      break;
    }
  }

  if(triggerflag){
    /* choose divisors/multipliers to fit the vector into the
       specified span.  In the decoder, these values are *multipliers*, so  */













  }
  return(triggerflag);
}

