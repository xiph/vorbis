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

 function: PCM data vector blocking, windowing and dis/reassembly
 last mod: $Id: block.c,v 1.30 2000/05/08 20:49:48 xiphmont Exp $

 Handle windowing, overlap-add, etc of the PCM vectors.  This is made
 more amusing by Vorbis' current two allowed block sizes.
 
 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"

#include "window.h"
#include "envelope.h"
#include "mdct.h"
#include "lpc.h"
#include "bitwise.h"
#include "registry.h"
#include "sharedbook.h"
#include "bookinternal.h"
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
  memset(vb,0,sizeof(vorbis_block));
  vb->vd=v;
  vb->localalloc=0;
  vb->localstore=NULL;
  if(v->analysisp)
    _oggpack_writeinit(&vb->opb);

  return(0);
}

void *_vorbis_block_alloc(vorbis_block *vb,long bytes){
  bytes=(bytes+(WORD_ALIGN-1)) & ~(WORD_ALIGN-1);
  if(bytes+vb->localtop>vb->localalloc){
    /* can't just realloc... there are outstanding pointers */
    if(vb->localstore){
      struct alloc_chain *link=malloc(sizeof(struct alloc_chain));
      vb->totaluse+=vb->localtop;
      link->next=vb->reap;
      link->ptr=vb->localstore;
      vb->reap=link;
    }
    /* highly conservative */
    vb->localalloc=bytes;
    vb->localstore=malloc(vb->localalloc);
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
    free(reap->ptr);
    memset(reap,0,sizeof(struct alloc_chain));
    free(reap);
    reap=next;
  }
  /* consolidate storage */
  if(vb->totaluse){
    vb->localstore=realloc(vb->localstore,vb->totaluse+vb->localalloc);
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
      _oggpack_writeclear(&vb->opb);
  _vorbis_block_ripcord(vb);
  if(vb->localstore)free(vb->localstore);

  memset(vb,0,sizeof(vorbis_block));
  return(0);
}

/* Analysis side code, but directly related to blocking.  Thus it's
   here and not in analysis.c (which is for analysis transforms only).
   The init is here because some of it is shared */

static int _vds_shared_init(vorbis_dsp_state *v,vorbis_info *vi,int encp){
  int i;
  memset(v,0,sizeof(vorbis_dsp_state));

  v->vi=vi;
  v->modebits=ilog2(vi->modes);

  v->transform[0]=calloc(VI_TRANSFORMB,sizeof(vorbis_look_transform *));
  v->transform[1]=calloc(VI_TRANSFORMB,sizeof(vorbis_look_transform *));

  /* MDCT is tranform 0 */

  v->transform[0][0]=calloc(1,sizeof(mdct_lookup));
  v->transform[1][0]=calloc(1,sizeof(mdct_lookup));
  mdct_init(v->transform[0][0],vi->blocksizes[0]);
  mdct_init(v->transform[1][0],vi->blocksizes[1]);

  v->window[0][0][0]=calloc(VI_WINDOWB,sizeof(double *));
  v->window[0][0][1]=v->window[0][0][0];
  v->window[0][1][0]=v->window[0][0][0];
  v->window[0][1][1]=v->window[0][0][0];
  v->window[1][0][0]=calloc(VI_WINDOWB,sizeof(double *));
  v->window[1][0][1]=calloc(VI_WINDOWB,sizeof(double *));
  v->window[1][1][0]=calloc(VI_WINDOWB,sizeof(double *));
  v->window[1][1][1]=calloc(VI_WINDOWB,sizeof(double *));

  for(i=0;i<VI_WINDOWB;i++){
    v->window[0][0][0][i]=
      _vorbis_window(i,vi->blocksizes[0],vi->blocksizes[0]/2,vi->blocksizes[0]/2);
    v->window[1][0][0][i]=
      _vorbis_window(i,vi->blocksizes[1],vi->blocksizes[0]/2,vi->blocksizes[0]/2);
    v->window[1][0][1][i]=
      _vorbis_window(i,vi->blocksizes[1],vi->blocksizes[0]/2,vi->blocksizes[1]/2);
    v->window[1][1][0][i]=
      _vorbis_window(i,vi->blocksizes[1],vi->blocksizes[1]/2,vi->blocksizes[0]/2);
    v->window[1][1][1][i]=
      _vorbis_window(i,vi->blocksizes[1],vi->blocksizes[1]/2,vi->blocksizes[1]/2);
  }

  if(encp){ /* encode/decode differ here */
    /* finish the codebooks */
    v->fullbooks=calloc(vi->books,sizeof(codebook));
    for(i=0;i<vi->books;i++)
      vorbis_book_init_encode(v->fullbooks+i,vi->book_param[i]);
    v->analysisp=1;
  }else{
    /* finish the codebooks */
    v->fullbooks=calloc(vi->books,sizeof(codebook));
    for(i=0;i<vi->books;i++)
      vorbis_book_init_decode(v->fullbooks+i,vi->book_param[i]);
  }

  /* initialize the storage vectors to a decent size greater than the
     minimum */
  
  v->pcm_storage=8192; /* we'll assume later that we have
			  a minimum of twice the blocksize of
			  accumulated samples in analysis */
  v->pcm=malloc(vi->channels*sizeof(double *));
  v->pcmret=malloc(vi->channels*sizeof(double *));
  {
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=calloc(v->pcm_storage,sizeof(double));
  }

  /* all 1 (large block) or 0 (small block) */
  /* explicitly set for the sake of clarity */
  v->lW=0; /* previous window size */
  v->W=0;  /* current window size */

  /* all vector indexes; multiples of samples_per_envelope_step */
  v->centerW=vi->blocksizes[1]/2;

  v->pcm_current=v->centerW;

  /* initialize all the mapping/backend lookups */
  v->mode=calloc(vi->modes,sizeof(vorbis_look_mapping *));
  for(i=0;i<vi->modes;i++){
    int mapnum=vi->mode_param[i]->mapping;
    int maptype=vi->map_type[mapnum];
    v->mode[i]=_mapping_P[maptype]->look(v,vi->mode_param[i],
					 vi->map_param[mapnum]);
  }

  return(0);
}

/* arbitrary settings and spec-mandated numbers get filled in here */
int vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_shared_init(v,vi,1);

  /* Initialize the envelope multiplier storage */

  v->envelope_storage=v->pcm_storage/vi->envelopesa;
  v->multipliers=calloc(v->envelope_storage,sizeof(double));
  _ve_envelope_init(&v->ve,vi->envelopesa);

  v->envelope_current=v->centerW/vi->envelopesa;
  return(0);
}

void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i,j,k;
  if(v){
    vorbis_info *vi=v->vi;

    if(v->window[0][0][0]){
      for(i=0;i<VI_WINDOWB;i++)
	if(v->window[0][0][0][i])free(v->window[0][0][0][i]);
      free(v->window[0][0][0]);

      for(j=0;j<2;j++)
	for(k=0;k<2;k++){
	  for(i=0;i<VI_WINDOWB;i++)
	    if(v->window[1][j][k][i])free(v->window[1][j][k][i]);
	  free(v->window[1][j][k]);
	}
    }
    
    if(v->pcm){
      for(i=0;i<vi->channels;i++)
	if(v->pcm[i])free(v->pcm[i]);
      free(v->pcm);
      if(v->pcmret)free(v->pcmret);
    }
    if(v->multipliers)free(v->multipliers);

    _ve_envelope_clear(&v->ve);
    if(v->transform[0]){
      mdct_clear(v->transform[0][0]);
      free(v->transform[0][0]);
      free(v->transform[0]);
    }
    if(v->transform[1]){
      mdct_clear(v->transform[1][0]);
      free(v->transform[1][0]);
      free(v->transform[1]);
    }

    /* free mode lookups; these are actually vorbis_look_mapping structs */
    if(vi){
      for(i=0;i<vi->modes;i++){
	int mapnum=vi->mode_param[i]->mapping;
	int maptype=vi->map_type[mapnum];
	_mapping_P[maptype]->free_look(v->mode[i]);
      }
      /* free codebooks */
      for(i=0;i<vi->books;i++)
	vorbis_book_clear(v->fullbooks+i);
    }

    if(v->mode)free(v->mode);    
    if(v->fullbooks)free(v->fullbooks);

    /* free header, header1, header2 */
    if(v->header)free(v->header);
    if(v->header1)free(v->header1);
    if(v->header2)free(v->header2);

    memset(v,0,sizeof(vorbis_dsp_state));
  }
}

double **vorbis_analysis_buffer(vorbis_dsp_state *v, int vals){
  int i;
  vorbis_info *vi=v->vi;

  /* free header, header1, header2 */
  if(v->header)free(v->header);v->header=NULL;
  if(v->header1)free(v->header1);v->header1=NULL;
  if(v->header2)free(v->header2);v->header2=NULL;

  /* Do we have enough storage space for the requested buffer? If not,
     expand the PCM (and envelope) storage */
    
  if(v->pcm_current+vals>=v->pcm_storage){
    v->pcm_storage=v->pcm_current+vals*2;
    v->envelope_storage=v->pcm_storage/v->vi->envelopesa;
   
    for(i=0;i<vi->channels;i++){
      v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double));
    }
    v->multipliers=realloc(v->multipliers,v->envelope_storage*sizeof(double));
  }

  for(i=0;i<vi->channels;i++)
    v->pcmret[i]=v->pcm[i]+v->pcm_current;
    
  return(v->pcmret);
}

/* call with val<=0 to set eof */

int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals){
  vorbis_info *vi=v->vi;
  if(vals<=0){
    /* We're encoding the end of the stream.  Just make sure we have
       [at least] a full block of zeroes at the end. */

    int i;
    vorbis_analysis_buffer(v,v->vi->blocksizes[1]*2);
    v->eofflag=v->pcm_current;
    v->pcm_current+=v->vi->blocksizes[1]*2;
    for(i=0;i<vi->channels;i++)
      memset(v->pcm[i]+v->eofflag,0,
	     (v->pcm_current-v->eofflag)*sizeof(double));
  }else{
    
    if(v->pcm_current+vals>v->pcm_storage)
      return(-1);

    v->pcm_current+=vals;
  }
  return(0);
}

/* do the deltas, envelope shaping, pre-echo and determine the size of
   the next block on which to continue analysis */
int vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb){
  int i;
  vorbis_info *vi=v->vi;
  long beginW=v->centerW-vi->blocksizes[v->W]/2,centerNext;

  /* check to see if we're done... */
  if(v->eofflag==-1)return(0);

  /* if we have any unfilled envelope blocks for which we have PCM
     data, fill them up in before proceeding. */

  if(v->pcm_current/vi->envelopesa>v->envelope_current){
    /* This generates the multipliers, but does not sparsify the vector.
       That's done by block before coding */
    _ve_envelope_deltas(v);
  }

  /* By our invariant, we have lW, W and centerW set.  Search for
     the next boundary so we can determine nW (the next window size)
     which lets us compute the shape of the current block's window */
  
  if(vi->blocksizes[0]<vi->blocksizes[1]){
    
    if(v->W)
      /* this is a long window; we start the search forward of centerW
	 because that's the fastest we could react anyway */
      i=v->centerW+vi->blocksizes[1]/4-vi->blocksizes[0]/4;
    else
      /* short window.  Search from centerW */
      i=v->centerW;
    i/=vi->envelopesa;
    
    for(;i<v->envelope_current-1;i++){
      /* Compare last with current; do we have an abrupt energy change? */
      
      if(v->multipliers[i-1]*vi->preecho_thresh<  
	 v->multipliers[i])break;
      
      /* because the overlapping nature of the delta finding
	 'smears' the energy cliffs, also compare completely
	 unoverlapped areas just in case the plosive happened in an
	 unlucky place */
      
      if(v->multipliers[i-1]*vi->preecho_thresh<  
	 v->multipliers[i+1])break;
	
    }
    
    if(i<v->envelope_current-1){
      /* Ooo, we hit a multiplier. Is it beyond the boundary to make the
	 upcoming block large ? */
      long largebound;
      if(v->W)
	/* min boundary; nW large, next small */
	largebound=v->centerW+vi->blocksizes[1]*3/4+vi->blocksizes[0]/4;
      else
	/* min boundary; nW large, next small */
	largebound=v->centerW+vi->blocksizes[0]/2+vi->blocksizes[1]/2;
      largebound/=vi->envelopesa;
      
      if(i>=largebound)
	v->nW=1;
      else
	v->nW=0;
      
    }else{
      /* Assume maximum; if the block is incomplete given current
	 buffered data, this will be detected below */
      v->nW=1;
    }
  }else
    v->nW=0;

  /* Do we actually have enough data *now* for the next block? The
     reason to check is that if we had no multipliers, that could
     simply been due to running out of data.  In that case, we don't
     know the size of the next block for sure and we need that now to
     figure out the window shape of this block */
  
  centerNext=v->centerW+vi->blocksizes[v->W]/4+vi->blocksizes[v->nW]/4;

  {
    /* center of next block + next block maximum right side.  Note
       that the next block needs an additional vi->envelopesa samples 
       to actually be written (for the last multiplier), but we didn't
       need that to determine its size */

    long blockbound=centerNext+vi->blocksizes[v->nW]/2;
    if(v->pcm_current<blockbound)return(0); /* not enough data yet */    
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
  vb->vd=v;
  vb->sequence=v->sequence;
  vb->frameno=v->frameno;
  vb->pcmend=vi->blocksizes[v->W];
  
  /* copy the vectors; this uses the local storage in vb */
  {
    vb->pcm=_vorbis_block_alloc(vb,sizeof(double *)*vi->channels);
    for(i=0;i<vi->channels;i++){
      vb->pcm[i]=_vorbis_block_alloc(vb,vb->pcmend*sizeof(double));
      memcpy(vb->pcm[i],v->pcm[i]+beginW,vi->blocksizes[v->W]*sizeof(double));
    }
  }
  
  /* handle eof detection: eof==0 means that we've not yet received EOF
                           eof>0  marks the last 'real' sample in pcm[]
                           eof<0  'no more to do'; doesn't get here */

  if(v->eofflag){
    if(v->centerW>=v->eofflag){
      v->eofflag=-1;
      vb->eofflag=1;
    }
  }

  /* advance storage vectors and clean up */
  {
    int new_centerNext=vi->blocksizes[1]/2;
    int movementW=centerNext-new_centerNext;
    int movementM=movementW/vi->envelopesa;

    /* the multipliers and pcm stay synced up because the blocksize
       must be multiples of samples_per_envelope_step (minimum
       multiple is 2) */

    v->pcm_current-=movementW;
    v->envelope_current-=movementM;

    for(i=0;i<vi->channels;i++)
      memmove(v->pcm[i],v->pcm[i]+movementW,
	      v->pcm_current*sizeof(double));
    
    memmove(v->multipliers,v->multipliers+movementM,
	    v->envelope_current*sizeof(double));

    v->lW=v->W;
    v->W=v->nW;
    v->centerW=new_centerNext;

    v->sequence++;
    v->frameno+=movementW;

    if(v->eofflag)
      v->eofflag-=movementW;
  }

  /* done */
  return(1);
}

int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_shared_init(v,vi,0);

  /* Adjust centerW to allow an easier mechanism for determining output */
  v->pcm_returned=v->centerW;
  v->centerW-= vi->blocksizes[v->W]/4+vi->blocksizes[v->lW]/4;
  return(0);
}

/* Unike in analysis, the window is only partially applied for each
   block.  The time domain envelope is not yet handled at the point of
   calling (as it relies on the previous block). */

int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;

  /* Shift out any PCM/multipliers that we returned previously */
  /* centerW is currently the center of the last block added */
  if(v->pcm_returned  && v->centerW>vi->blocksizes[1]/2){

    /* don't shift too much; we need to have a minimum PCM buffer of
       1/2 long block */

    int shiftPCM=v->centerW-vi->blocksizes[1]/2;
    shiftPCM=(v->pcm_returned<shiftPCM?v->pcm_returned:shiftPCM);

    v->pcm_current-=shiftPCM;
    v->centerW-=shiftPCM;
    v->pcm_returned-=shiftPCM;
    
    if(shiftPCM){
      int i;
      for(i=0;i<vi->channels;i++)
	memmove(v->pcm[i],v->pcm[i]+shiftPCM,
		v->pcm_current*sizeof(double));
    }
  }

  v->lW=v->W;
  v->W=vb->W;
  v->nW=-1;

  v->glue_bits+=vb->glue_bits;
  v->time_bits+=vb->time_bits;
  v->floor_bits+=vb->floor_bits;
  v->res_bits+=vb->res_bits;
  v->sequence=vb->sequence;

  {
    int sizeW=vi->blocksizes[v->W];
    int centerW=v->centerW+vi->blocksizes[v->lW]/4+sizeW/4;
    int beginW=centerW-sizeW/2;
    int endW=beginW+sizeW;
    int beginSl;
    int endSl;
    int i,j;

    /* Do we have enough PCM/mult storage for the block? */
    if(endW>v->pcm_storage){
      /* expand the storage */
      v->pcm_storage=endW+vi->blocksizes[1];
   
      for(i=0;i<vi->channels;i++)
	v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double)); 
    }

    /* overlap/add PCM */

    switch(v->W){
    case 0:
      beginSl=0;
      endSl=vi->blocksizes[0]/2;
      break;
    case 1:
      beginSl=vi->blocksizes[1]/4-vi->blocksizes[v->lW]/4;
      endSl=beginSl+vi->blocksizes[v->lW]/2;
      break;
    }

    for(j=0;j<vi->channels;j++){
      double *pcm=v->pcm[j]+beginW;
      
      /* the overlap/add section */
      for(i=beginSl;i<endSl;i++)
	pcm[i]+=vb->pcm[j][i];
      /* the remaining section */
      for(;i<sizeW;i++)
	pcm[i]=vb->pcm[j][i];
    }

    /* Update, cleanup */

    v->centerW=centerW;
    v->pcm_current=endW;

    if(vb->eofflag)v->eofflag=1;
  }
  return(0);
}

int vorbis_synthesis_pcmout(vorbis_dsp_state *v,double ***pcm){
  vorbis_info *vi=v->vi;
  if(v->pcm_returned<v->centerW){
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcmret[i]=v->pcm[i]+v->pcm_returned;
    *pcm=v->pcmret;
    return(v->centerW-v->pcm_returned);
  }
  return(0);
}

int vorbis_synthesis_read(vorbis_dsp_state *v,int bytes){
  if(bytes && v->pcm_returned+bytes>v->centerW)return(-1);
  v->pcm_returned+=bytes;
  return(0);
}

