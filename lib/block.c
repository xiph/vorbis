/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: PCM data vector blocking, windowing and dis/reassembly
 last mod: $Id: block.c,v 1.51 2001/12/12 09:45:24 xiphmont Exp $

 Handle windowing, overlap-add, etc of the PCM vectors.  This is made
 more amusing by Vorbis' current two allowed block sizes.
 
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "window.h"
#include "mdct.h"
#include "lpc.h"
#include "registry.h"
#include "misc.h"

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* pcm accumulator examples (not exhaustive):

 <-------------- lW ---------------->
                   <--------------- W ---------------->
:            .....|.....       _______________         |
:        .'''     |     '''_---      |       |\        |
:.....'''         |_____--- '''......|       | \_______|
:.................|__________________|_______|__|______|
                  |<------ Sl ------>|      > Sr <     |endW
                  |beginSl           |endSl  |  |endSr   
                  |beginW            |endlW  |beginSr


                      |< lW >|       
                   <--------------- W ---------------->
                  |   |  ..  ______________            |
                  |   | '  `/        |     ---_        |
                  |___.'___/`.       |         ---_____| 
                  |_______|__|_______|_________________|
                  |      >|Sl|<      |<------ Sr ----->|endW
                  |       |  |endSl  |beginSr          |endSr
                  |beginW |  |endlW                     
                  mult[0] |beginSl                     mult[n]

 <-------------- lW ----------------->
                          |<--W-->|                               
:            ..............  ___  |   |                    
:        .'''             |`/   \ |   |                       
:.....'''                 |/`....\|...|                    
:.........................|___|___|___|                  
                          |Sl |Sr |endW    
                          |   |   |endSr
                          |   |beginSr
                          |   |endSl
			  |beginSl
			  |beginW
*/

/* block abstraction setup *********************************************/

#ifndef WORD_ALIGN
#define WORD_ALIGN 8
#endif

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  memset(vb,0,sizeof(*vb));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;
  if(v->analysisp){
    vorbis_block_internal *vbi=
      vb->internal=_ogg_calloc(1,sizeof(vorbis_block_internal));
    oggpack_writeinit(&vb->opb);
    vbi->ampmax=-9999;
    vbi->packet_markers=_ogg_malloc(vorbis_bitrate_maxmarkers()*
			       sizeof(*vbi->packet_markers));
  }
  
  return(0);
}

void *_vorbis_block_alloc(vorbis_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    /* can't just _ogg_realloc... there are outstanding pointers */
    if(vb->localstore){
      struct alloc_chain *link=_ogg_malloc(sizeof(*link));
      vb->totaluse+=vb->localtop;
      link->next=vb->reap;
      link->ptr=vb->localstore;
      vb->reap=link;
    }
    /* highly conservative */
    vb->localalloc=bytes;
    vb->localstore=_ogg_malloc(vb->localalloc);
    vb->localtop=0;
  }
  {
    void *ret=(void *)(((char *)vb->localstore)+vb->localtop);
    vb->localtop+=bytes;
    return ret;
  }
}

/* reap the chain, pull the ripcord */
void _vorbis_block_ripcord(vorbis_block *vb){
  /* reap the chain */
  struct alloc_chain *reap=vb->reap;
  while(reap){
    struct alloc_chain *next=reap->next;
    _ogg_free(reap->ptr);
    memset(reap,0,sizeof(*reap));
    _ogg_free(reap);
    reap=next;
  }
  /* consolidate storage */
  if(vb->totaluse){
    vb->localstore=_ogg_realloc(vb->localstore,vb->totaluse+vb->localalloc);
    vb->localalloc+=vb->totaluse;
    vb->totaluse=0;
  }

  /* pull the ripcord */
  vb->localtop=0;
  vb->reap=NULL;
}

int vorbis_block_clear(vorbis_block *vb){
  if(vb->vd)
    if(vb->vd->analysisp)
      oggpack_writeclear(&vb->opb);
  _vorbis_block_ripcord(vb);
  if(vb->localstore)_ogg_free(vb->localstore);

  if(vb->internal){
    vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;
    if(vbi->packet_markers)_ogg_free(vbi->packet_markers);

    _ogg_free(vb->internal);
  }

  memset(vb,0,sizeof(*vb));
  return(0);
}

/* Analysis side code, but directly related to blocking.  Thus it's
   here and not in analysis.c (which is for analysis transforms only).
   The init is here because some of it is shared */

static int _vds_shared_init(vorbis_dsp_state *v,vorbis_info *vi,int encp){
  int i;
  codec_setup_info *ci=vi->codec_setup;
  backend_lookup_state *b=NULL;

  memset(v,0,sizeof(*v));
  b=v->backend_state=_ogg_calloc(1,sizeof(*b));

  v->vi=vi;
  b->modebits=ilog2(ci->modes);

  b->transform[0]=_ogg_calloc(VI_TRANSFORMB,sizeof(*b->transform[0]));
  b->transform[1]=_ogg_calloc(VI_TRANSFORMB,sizeof(*b->transform[1]));

  /* MDCT is tranform 0 */

  b->transform[0][0]=_ogg_calloc(1,sizeof(mdct_lookup));
  b->transform[1][0]=_ogg_calloc(1,sizeof(mdct_lookup));
  mdct_init(b->transform[0][0],ci->blocksizes[0]);
  mdct_init(b->transform[1][0],ci->blocksizes[1]);

  b->window[0][0][0]=_ogg_calloc(VI_WINDOWB,sizeof(*b->window[0][0][0]));
  b->window[0][0][1]=b->window[0][0][0];
  b->window[0][1][0]=b->window[0][0][0];
  b->window[0][1][1]=b->window[0][0][0];
  b->window[1][0][0]=_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][0][0]));
  b->window[1][0][1]=_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][0][1]));
  b->window[1][1][0]=_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][1][0]));
  b->window[1][1][1]=_ogg_calloc(VI_WINDOWB,sizeof(*b->window[1][1][1]));

  for(i=0;i<VI_WINDOWB;i++){
    b->window[0][0][0][i]=
      _vorbis_window(i,ci->blocksizes[0],ci->blocksizes[0]/2,ci->blocksizes[0]/2);
    b->window[1][0][0][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[0]/2,ci->blocksizes[0]/2);
    b->window[1][0][1][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[0]/2,ci->blocksizes[1]/2);
    b->window[1][1][0][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[1]/2,ci->blocksizes[0]/2);
    b->window[1][1][1][i]=
      _vorbis_window(i,ci->blocksizes[1],ci->blocksizes[1]/2,ci->blocksizes[1]/2);
  }

  if(encp){ /* encode/decode differ here */
    /* finish the codebooks */
    b->fullbooks=_ogg_calloc(ci->books,sizeof(*b->fullbooks));
    for(i=0;i<ci->books;i++)
      vorbis_book_init_encode(b->fullbooks+i,ci->book_param[i]);
    v->analysisp=1;
  }else{
    /* finish the codebooks */
    b->fullbooks=_ogg_calloc(ci->books,sizeof(*b->fullbooks));
    for(i=0;i<ci->books;i++)
      vorbis_book_init_decode(b->fullbooks+i,ci->book_param[i]);
  }

  /* initialize the storage vectors to a decent size greater than the
     minimum */
  
  v->pcm_storage=8192; /* we'll assume later that we have
			  a minimum of twice the blocksize of
			  accumulated samples in analysis */
  v->pcm=_ogg_malloc(vi->channels*sizeof(*v->pcm));
  v->pcmret=_ogg_malloc(vi->channels*sizeof(*v->pcmret));
  {
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=_ogg_calloc(v->pcm_storage,sizeof(*v->pcm[i]));
  }

  /* all 1 (large block) or 0 (small block) */
  /* explicitly set for the sake of clarity */
  v->lW=0; /* previous window size */
  v->W=0;  /* current window size */

  /* all vector indexes */
  v->centerW=ci->blocksizes[1]/2;

  v->pcm_current=v->centerW;

  /* initialize all the mapping/backend lookups */
  b->mode=_ogg_calloc(ci->modes,sizeof(*b->mode));
  for(i=0;i<ci->modes;i++){
    int mapnum=ci->mode_param[i]->mapping;
    int maptype=ci->map_type[mapnum];
    b->mode[i]=_mapping_P[maptype]->look(v,ci->mode_param[i],
					 ci->map_param[mapnum]);
  }

  return(0);
}

/* arbitrary settings and spec-mandated numbers get filled in here */
int vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi){
  backend_lookup_state *b=NULL;

  _vds_shared_init(v,vi,1);
  b=v->backend_state;
  b->psy_g_look=_vp_global_look(vi);

  /* Initialize the envelope state storage */
  b->ve=_ogg_calloc(1,sizeof(*b->ve));
  _ve_envelope_init(b->ve,vi);

  vorbis_bitrate_init(vi,&b->bms);

  return(0);
}

void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i,j,k;
  if(v){
    vorbis_info *vi=v->vi;
    codec_setup_info *ci=(vi?vi->codec_setup:NULL);
    backend_lookup_state *b=v->backend_state;

    if(b){
      if(b->window[0][0][0]){
	for(i=0;i<VI_WINDOWB;i++)
	  if(b->window[0][0][0][i])_ogg_free(b->window[0][0][0][i]);
	_ogg_free(b->window[0][0][0]);
	
	for(j=0;j<2;j++)
	  for(k=0;k<2;k++){
	    for(i=0;i<VI_WINDOWB;i++)
	      if(b->window[1][j][k][i])_ogg_free(b->window[1][j][k][i]);
	    _ogg_free(b->window[1][j][k]);
	  }
      }

      if(b->ve){
	_ve_envelope_clear(b->ve);
	_ogg_free(b->ve);
      }

      if(b->transform[0]){
	mdct_clear(b->transform[0][0]);
	_ogg_free(b->transform[0][0]);
	_ogg_free(b->transform[0]);
      }
      if(b->transform[1]){
	mdct_clear(b->transform[1][0]);
	_ogg_free(b->transform[1][0]);
	_ogg_free(b->transform[1]);
      }
      if(b->psy_g_look)_vp_global_free(b->psy_g_look);
      vorbis_bitrate_clear(&b->bms);
    }
    
    if(v->pcm){
      for(i=0;i<vi->channels;i++)
	if(v->pcm[i])_ogg_free(v->pcm[i]);
      _ogg_free(v->pcm);
      if(v->pcmret)_ogg_free(v->pcmret);
    }

    /* free mode lookups; these are actually vorbis_look_mapping structs */
    if(ci){
      for(i=0;i<ci->modes;i++){
	int mapnum=ci->mode_param[i]->mapping;
	int maptype=ci->map_type[mapnum];
	if(b && b->mode)_mapping_P[maptype]->free_look(b->mode[i]);
      }
      /* free codebooks */
      for(i=0;i<ci->books;i++)
	if(b && b->fullbooks)vorbis_book_clear(b->fullbooks+i);
    }

    if(b){
      if(b->mode)_ogg_free(b->mode);    
      if(b->fullbooks)_ogg_free(b->fullbooks);
      
      /* free header, header1, header2 */
      if(b->header)_ogg_free(b->header);
      if(b->header1)_ogg_free(b->header1);
      if(b->header2)_ogg_free(b->header2);
      _ogg_free(b);
    }
    
    memset(v,0,sizeof(*v));
  }
}

float **vorbis_analysis_buffer(vorbis_dsp_state *v, int vals){
  int i;
  vorbis_info *vi=v->vi;
  backend_lookup_state *b=v->backend_state;

  /* free header, header1, header2 */
  if(b->header)_ogg_free(b->header);b->header=NULL;
  if(b->header1)_ogg_free(b->header1);b->header1=NULL;
  if(b->header2)_ogg_free(b->header2);b->header2=NULL;

  /* Do we have enough storage space for the requested buffer? If not,
     expand the PCM (and envelope) storage */
    
  if(v->pcm_current+vals>=v->pcm_storage){
    v->pcm_storage=v->pcm_current+vals*2;
   
    for(i=0;i<vi->channels;i++){
      v->pcm[i]=_ogg_realloc(v->pcm[i],v->pcm_storage*sizeof(*v->pcm[i]));
    }
  }

  for(i=0;i<vi->channels;i++)
    v->pcmret[i]=v->pcm[i]+v->pcm_current;
    
  return(v->pcmret);
}

static int seq=0;
static void _preextrapolate_helper(vorbis_dsp_state *v){
  int i;
  int order=32;
  float *lpc=alloca(order*sizeof(*lpc));
  float *work=alloca(v->pcm_current*sizeof(*work));
  long j;
  v->preextrapolate=1;

  if(v->pcm_current-v->centerW>order*2){ /* safety */
    for(i=0;i<v->vi->channels;i++){
      /* need to run the extrapolation in reverse! */
      for(j=0;j<v->pcm_current;j++)
	work[j]=v->pcm[i][v->pcm_current-j-1];
      
      _analysis_output("preextrap",seq,v->pcm[i],v->pcm_current,0,0);
      _analysis_output("workextrap",seq,work,v->pcm_current,0,0);

      /* prime as above */
      vorbis_lpc_from_data(work,lpc,v->pcm_current-v->centerW,order);
      _analysis_output("lpc",seq,lpc,order,0,0);
      
      /* run the predictor filter */
      vorbis_lpc_predict(lpc,work+v->pcm_current-v->centerW-order,
			 order,
			 work+v->pcm_current-v->centerW,
			 v->centerW);

      _analysis_output("extrap",seq,work,v->pcm_current,0,0);


      for(j=0;j<v->pcm_current;j++)
	v->pcm[i][v->pcm_current-j-1]=work[j];

      _analysis_output("postextrap",seq++,v->pcm[i],v->pcm_current,0,0);
    }
  }
}


/* call with val<=0 to set eof */

int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  /*backend_lookup_state *b=v->backend_state;*/

  if(vals<=0){
    int order=32;
    int i;
    float *lpc=alloca(order*sizeof(*lpc));

    /* if it wasn't done earlier (very short sample) */
    if(!v->preextrapolate)
      _preextrapolate_helper(v);

    /* We're encoding the end of the stream.  Just make sure we have
       [at least] a full block of zeroes at the end. */
    /* actually, we don't want zeroes; that could drop a large
       amplitude off a cliff, creating spread spectrum noise that will
       suck to encode.  Extrapolate for the sake of cleanliness. */

    vorbis_analysis_buffer(v,ci->blocksizes[1]*2);
    v->eofflag=v->pcm_current;
    v->pcm_current+=ci->blocksizes[1]*2;

    for(i=0;i<vi->channels;i++){
      if(v->eofflag>order*2){
	/* extrapolate with LPC to fill in */
	long n;

	/* make a predictor filter */
	n=v->eofflag;
	if(n>ci->blocksizes[1])n=ci->blocksizes[1];
	vorbis_lpc_from_data(v->pcm[i]+v->eofflag-n,lpc,n,order);

	/* run the predictor filter */
	vorbis_lpc_predict(lpc,v->pcm[i]+v->eofflag-order,order,
			   v->pcm[i]+v->eofflag,v->pcm_current-v->eofflag);
      }else{
	/* not enough data to extrapolate (unlikely to happen due to
           guarding the overlap, but bulletproof in case that
           assumtion goes away). zeroes will do. */
	memset(v->pcm[i]+v->eofflag,0,
	       (v->pcm_current-v->eofflag)*sizeof(*v->pcm[i]));

      }
    }
  }else{

    if(v->pcm_current+vals>v->pcm_storage)
      return(OV_EINVAL);

    v->pcm_current+=vals;

    /* we may want to reverse extrapolate the beginning of a stream
       too... in case we're beginning on a cliff! */
    /* clumsy, but simple.  It only runs once, so simple is good. */
    if(!v->preextrapolate && v->pcm_current-v->centerW>ci->blocksizes[1])
      _preextrapolate_helper(v);

  }
  return(0);
}

/* do the deltas, envelope shaping, pre-echo and determine the size of
   the next block on which to continue analysis */
int vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb){
  int i;
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;
  backend_lookup_state *b=v->backend_state;
  vorbis_look_psy_global *g=b->psy_g_look;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  long beginW=v->centerW-ci->blocksizes[v->W]/2,centerNext;
  vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;

  /* check to see if we're started... */
  if(!v->preextrapolate)return(0);

  /* check to see if we're done... */
  if(v->eofflag==-1)return(0);

  /* By our invariant, we have lW, W and centerW set.  Search for
     the next boundary so we can determine nW (the next window size)
     which lets us compute the shape of the current block's window */
  
  if(ci->blocksizes[0]<ci->blocksizes[1]){
    long bp=_ve_envelope_search(v);
    if(bp==-1)return(0); /* not enough data currently to search for a
                            full long block */
    v->nW=bp;
    //v->nW=0;

  }else
    v->nW=0;
  
  centerNext=v->centerW+ci->blocksizes[v->W]/4+ci->blocksizes[v->nW]/4;

  {
    /* center of next block + next block maximum right side. */

    long blockbound=centerNext+ci->blocksizes[v->nW]/2;
    if(v->pcm_current<blockbound)return(0); /* not enough data yet;
                                               although this check is
                                               less strict that the
                                               _ve_envelope_search,
                                               the search is not run
                                               if we only use one
                                               block size */


  }
  
  /* fill in the block.  Note that for a short window, lW and nW are *short*
     regardless of actual settings in the stream */

  _vorbis_block_ripcord(vb);
  if(v->W){
    vb->lW=v->lW;
    vb->W=v->W;
    vb->nW=v->nW;
  }else{
    vb->lW=0;
    vb->W=v->W;
    vb->nW=0;
  }

  if(v->W){
    if(!v->lW || !v->nW)
      vbi->blocktype=BLOCKTYPE_TRANSITION;
    else
      vbi->blocktype=BLOCKTYPE_LONG;
  }else{
    if(_ve_envelope_mark(v))
      vbi->blocktype=BLOCKTYPE_IMPULSE;
    else
      vbi->blocktype=BLOCKTYPE_PADDING;
  }
 
  vb->vd=v;
  vb->sequence=v->sequence;
  vb->granulepos=v->granulepos;
  vb->pcmend=ci->blocksizes[v->W];

  
  /* copy the vectors; this uses the local storage in vb */
  {
    vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;

    /* this tracks 'strongest peak' for later psychoacoustics */
    /* moved to the global psy state; clean this mess up */
    if(vbi->ampmax>g->ampmax)g->ampmax=vbi->ampmax;
    g->ampmax=_vp_ampmax_decay(g->ampmax,v);
    vbi->ampmax=g->ampmax;

    vb->pcm=_vorbis_block_alloc(vb,sizeof(*vb->pcm)*vi->channels);
    vbi->pcmdelay=_vorbis_block_alloc(vb,sizeof(*vbi->pcmdelay)*vi->channels);
    for(i=0;i<vi->channels;i++){
      vbi->pcmdelay[i]=
	_vorbis_block_alloc(vb,(vb->pcmend+beginW)*sizeof(*vbi->pcmdelay[i]));
      memcpy(vbi->pcmdelay[i],v->pcm[i],(vb->pcmend+beginW)*sizeof(*vbi->pcmdelay[i]));
      vb->pcm[i]=vbi->pcmdelay[i]+beginW;
      
      /* before we added the delay 
      vb->pcm[i]=_vorbis_block_alloc(vb,vb->pcmend*sizeof(*vb->pcm[i]));
      memcpy(vb->pcm[i],v->pcm[i]+beginW,ci->blocksizes[v->W]*sizeof(*vb->pcm[i]));
      */

    }
  }
  
  /* handle eof detection: eof==0 means that we've not yet received EOF
                           eof>0  marks the last 'real' sample in pcm[]
                           eof<0  'no more to do'; doesn't get here */

  if(v->eofflag){
    if(v->centerW>=v->eofflag){
      v->eofflag=-1;
      vb->eofflag=1;
      return(1);
    }
  }

  /* advance storage vectors and clean up */
  {
    int new_centerNext=ci->blocksizes[1]/2+gi->delaycache;
    int movementW=centerNext-new_centerNext;

    if(movementW>0){

      _ve_envelope_shift(b->ve,movementW);
      v->pcm_current-=movementW;
      
      for(i=0;i<vi->channels;i++)
	memmove(v->pcm[i],v->pcm[i]+movementW,
		v->pcm_current*sizeof(*v->pcm[i]));
      
      
      v->lW=v->W;
      v->W=v->nW;
      v->centerW=new_centerNext;
      
      v->sequence++;
      
      if(v->eofflag){
	v->eofflag-=movementW;
	/* do not add padding to end of stream! */
	if(v->centerW>=v->eofflag){
	  v->granulepos+=movementW-(v->centerW-v->eofflag);
	}else{
	  v->granulepos+=movementW;
	}
      }else{
	v->granulepos+=movementW;
      }
    }
  }

  /* done */
  return(1);
}

int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_shared_init(v,vi,0);

  v->pcm_returned=-1;
  v->granulepos=-1;
  v->sequence=-1;

  return(0);
}

/* Unlike in analysis, the window is only partially applied for each
   block.  The time domain envelope is not yet handled at the point of
   calling (as it relies on the previous block). */

int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;
  codec_setup_info *ci=vi->codec_setup;

  /* Shift out any PCM that we returned previously */
  /* centerW is currently the center of the last block added */

  if(v->centerW>ci->blocksizes[1]/2 &&
  /* Quick additional hack; to avoid *alot* of shifts, use an
     oversized buffer.  This increases memory usage, but doesn't make
     much difference wrt L1/L2 cache pressure. */
     v->pcm_returned>8192){

    /* don't shift too much; we need to have a minimum PCM buffer of
       1/2 long block */

    int shiftPCM=v->centerW-ci->blocksizes[1]/2;
    shiftPCM=(v->pcm_returned<shiftPCM?v->pcm_returned:shiftPCM);

    v->pcm_current-=shiftPCM;
    v->centerW-=shiftPCM;
    v->pcm_returned-=shiftPCM;
    
    if(shiftPCM){
      int i;
      for(i=0;i<vi->channels;i++)
	memmove(v->pcm[i],v->pcm[i]+shiftPCM,
		v->pcm_current*sizeof(*v->pcm[i]));
    }
  }

  v->lW=v->W;
  v->W=vb->W;
  v->nW=-1;

  v->glue_bits+=vb->glue_bits;
  v->time_bits+=vb->time_bits;
  v->floor_bits+=vb->floor_bits;
  v->res_bits+=vb->res_bits;

  if(v->sequence+1 != vb->sequence)v->granulepos=-1; /* out of sequence;
                                                     lose count */

  v->sequence=vb->sequence;

  {
    int sizeW=ci->blocksizes[v->W];
    int centerW=v->centerW+ci->blocksizes[v->lW]/4+sizeW/4;
    int beginW=centerW-sizeW/2;
    int endW=beginW+sizeW;
    int beginSl;
    int endSl;
    int i,j;

    /* Do we have enough PCM/mult storage for the block? */
    if(endW>v->pcm_storage){
      /* expand the storage */
      v->pcm_storage=endW+ci->blocksizes[1];
   
      for(i=0;i<vi->channels;i++)
	v->pcm[i]=_ogg_realloc(v->pcm[i],v->pcm_storage*sizeof(*v->pcm[i])); 
    }

    /* overlap/add PCM */

    switch(v->W){
    case 0:
      beginSl=0;
      endSl=ci->blocksizes[0]/2;
      break;
    case 1:
      beginSl=ci->blocksizes[1]/4-ci->blocksizes[v->lW]/4;
      endSl=beginSl+ci->blocksizes[v->lW]/2;
      break;
    default:
      return(-1);
    }

    for(j=0;j<vi->channels;j++){
      static int seq=0;
      float *pcm=v->pcm[j]+beginW;
      float *p=vb->pcm[j];

      /* the overlap/add section */
      for(i=beginSl;i<endSl;i++)
	pcm[i]+=p[i];
      /* the remaining section */
      for(;i<sizeW;i++)
	pcm[i]=p[i];

      //_analysis_output("lapped",seq,pcm,sizeW,0,0);
      //_analysis_output("buffered",seq++,v->pcm[j],sizeW+beginW,0,0);
    
    }

    /* deal with initial packet state; we do this using the explicit
       pcm_returned==-1 flag otherwise we're sensitive to first block
       being short or long */

    if(v->pcm_returned==-1)
      v->pcm_returned=centerW;

    /* track the frame number... This is for convenience, but also
       making sure our last packet doesn't end with added padding.  If
       the last packet is partial, the number of samples we'll have to
       return will be past the vb->granulepos.
       
       This is not foolproof!  It will be confused if we begin
       decoding at the last page after a seek or hole.  In that case,
       we don't have a starting point to judge where the last frame
       is.  For this reason, vorbisfile will always try to make sure
       it reads the last two marked pages in proper sequence */

    if(v->granulepos==-1)
      if(vb->granulepos==-1){
	v->granulepos=0;
      }else{
	v->granulepos=vb->granulepos;
      }
    else{
      v->granulepos+=(centerW-v->centerW);
      if(vb->granulepos!=-1 && v->granulepos!=vb->granulepos){

	if(v->granulepos>vb->granulepos){
	  long extra=v->granulepos-vb->granulepos;

	  if(vb->eofflag){
	    /* partial last frame.  Strip the extra samples off */
	    centerW-=extra;
	  }else if(vb->sequence == 1){
	    /* ^^^ argh, this can be 1 from seeking! */


	    /* partial first frame.  Discard extra leading samples */
	    v->pcm_returned+=extra;
	    if(v->pcm_returned>centerW)v->pcm_returned=centerW;
	    
	  }
	  
	}/* else{ Shouldn't happen *unless* the bitstream is out of
	    spec.  Either way, believe the bitstream } */
	v->granulepos=vb->granulepos;
      }
    }

    /* Update, cleanup */

    v->centerW=centerW;
    v->pcm_current=endW;

    if(vb->eofflag)v->eofflag=1;
  }

  return(0);
}

/* pcm==NULL indicates we just want the pending samples, no more */
int vorbis_synthesis_pcmout(vorbis_dsp_state *v,float ***pcm){
  vorbis_info *vi=v->vi;
  if(v->pcm_returned>-1 && v->pcm_returned<v->centerW){
    if(pcm){
      int i;
      for(i=0;i<vi->channels;i++)
	v->pcmret[i]=v->pcm[i]+v->pcm_returned;
      *pcm=v->pcmret;
    }
    return(v->centerW-v->pcm_returned);
  }
  return(0);
}

int vorbis_synthesis_read(vorbis_dsp_state *v,int bytes){
  if(bytes && v->pcm_returned+bytes>v->centerW)return(OV_EINVAL);
  v->pcm_returned+=bytes;
  return(0);
}

