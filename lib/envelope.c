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
 last mod: $Id: envelope.c,v 1.21.2.2 2000/08/31 09:00:00 xiphmont Exp $

 Preecho calculation.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vorbis/codec.h"

#include "os.h"
#include "scales.h"
#include "envelope.h"
#include "bitwise.h"
#include "misc.h"

/* We use a Chebyshev bandbass for the preecho trigger bandpass; it's
   close enough for sample rates 32000-48000 Hz (corner frequencies at
   6k/14k assuming sample rate of 44.1kHz) */

/* Digital filter designed by mkfilter/mkshape/gencode A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Ch \
   -6.0000000000e+00 -Bp -o 5 -a 1.3605442177e-01 3.1746031746e-01 -l */

#if 0
static int    cheb_bandpass_stages=10;
static float cheb_bandpass_gain=5.589612458e+01;
static float cheb_bandpass_B[]={-1.,0.,5.,0.,-10.,0.,10.,0.,-5.,0.,1};
static float cheb_bandpass_A[]={
  -0.1917409386,
  0.0078657069,
  -0.7126903444,
  0.0266343467,
  -1.4047174730,
  0.0466964232,
  -1.9032773429,
  0.0451493360,
  -1.4471447397,
  0.0303413711};
#endif 

static int    cheb_highpass_stages=10;
static float cheb_highpass_gain= 5.291963434e+01;
/* z^-stage, z^-stage+1... */
static float cheb_highpass_B[]={1,-10,45,-120,210,-252,210,-120,45,-10,1};
static float cheb_highpass_A[]={
  -0.1247628029,
  0.1334086523,
  -0.3997715614,
  0.3213011089,
  -1.1131924119,
  1.7692446626,
  -3.6241199038,
  4.1950871291,
  -4.2771757867,
  2.3920318913};

void _ve_envelope_init(envelope_lookup *e,vorbis_info *vi){
  int ch=vi->channels;
  int window=vi->envelopesa;
  int i;
  e->winlength=window;
  e->minenergy=fromdB(vi->preecho_minenergy);
  e->iir=calloc(ch,sizeof(IIR_state));
  e->filtered=calloc(ch,sizeof(float *));
  e->ch=ch;
  e->storage=128;
  for(i=0;i<ch;i++){
    IIR_init(e->iir+i,cheb_highpass_stages,cheb_highpass_gain,
	     cheb_highpass_A,cheb_highpass_B);
    e->filtered[i]=calloc(e->storage,sizeof(float));
  }

  drft_init(&e->drft,window);
  e->window=malloc(e->winlength*sizeof(float));
  /* We just use a straight sin(x) window for this */
  for(i=0;i<e->winlength;i++)
    e->window[i]=sin((i+.5)/e->winlength*M_PI);
}

void _ve_envelope_clear(envelope_lookup *e){
  int i;
  for(i=0;i<e->ch;i++){
    IIR_clear((e->iir+i));
    free(e->filtered[i]);
  }
  drft_clear(&e->drft);
  free(e->window);
  free(e->filtered);
  memset(e,0,sizeof(envelope_lookup));
}

static float _ve_deltai(envelope_lookup *ve,IIR_state *iir,
		      float *pre,float *post){
  long n2=ve->winlength*2;
  long n=ve->winlength;

  float *workA=alloca(sizeof(float)*n2),A=0.;
  float *workB=alloca(sizeof(float)*n2),B=0.;
  long i;

  /*_analysis_output("A",frameno,pre,n,0,0);
    _analysis_output("B",frameno,post,n,0,0);*/

  for(i=0;i<n;i++){
    workA[i]=pre[i]*ve->window[i];
    workB[i]=post[i]*ve->window[i];
  }

  /*_analysis_output("Awin",frameno,workA,n,0,0);
    _analysis_output("Bwin",frameno,workB,n,0,0);*/

  drft_forward(&ve->drft,workA);
  drft_forward(&ve->drft,workB);

  /* we want to have a 'minimum bar' for energy, else we're just
     basing blocks on quantization noise that outweighs the signal
     itself (for low power signals) */
  {
    float min=ve->minenergy;
    for(i=0;i<n;i++){
      if(fabs(workA[i])<min)workA[i]=min;
      if(fabs(workB[i])<min)workB[i]=min;
    }
  }

  /*_analysis_output("Afft",frameno,workA,n,0,0);
    _analysis_output("Bfft",frameno,workB,n,0,0);*/

  for(i=0;i<n;i++){
    A+=workA[i]*workA[i];
    B+=workB[i]*workB[i];
  }

  A=todB(A);
  B=todB(B);

  return(B-A);
}

long _ve_envelope_search(vorbis_dsp_state *v,long searchpoint){
  vorbis_info *vi=v->vi;
  envelope_lookup *ve=v->ve;
  long i,j;
  
  /* make sure we have enough storage to match the PCM */
  if(v->pcm_storage>ve->storage){
    ve->storage=v->pcm_storage;
    for(i=0;i<ve->ch;i++)
      ve->filtered[i]=realloc(ve->filtered[i],ve->storage*sizeof(float));
  }

  /* catch up the highpass to match the pcm */
  for(i=0;i<ve->ch;i++){
    float *filtered=ve->filtered[i];
    float *pcm=v->pcm[i];
    IIR_state *iir=ve->iir+i;
    
    for(j=ve->current;j<v->pcm_current;j++)
      filtered[j]=IIR_filter(iir,pcm[j]);
  }
  ve->current=v->pcm_current;

  /* Now search through our cached highpass data for breaking points */
  /* starting point */
  if(v->W)
    j=v->centerW+vi->blocksizes[1]/4-vi->blocksizes[0]/4;
  else
    j=v->centerW;

  while(j+ve->winlength<=v->pcm_current){
    for(i=0;i<ve->ch;i++){
      float *filtered=ve->filtered[i]+j;
      IIR_state *iir=ve->iir+i;
      float m=_ve_deltai(ve,iir,filtered-ve->winlength,filtered);
      
      if(m>vi->preecho_thresh){
	/*frameno++;*/
	return(0);
      }
      /*frameno++;*/
    }
    
    j+=vi->blocksizes[0]/2;
    if(j>=searchpoint)return(1);
  }
  
  return(-1);
}

void _ve_envelope_shift(envelope_lookup *e,long shift){
  int i;
  for(i=0;i<e->ch;i++)
    memmove(e->filtered[i],e->filtered[i]+shift,(e->current-shift)*
	    sizeof(float));
  e->current-=shift;
}


