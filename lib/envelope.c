/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: PCM data envelope analysis and manipulation
 last mod: $Id: envelope.c,v 1.28 2000/12/21 21:04:39 xiphmont Exp $

 Preecho calculation.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "os.h"
#include "scales.h"
#include "envelope.h"
#include "misc.h"

/* We use a Chebyshev bandbass for the preecho trigger bandpass; it's
   close enough for sample rates 32000-48000 Hz (corner frequencies at
   6k/14k assuming sample rate of 44.1kHz) */

/* Digital filter designed by mkfilter/mkshape/gencode A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Ch \
   -6.0000000000e+00 -Bp -o 5 -a 1.3605442177e-01 3.1746031746e-01 -l */

#if 0
static int    cheb_bandpass_stages=10;
static float cheb_bandpass_gain=5.589612458e+01f;
static float cheb_bandpass_B[]={-1.f,0.f,5.f,0.f,-10.f,0.f,
				10.f,0.f,-5.f,0.f,1f};
static float cheb_bandpass_A[]={
  -0.1917409386f,
  0.0078657069f,
  -0.7126903444f,
  0.0266343467f,
  -1.4047174730f,
  0.0466964232f,
  -1.9032773429f,
  0.0451493360f,
  -1.4471447397f,
  0.0303413711f};
#endif 

static int    cheb_highpass_stages=10;
static float cheb_highpass_gain= 5.291963434e+01f;
/* z^-stage, z^-stage+1... */
static float cheb_highpass_B[]={1.f,-10.f,45.f,-120.f,210.f,
				-252.f,210.f,-120.f,45.f,-10.f,1.f};
static float cheb_highpass_A[]={
  -0.1247628029f,
  0.1334086523f,
  -0.3997715614f,
  0.3213011089f,
  -1.1131924119f,
  1.7692446626f,
  -3.6241199038f,
  4.1950871291f,
  -4.2771757867f,
  2.3920318913f};

void _ve_envelope_init(envelope_lookup *e,vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  int ch=vi->channels;
  int window=ci->envelopesa;
  int i;
  e->winlength=window;
  e->minenergy=fromdB(ci->preecho_minenergy);
  e->iir=_ogg_calloc(ch,sizeof(IIR_state));
  e->filtered=_ogg_calloc(ch,sizeof(float *));
  e->ch=ch;
  e->storage=128;
  for(i=0;i<ch;i++){
    IIR_init(e->iir+i,cheb_highpass_stages,cheb_highpass_gain,
	     cheb_highpass_A,cheb_highpass_B);
    e->filtered[i]=_ogg_calloc(e->storage,sizeof(float));
  }

  drft_init(&e->drft,window);
  e->window=_ogg_malloc(e->winlength*sizeof(float));
  /* We just use a straight sin(x) window for this */
  for(i=0;i<e->winlength;i++)
    e->window[i]=sin((i+.5)/e->winlength*M_PI);
}

void _ve_envelope_clear(envelope_lookup *e){
  int i;
  for(i=0;i<e->ch;i++){
    IIR_clear((e->iir+i));
    _ogg_free(e->filtered[i]);
  }
  drft_clear(&e->drft);
  _ogg_free(e->window);
  _ogg_free(e->filtered);
  _ogg_free(e->iir);
  memset(e,0,sizeof(envelope_lookup));
}

static float _ve_deltai(envelope_lookup *ve,
		      float *pre,float *post){
  long n2=ve->winlength*2;
  long n=ve->winlength;

  float *workA=alloca(sizeof(float)*n2),A=0.f;
  float *workB=alloca(sizeof(float)*n2),B=0.f;
  long i;

  /*_analysis_output("A",granulepos,pre,n,0,0);
    _analysis_output("B",granulepos,post,n,0,0);*/

  for(i=0;i<n;i++){
    workA[i]=pre[i]*ve->window[i];
    workB[i]=post[i]*ve->window[i];
  }

  /*_analysis_output("Awin",granulepos,workA,n,0,0);
    _analysis_output("Bwin",granulepos,workB,n,0,0);*/

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

  /*_analysis_output("Afft",granulepos,workA,n,0,0);
    _analysis_output("Bfft",granulepos,workB,n,0,0);*/

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
  codec_setup_info *ci=vi->codec_setup;
  envelope_lookup *ve=((backend_lookup_state *)(v->backend_state))->ve;
  long i,j;
  
  /* make sure we have enough storage to match the PCM */
  if(v->pcm_storage>ve->storage){
    ve->storage=v->pcm_storage;
    for(i=0;i<ve->ch;i++)
      ve->filtered[i]=_ogg_realloc(ve->filtered[i],ve->storage*sizeof(float));
  }

  /* catch up the highpass to match the pcm */
  for(i=0;i<ve->ch;i++){
    float *filtered=ve->filtered[i];
    float *pcm=v->pcm[i];
    IIR_state *iir=ve->iir+i;
    int flag=1;
    
    for(j=ve->current;j<v->pcm_current;j++){
      filtered[j]=IIR_filter(iir,pcm[j]);
      if(pcm[j])flag=0;
    }
    if(flag && ve->current+64<v->pcm_current)IIR_reset(iir);
  }

  ve->current=v->pcm_current;

  /* Now search through our cached highpass data for breaking points */
  /* starting point */
  if(v->W)
    j=v->centerW+ci->blocksizes[1]/4-ci->blocksizes[0]/4;
  else
    j=v->centerW;

  while(j+ve->winlength<=v->pcm_current){
    for(i=0;i<ve->ch;i++){
      float *filtered=ve->filtered[i]+j;
      float m=_ve_deltai(ve,filtered-ve->winlength,filtered);
      
      if(m>ci->preecho_thresh){
	/*granulepos++;*/
	return(0);
      }
      /*granulepos++;*/
    }
    
    j+=ci->blocksizes[0]/2;
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


