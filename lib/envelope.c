/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: PCM data envelope analysis 
 last mod: $Id: envelope.c,v 1.44 2002/03/24 21:04:00 xiphmont Exp $

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
#include "mdct.h"
#include "misc.h"

void _ve_envelope_init(envelope_lookup *e,vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  int ch=vi->channels;
  int i;
  int n=e->winlength=ci->blocksizes[0];
  e->searchstep=ci->blocksizes[0]/VE_DIV; /* not random */

  e->minenergy=gi->preecho_minenergy;
  e->ch=ch;
  e->storage=128;
  e->cursor=ci->blocksizes[1]/2;
  e->mdct_win=_ogg_calloc(n,sizeof(*e->mdct_win));
  mdct_init(&e->mdct,n);

  for(i=0;i<n;i++){
    e->mdct_win[i]=sin((i+.5)/n*M_PI);
    e->mdct_win[i]*=e->mdct_win[i];
  }

  /* overlapping bands, assuming 22050 (which is not always true, but
     to Hell with that) */
  /* 2(1.3-3) 4(2.6-6) 8(5.3-12) 16(10.6-18) */

  e->band[0].begin=rint(3000.f/22050.f*n/4.f)*2.f;
  e->band[0].end=rint(6000.f/22050.f*n/4.f)*2.f-e->band[0].begin;
  e->band[1].begin=rint(5000.f/22050.f*n/4.f)*2.f;
  e->band[1].end=rint(10000.f/22050.f*n/4.f)*2.f-e->band[1].begin;
  e->band[2].begin=rint(8000.f/22050.f*n/4.f)*2.f;
  e->band[2].end=rint(16000.f/22050.f*n/4.f)*2.f-e->band[2].begin;
  e->band[3].begin=rint(12000.f/22050.f*n/4.f)*2.f;
  e->band[3].end=rint(20000.f/22050.f*n/4.f)*2.f-e->band[3].begin;

  e->band[0].window=_ogg_malloc((e->band[0].end)/2*sizeof(*e->band[0].window));
  e->band[1].window=_ogg_malloc((e->band[1].end)/2*sizeof(*e->band[1].window));
  e->band[2].window=_ogg_malloc((e->band[2].end)/2*sizeof(*e->band[2].window));
  e->band[3].window=_ogg_malloc((e->band[3].end)/2*sizeof(*e->band[3].window));
  
  n=e->band[0].end/2;
  for(i=0;i<n;i++){
    e->band[0].window[i]=sin((i+.5)/n*M_PI);
    e->band[0].total+=e->band[0].window[i];
  }
  n=e->band[1].end/2;
  for(i=0;i<n;i++){
    e->band[1].window[i]=sin((i+.5)/n*M_PI);
    e->band[1].total+=e->band[1].window[i];
  }
  n=e->band[2].end/2;
  for(i=0;i<n;i++){
    e->band[2].window[i]=sin((i+.5)/n*M_PI);
    e->band[2].total+=e->band[2].window[i];
  }
  n=e->band[3].end/2;
  for(i=0;i<n;i++){
    e->band[3].window[i]=sin((i+.5)/n*M_PI);
    e->band[3].total+=e->band[3].window[i];
  }

  
  e->filter=_ogg_calloc(VE_BANDS*ch,sizeof(*e->filter));
  e->mark=_ogg_calloc(e->storage,sizeof(*e->mark));


}

void _ve_envelope_clear(envelope_lookup *e){
  int i;
  mdct_clear(&e->mdct);
  for(i=0;i<VE_BANDS;i++)
    _ogg_free(e->band[i].window);
  _ogg_free(e->mdct_win);
  _ogg_free(e->filter);
  _ogg_free(e->mark);
  memset(e,0,sizeof(*e));
}

/* fairly straight threshhold-by-band based until we find something
   that works better and isn't patented. */
static int _ve_amp(envelope_lookup *ve,
		   vorbis_info_psy_global *gi,
		   float *data,
		   envelope_band *bands,
		   envelope_filter_state *filters,
		   long pos){
  long n=ve->winlength;
  int ret=0;
  long i,j;

  /* we want to have a 'minimum bar' for energy, else we're just
     basing blocks on quantization noise that outweighs the signal
     itself (for low power signals) */

  float minV=ve->minenergy,acc[VE_BANDS];
  float *vec=alloca(n*sizeof(*vec));
  memset(acc,0,sizeof(acc));
 
 /* window and transform */
  for(i=0;i<n;i++)
    vec[i]=data[i]*ve->mdct_win[i];
  mdct_forward(&ve->mdct,vec,vec);

  /* accumulate amplitude by band */
  for(j=0;j<VE_BANDS;j++){
    for(i=0;i<bands[j].end;i+=2){
      float val=FABS(vec+i+bands[j].begin)+FABS(vec+i+bands[j].begin+1);
      acc[j]+=todB(&val)*bands[j].window[i>>1];
    }
    acc[j]/=bands[j].total;
    if(acc[j]<minV)acc[j]=minV;
    //fprintf(stderr,"%d %gdB :: ",j,acc[j]);
  }
  //fprintf(stderr,"\n");

  /* convert amplitude to delta */
  for(j=0;j<VE_BANDS;j++){
    float val=acc[j]-filters[j].ampbuf[filters[j].ampptr];
    filters[j].ampbuf[filters[j].ampptr]=acc[j];
    acc[j]=val;
    filters[j].ampptr++;
    if(filters[j].ampptr>=VE_DIV)filters[j].ampptr=0;
  }

  /* convolve deltas to threshhold values */
  for(j=0;j<VE_BANDS;j++){
    float *buf=filters[j].delbuf;
    float val=.14*buf[0]+.14*buf[1]+.72*acc[j];
    buf[0]=buf[1];buf[1]=acc[j];
    acc[j]=val;
    /*filters[j].markers[pos+1]=val;*/
  }

  /* look at local min/max */
  for(j=0;j<VE_BANDS;j++){
    float *buf=filters[j].convbuf;
    if(buf[1]>gi->preecho_thresh[j] && buf[0]<buf[1] && acc[j]<buf[1])ret|=1;
    if(buf[1]<gi->postecho_thresh[j] && buf[0]>buf[1] && acc[j]>buf[1])ret|=2;
    buf[0]=buf[1];buf[1]=acc[j];
  }
  return(ret);
}

long _ve_envelope_search(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  envelope_lookup *ve=((backend_lookup_state *)(v->backend_state))->ve;
  long i,j;

  int first=ve->current/ve->searchstep;
  int last=v->pcm_current/ve->searchstep-VE_DIV;
  if(first<0)first=0;

  /* make sure we have enough storage to match the PCM */
  if(last>ve->storage){
    ve->storage=last+VE_DIV;
    ve->mark=_ogg_realloc(ve->mark,ve->storage*sizeof(*ve->mark));
  }

  for(j=first;j<last;j++){
    int ret=0;
    for(i=0;i<ve->ch;i++){
      /* the mark delay is one searchstep because of min/max finder */
      float *pcm=v->pcm[i]+ve->searchstep*(j+1);
      ret|=_ve_amp(ve,gi,pcm,ve->band,ve->filter+i*VE_BANDS,j);
    }

    /* we assume a 'transient' occupies half a short block; this way,
       it's contained in two short blocks, else the first block is
       short and the second long, causing smearing.

      preecho triggers follow the impulse marker; postecho triger preceed it */

    ve->mark[j+VE_DIV/2]=0;
    if(ret&1)
      for(i=0;i<=VE_DIV/2;i++)
	ve->mark[j+i]=1;
    if(ret&2)
      for(i=-1;i>=-VE_DIV/2 && j+i>=0;i--)
	ve->mark[j+i]=1;
  }

  ve->current=last*ve->searchstep;

  {
    long centerW=v->centerW;
    long testW=
      centerW+
      ci->blocksizes[v->W]/4+
      ci->blocksizes[1]/2+
      ci->blocksizes[0]/4;
    
    j=ve->cursor;
    
    while(j<ve->current-(VE_DIV/2*ve->searchstep)){ /* modified to
                                                       stay clear of
                                                       possibly
                                                       unfinished
                                                       postecho
                                                       detection */
      if(j>=testW)return(1);
      if(ve->mark[j/ve->searchstep]){
	if(j>centerW){

#if 0
	  if(j>ve->curmark){
	    float *marker=alloca(v->pcm_current*sizeof(*marker));
	    int l;
	    memset(marker,0,sizeof(*marker)*v->pcm_current);
	    fprintf(stderr,"mark! seq=%d, cursor:%fs time:%fs\n",
		    seq,
		    (totalshift+ve->cursor)/44100.,
		    (totalshift+j)/44100.);
	    _analysis_output_always("pcmL",seq,v->pcm[0],v->pcm_current,0,0,totalshift);
	    _analysis_output_always("pcmR",seq,v->pcm[1],v->pcm_current,0,0,totalshift);

	    _analysis_output_always("markL",seq,v->pcm[0],j,0,0,totalshift);
	    _analysis_output_always("markR",seq,v->pcm[1],j,0,0,totalshift);
	    

	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[0].markers[l]*.01;
	    _analysis_output_always("delL0",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[1].markers[l]*.01;
	    _analysis_output_always("delL1",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[2].markers[l]*.01;
	    _analysis_output_always("delL2",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[3].markers[l]*.01;
	    _analysis_output_always("delL3",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[4].markers[l]*.01;
	    _analysis_output_always("delR0",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[5].markers[l]*.01;
	    _analysis_output_always("delR1",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[6].markers[l]*.01;
	    _analysis_output_always("delR2",seq,marker,v->pcm_current,0,0,totalshift);
	    for(l=0;l<last;l++)marker[l*ve->searchstep]=ve->filter[7].markers[l]*.01;
	    _analysis_output_always("delR3",seq,marker,v->pcm_current,0,0,totalshift);
	    seq++;

	  }
#endif      

	  ve->curmark=j;
	  ve->cursor=j;
	  if(j>=testW)return(1);
	  return(0);
	}
      }
      j+=ve->searchstep;
    }
  }
  ve->cursor=j;
 
  return(-1);
}

int _ve_envelope_mark(vorbis_dsp_state *v){
  envelope_lookup *ve=((backend_lookup_state *)(v->backend_state))->ve;
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  long centerW=v->centerW;
  long beginW=centerW-ci->blocksizes[v->W]/4;
  long endW=centerW+ci->blocksizes[v->W]/4;
  if(v->W){
    beginW-=ci->blocksizes[v->lW]/4;
    endW+=ci->blocksizes[v->nW]/4;
  }else{
    beginW-=ci->blocksizes[0]/4;
    endW+=ci->blocksizes[0]/4;
  }

  if(ve->curmark>=beginW && ve->curmark<endW)return(1);
  {
    long first=beginW/ve->searchstep;
    long last=endW/ve->searchstep;
    long i;
    for(i=first;i<last;i++)
      if(ve->mark[i])return(1);
  }
  return(0);
}

void _ve_envelope_shift(envelope_lookup *e,long shift){
  int smallsize=e->current/e->searchstep+VE_DIV/2; /* VE_DIV/2 is to
                                                      match setting a
                                                      mark on a region
                                                      in
                                                      envelope_search */
  int smallshift=shift/e->searchstep;
  int i;

  memmove(e->mark,e->mark+smallshift,(smallsize-smallshift)*sizeof(*e->mark));
  
#if 0
  for(i=0;i<VE_BANDS*e->ch;i++)
    memmove(e->filter[i].markers,
	    e->filter[i].markers+smallshift,
	    (1024-smallshift)*sizeof(*(*e->filter).markers));
  totalshift+=shift;
#endif

  e->current-=shift;
  if(e->curmark>=0)
    e->curmark-=shift;
  e->cursor-=shift;
}


