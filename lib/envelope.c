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

 function: PCM data envelope analysis and manipulation
 last mod: $Id: envelope.c,v 1.19 2000/06/18 12:33:47 xiphmont Exp $

 Preecho calculation.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vorbis/codec.h"

#include "os.h"
#include "scales.h"
#include "smallft.h"
#include "envelope.h"
#include "bitwise.h"
#include "window.h"
#include "misc.h"

void _ve_envelope_init(envelope_lookup *e,int samples_per){
  int i;
  e->winlen=samples_per*2;
  e->window=malloc(e->winlen*sizeof(double));

  e->fft=calloc(1,sizeof(drft_lookup));
  drft_init(e->fft,samples_per*2);

  /* We just use a straight sin(x) window for this */
  for(i=0;i<e->winlen;i++)
    e->window[i]=sin((i+.5)/e->winlen*M_PI);
}

void _ve_envelope_clear(envelope_lookup *e){
  drft_clear(e->fft);
  free(e->fft);
  if(e->window)free(e->window);
  memset(e,0,sizeof(envelope_lookup));
}

static void smooth_noise(long n,double *f,double *noise){
  long i;
  long lo=0,hi=0;
  double acc=0.;

  for(i=0;i<n;i++){
    /* not exactly correct, (the center frequency should be centered
       on a *log* scale), but not worth quibbling */
    long newhi=i*1.0442718740+5;
    long newlo=i*.8781245150-5;
    if(newhi>n)newhi=n;

    for(;lo<newlo;lo++)
      acc-=todB(f[lo]); /* yeah, this ain't RMS */
    for(;hi<newhi;hi++)
      acc+=todB(f[hi]);
    noise[i]=acc/(hi-lo);
  }
}

/* use FFT for spectral power estimation */
static int frameno=0;
static int frameno2=0;

static void _ve_deltas(double *deltas,double *pcm,int n,double *window,
		       int samples_per,drft_lookup *l){
  int i,j;
  double *out=alloca(sizeof(double)*samples_per*2);
  double *cache=alloca(sizeof(double)*samples_per*2);
  
  for(j=-1;j<n;j++){

    memcpy(out,pcm+(j+1)*samples_per,samples_per*2*sizeof(double));
    for(i=0;i<samples_per*2;i++)
      out[i]*=window[i];

    _analysis_output("Dpcm",frameno*1000+frameno2,out,samples_per*2,0,0);

   
    drft_forward(l,out);
    for(i=1;i<samples_per;i++)
      out[i]=hypot(out[i*2],out[i*2-1]);
    _analysis_output("Dfft",frameno*1000+frameno2,out,samples_per,0,1);
    smooth_noise(samples_per,out,out+samples_per);

    if(j==-1){
      for(i=samples_per/10;i<samples_per;i++)
	cache[i]=out[i+samples_per];
    }else{
      double max=0;  
      _analysis_output("Dcache",frameno*1000+frameno2,cache,samples_per,0,0);
      for(i=samples_per/10;i<samples_per;i++){
	double val=out[i+samples_per]-cache[i];
	cache[i]=out[i+samples_per];
	if(val>0)max+=val;
      }
      max/=samples_per;
      if(deltas[j]<max)deltas[j]=max;
    }
    _analysis_output("Dnoise",frameno*1000+frameno2++,out+samples_per,samples_per,0,0);
  }
}

void _ve_envelope_deltas(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  int step=vi->envelopesa;
  
  int dtotal=v->pcm_current/vi->envelopesa-1;
  int dcurr=v->envelope_current;
  int pch;
  
  if(dtotal>dcurr){
    double *mult=v->multipliers+dcurr;
    memset(mult,0,sizeof(double)*(dtotal-dcurr));
      
    for(pch=0;pch<vi->channels;pch++){
      double *pcm=v->pcm[pch]+(dcurr-1)*step;
      _ve_deltas(mult,pcm,dtotal-dcurr,v->ve.window,step,v->ve.fft);

      {
	double *multexp=alloca(sizeof(double)*v->pcm_current);
	int i,j,k;

	memset(multexp,0,sizeof(double)*v->pcm_current);
	j=0;
	for(i=0;i<dtotal;i++)
	  for(k=0;k<step;k++)
	    multexp[j++]=v->multipliers[i];

	_analysis_output("Apcm",frameno,v->pcm[pch],v->pcm_current,0,0);
	_analysis_output("Amult",frameno++,multexp,v->pcm_current,0,0);
      }

    }
    v->envelope_current=dtotal;
  }
}






