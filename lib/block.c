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

 function: PCM data vector blocking, windowing and dis/reassembly
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 2 1999

 Handle windowing, overlap-add, etc of the PCM vectors.  This is made
 more amusing by Vorbis' current two allowed block sizes.
 
 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "codec.h"
#include "window.h"
#include "envelope.h"
#include "mdct.h"
#include "lpc.h"

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

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  int i;
  vorbis_info *vi=v->vi;
  memset(vb,0,sizeof(vorbis_block));
  vb->vd=v;

  vb->pcm_storage=vi->blocksize[1];
  vb->pcm_channels=vi->channels;
  vb->mult_storage=vi->blocksize[1]/vi->envelopesa;
  vb->mult_channels=vi->envelopech;
  vb->floor_channels=vi->floorch;
  vb->floor_storage=max(vi->floororder[0],vi->floororder[1]);
  
  vb->pcm=malloc(vb->pcm_channels*sizeof(double *));
  for(i=0;i<vb->pcm_channels;i++)
    vb->pcm[i]=malloc(vb->pcm_storage*sizeof(double));
  
  vb->mult=malloc(vb->mult_channels*sizeof(double *));
  for(i=0;i<vb->mult_channels;i++)
    vb->mult[i]=malloc(vb->mult_storage*sizeof(double));

  vb->lsp=malloc(vb->floor_channels*sizeof(double *));
  vb->lpc=malloc(vb->floor_channels*sizeof(double *));
  vb->amp=malloc(vb->floor_channels*sizeof(double));
  for(i=0;i<vb->floor_channels;i++){
    vb->lsp[i]=malloc(vb->floor_storage*sizeof(double));
    vb->lpc[i]=malloc(vb->floor_storage*sizeof(double));
  }
  if(v->analysisp)
    _oggpack_writeinit(&vb->opb);

  return(0);
}

int vorbis_block_clear(vorbis_block *vb){
  int i;
  if(vb->pcm){
    for(i=0;i<vb->pcm_channels;i++)
      free(vb->pcm[i]);
    free(vb->pcm);
  }
  if(vb->mult){
    for(i=0;i<vb->mult_channels;i++)
      free(vb->mult[i]);
    free(vb->mult);
  }
  if(vb->vd->analysisp)
    _oggpack_writeclear(&vb->opb);

  memset(vb,0,sizeof(vorbis_block));
  return(0);
}

/* Analysis side code, but directly related to blocking.  Thus it's
   here and not in analysis.c (which is for analysis transforms only).
   The init is here because some of it is shared */

static int _vds_shared_init(vorbis_dsp_state *v,vorbis_info *vi){
  memset(v,0,sizeof(vorbis_dsp_state));

  v->vi=vi;
  _ve_envelope_init(&v->ve,vi->envelopesa);
  mdct_init(&v->vm[0],vi->blocksize[0]);
  mdct_init(&v->vm[1],vi->blocksize[1]);

  v->window[0][0][0]=_vorbis_window(vi->blocksize[0],
				   vi->blocksize[0]/2,vi->blocksize[0]/2);
  v->window[0][0][1]=v->window[0][0][0];
  v->window[0][1][0]=v->window[0][0][0];
  v->window[0][1][1]=v->window[0][0][0];

  v->window[1][0][0]=_vorbis_window(vi->blocksize[1],
				   vi->blocksize[0]/2,vi->blocksize[0]/2);
  v->window[1][0][1]=_vorbis_window(vi->blocksize[1],
				   vi->blocksize[0]/2,vi->blocksize[1]/2);
  v->window[1][1][0]=_vorbis_window(vi->blocksize[1],
				   vi->blocksize[1]/2,vi->blocksize[0]/2);
  v->window[1][1][1]=_vorbis_window(vi->blocksize[1],
				    vi->blocksize[1]/2,vi->blocksize[1]/2);

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

  /* Initialize the envelope multiplier storage */

  if(vi->envelopech){
    v->envelope_storage=v->pcm_storage/vi->envelopesa;
    v->multipliers=calloc(vi->envelopech,sizeof(double *));
    {
      int i;
      for(i=0;i<vi->envelopech;i++){
	v->multipliers[i]=calloc(v->envelope_storage,sizeof(double));
      }
    }
  }

  /* all 1 (large block) or 0 (small block) */
  /* explicitly set for the sake of clarity */
  v->lW=0; /* previous window size */
  v->W=0;  /* current window size */

  /* all vector indexes; multiples of samples_per_envelope_step */
  v->centerW=vi->blocksize[1]/2;

  v->pcm_current=v->centerW;
  v->envelope_current=v->centerW/vi->envelopesa;
  return(0);
}

/* arbitrary settings and spec-mandated numbers get filled in here */
int vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_shared_init(v,vi);

  /* the coder init is different for read/write */
  v->analysisp=1;

  /* Yes, wasteful to have four lookups.  This will get collapsed once
     things crystallize */
  lpc_init(&v->vl[0],vi->blocksize[0]/2,vi->blocksize[0]/2,
	   vi->floororder[0],vi->flooroctaves[0],1);
  lpc_init(&v->vl[1],vi->blocksize[1]/2,vi->blocksize[1]/2,
	   vi->floororder[0],vi->flooroctaves[0],1);

  /*lpc_init(&v->vbal[0],vi->blocksize[0]/2,256,
	   vi->balanceorder,vi->balanceoctaves,1);
  lpc_init(&v->vbal[1],vi->blocksize[1]/2,256,
           vi->balanceorder,vi->balanceoctaves,1);*/

  return(0);
}

void vorbis_dsp_clear(vorbis_dsp_state *v){
  int i,j,k;
  if(v){
    vorbis_info *vi=v->vi;

    if(v->window[0][0][0])free(v->window[0][0][0]);
    for(j=0;j<2;j++)
      for(k=0;k<2;k++)
	if(v->window[1][j][k])free(v->window[1][j][k]);
    if(v->pcm){
      for(i=0;i<vi->channels;i++)
	if(v->pcm[i])free(v->pcm[i]);
      free(v->pcm);
      if(v->pcmret)free(v->pcmret);
    }
    if(v->multipliers){
      for(i=0;i<vi->envelopech;i++)
	if(v->multipliers[i])free(v->multipliers[i]);
      free(v->multipliers);
    }
    _ve_envelope_clear(&v->ve);
    mdct_clear(&v->vm[0]);
    mdct_clear(&v->vm[1]);
    lpc_clear(&v->vl[0]);
    lpc_clear(&v->vl[1]);
    /*lpc_clear(&v->vbal[0]);
      lpc_clear(&v->vbal[1]);*/
    memset(v,0,sizeof(vorbis_dsp_state));
  }
}

double **vorbis_analysis_buffer(vorbis_dsp_state *v, int vals){
  int i;
  vorbis_info *vi=v->vi;

  /* Do we have enough storage space for the requested buffer? If not,
     expand the PCM (and envelope) storage */
    
  if(v->pcm_current+vals>=v->pcm_storage){
    v->pcm_storage=v->pcm_current+vals*2;
    v->envelope_storage=v->pcm_storage/v->vi->envelopesa;
   
    for(i=0;i<vi->channels;i++){
      v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double));
    }
    for(i=0;i<vi->envelopech;i++){
      v->multipliers[i]=realloc(v->multipliers[i],
				v->envelope_storage*sizeof(double));
    }
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
    vorbis_analysis_buffer(v,v->vi->blocksize[1]*2);
    v->eofflag=v->pcm_current;
    v->pcm_current+=v->vi->blocksize[1]*2;
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
  int i,j;
  vorbis_info *vi=v->vi;
  long beginW=v->centerW-vi->blocksize[v->W]/2,centerNext;
  long beginM=beginW/vi->envelopesa;

  /* check to see if we're done... */
  if(v->eofflag==-1)return(0);

  /* if we have any unfilled envelope blocks for which we have PCM
     data, fill them up in before proceeding. */

  if(v->pcm_current/vi->envelopesa>v->envelope_current){
    /* This generates the multipliers, but does not sparsify the vector.
       That's done by block before coding */
    _ve_envelope_multipliers(v);
  }

  /* By our invariant, we have lW, W and centerW set.  Search for
     the next boundary so we can determine nW (the next window size)
     which lets us compute the shape of the current block's window */

  /* overconserve for now; any block with a non-placeholder multiplier
     should be minimal size. We can be greedy and only look at nW size */
  
  if(vi->blocksize[0]<vi->blocksize[1]){
    
    if(v->W)
      /* this is a long window; we start the search forward of centerW
	 because that's the fastest we could react anyway */
      i=v->centerW+vi->blocksize[1]/4-vi->blocksize[0]/4;
    else
      /* short window.  Search from centerW */
      i=v->centerW;
    i/=vi->envelopesa;
    
    for(;i<v->envelope_current;i++){
      for(j=0;j<vi->envelopech;j++)
	if(v->multipliers[j][i-1]*vi->preecho_thresh<  
	   v->multipliers[j][i])break;
      if(j<vi->envelopech)break;
    }
    
    if(i<v->envelope_current){
      /* Ooo, we hit a multiplier. Is it beyond the boundary to make the
	 upcoming block large ? */
      long largebound;
      if(v->W)
	largebound=v->centerW+vi->blocksize[1];
      else
	largebound=v->centerW+vi->blocksize[0]/4+vi->blocksize[1]*3/4;
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
    v->nW=1;
    v->nW=1;

  /* Do we actually have enough data *now* for the next block? The
     reason to check is that if we had no multipliers, that could
     simply been due to running out of data.  In that case, we don;t
     know the size of the next block for sure and we need that now to
     figure out the window shape of this block */
  
  centerNext=v->centerW+vi->blocksize[v->W]/4+vi->blocksize[v->nW]/4;

  {
    long blockbound=centerNext+vi->blocksize[v->nW]/2;
    if(v->pcm_current<blockbound)return(0); /* not enough data yet */    
  }

  /* fill in the block */
  vb->lW=v->lW;
  vb->W=v->W;
  vb->nW=v->nW;
  vb->vd=v;

  vb->pcmend=vi->blocksize[v->W];
  vb->multend=vb->pcmend / vi->envelopesa;

  /* copy the vectors */
  for(i=0;i<vi->channels;i++)
    memcpy(vb->pcm[i],v->pcm[i]+beginW,vi->blocksize[v->W]*sizeof(double));
  for(i=0;i<vi->envelopech;i++)
    memcpy(vb->mult[i],v->multipliers[i]+beginM,vi->blocksize[v->W]/
	   vi->envelopesa*sizeof(double));

  vb->frameno=v->frame;

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
    int new_centerNext=vi->blocksize[1]/2;
    int movementW=centerNext-new_centerNext;
    int movementM=movementW/vi->envelopesa;

    /* the multipliers and pcm stay synced up because the blocksizes
       must be multiples of samples_per_envelope_step (minimum
       multiple is 2) */

    for(i=0;i<vi->channels;i++)
      memmove(v->pcm[i],v->pcm[i]+movementW,
	      (v->pcm_current-movementW)*sizeof(double));
    
    for(i=0;i<vi->envelopech;i++){
      memmove(v->multipliers[i],v->multipliers[i]+movementM,
	      (v->envelope_current-movementM)*sizeof(double));
    }

    v->pcm_current-=movementW;
    v->envelope_current-=movementM;

    v->lW=v->W;
    v->W=v->nW;
    v->centerW=new_centerNext;

    v->frame++;
    v->samples+=movementW;

    if(v->eofflag)
      v->eofflag-=movementW;
  }

  /* done */
  return(1);
}

int vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi){
  int temp=vi->envelopech;
  vi->envelopech=0; /* we don't need multiplier buffering in syn */
  _vds_shared_init(v,vi);
  vi->envelopech=temp;

  /* Yes, wasteful to have four lookups.  This will get collapsed once
     things crystallize */
  lpc_init(&v->vl[0],vi->blocksize[0]/2,vi->blocksize[0]/2,
	   vi->floororder[0],vi->flooroctaves[0],0);
  lpc_init(&v->vl[1],vi->blocksize[1]/2,vi->blocksize[1]/2,
	   vi->floororder[1],vi->flooroctaves[1],0);
  /*lpc_init(&v->vbal[0],vi->blocksize[0]/2,256,
	   vi->balanceorder,vi->balanceoctaves,0);
  lpc_init(&v->vbal[1],vi->blocksize[1]/2,256,
           vi->balanceorder,vi->balanceoctaves,0);*/


  /* Adjust centerW to allow an easier mechanism for determining output */
  v->pcm_returned=v->centerW;
  v->centerW-= vi->blocksize[v->W]/4+vi->blocksize[v->lW]/4;
  return(0);
}

/* Unike in analysis, the window is only partially applied.  Envelope
   is previously applied (the whole envelope, if any, is shipped in
   each block) */

int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){
  vorbis_info *vi=v->vi;

  /* Shift out any PCM that we returned previously */

  if(v->pcm_returned  && v->centerW>vi->blocksize[1]/2){

    /* don't shift too much; we need to have a minimum PCM buffer of
       1/2 long block */

    int shift=v->centerW-vi->blocksize[1]/2;
    shift=(v->pcm_returned<shift?v->pcm_returned:shift);

    v->pcm_current-=shift;
    v->centerW-=shift;
    v->pcm_returned-=shift;
    
    if(shift){
      int i;
      for(i=0;i<vi->channels;i++)
	memmove(v->pcm[i],v->pcm[i]+shift,
		v->pcm_current*sizeof(double));
    }
  }

  v->lW=v->W;
  v->W=vb->W;

  {
    int sizeW=vi->blocksize[v->W];
    int centerW=v->centerW+vi->blocksize[v->lW]/4+sizeW/4;
    int beginW=centerW-sizeW/2;
    int endW=beginW+sizeW;
    int beginSl;
    int endSl;
    int i,j;
    double *window;

    /* Do we have enough PCM storage for the block? */
    if(endW>v->pcm_storage){
      /* expand the PCM storage */

      v->pcm_storage=endW+vi->blocksize[1];
   
      for(i=0;i<vi->channels;i++)
	v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double)); 
    }

    /* Overlap/add */
    switch(v->W){
    case 0:
      beginSl=0;
      endSl=vi->blocksize[0]/2;
      break;
    case 1:
      beginSl=vi->blocksize[1]/4-vi->blocksize[v->lW]/4;
      endSl=beginSl+vi->blocksize[v->lW]/2;
      break;
    }

    window=v->window[v->W][0][v->lW]+vi->blocksize[v->W]/2;

    for(j=0;j<vi->channels;j++){
      double *pcm=v->pcm[j]+beginW;

      /* the add section */
      for(i=beginSl;i<endSl;i++)
	pcm[i]=pcm[i]*window[i]+vb->pcm[j][i];
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

