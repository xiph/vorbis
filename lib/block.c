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
 last modification date: Aug 05 1999

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

static int _vds_shared_init(vorbis_dsp_state *v,vorbis_info *vi){
  memset(v,0,sizeof(vorbis_dsp_state));

  memcpy(&v->vi,vi,sizeof(vorbis_info));
  _ve_envelope_init(&v->ve,vi->envelopesa);
  mdct_init(&v->vm[0],vi->smallblock);
  mdct_init(&v->vm[1],vi->largeblock);

  v->samples_per_envelope_step=vi->envelopesa;
  v->block_size[0]=vi->smallblock;
  v->block_size[1]=vi->largeblock;
  
  v->window[0][0][0]=_vorbis_window(v->block_size[0],
				   v->block_size[0]/2,v->block_size[0]/2);
  v->window[0][0][1]=v->window[0][0][0];
  v->window[0][1][0]=v->window[0][0][0];
  v->window[0][1][1]=v->window[0][0][0];

  v->window[1][0][0]=_vorbis_window(v->block_size[1],
				   v->block_size[0]/2,v->block_size[0]/2);
  v->window[1][0][1]=_vorbis_window(v->block_size[1],
				   v->block_size[0]/2,v->block_size[1]/2);
  v->window[1][1][0]=_vorbis_window(v->block_size[1],
				   v->block_size[1]/2,v->block_size[0]/2);
  v->window[1][1][1]=_vorbis_window(v->block_size[1],
				    v->block_size[1]/2,v->block_size[1]/2);

  /* initialize the storage vectors to a decent size greater than the
     minimum */
  
  v->pcm_storage=8192; /* we'll assume later that we have
			  a minimum of twice the blocksize of
			  accumulated samples in analysis */
  v->pcm_channels=v->vi.channels=vi->channels;
  v->pcm=malloc(vi->channels*sizeof(double *));
  v->pcmret=malloc(vi->channels*sizeof(double *));
  {
    int i;
    for(i=0;i<vi->channels;i++)
      v->pcm[i]=calloc(v->pcm_storage,sizeof(double));
  }

  /* Initialize the envelope multiplier storage */

  if(vi->envelopech){
    v->envelope_storage=v->pcm_storage/v->samples_per_envelope_step;
    v->envelope_channels=vi->envelopech;
    v->multipliers=calloc(v->envelope_channels,sizeof(double *));
    {
      int i;
      for(i=0;i<v->envelope_channels;i++){
	v->multipliers[i]=calloc(v->envelope_storage,sizeof(double));
      }
    }
  }

  /* all 1 (large block) or 0 (small block) */
  /* explicitly set for the sake of clarity */
  v->lW=0; /* previous window size */
  v->W=0;  /* current window size */

  /* all vector indexes; multiples of samples_per_envelope_step */
  v->centerW=v->block_size[1]/2;

  v->pcm_current=v->centerW;
  v->envelope_current=v->centerW/v->samples_per_envelope_step;
  return(0);
}

/* arbitrary settings and spec-mandated numbers get filled in here */
int vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi){
  _vds_shared_init(v,vi);

  /* Yes, wasteful to have four lookups.  This will get collapsed once
     things crystallize */
  lpc_init(&v->vl[0],vi->smallblock/2,vi->smallblock/2,
	   vi->floororder,vi->flooroctaves,1);
  lpc_init(&v->vl[1],vi->largeblock/2,vi->largeblock/2,
	   vi->floororder,vi->flooroctaves,1);

  lpc_init(&v->vbal[0],vi->smallblock/2,256,
	   vi->balanceorder,vi->balanceoctaves,1);
  lpc_init(&v->vbal[1],vi->largeblock/2,256,
	   vi->balanceorder,vi->balanceoctaves,1);

  return(0);
}

void vorbis_analysis_clear(vorbis_dsp_state *v){
  int i,j,k;
  if(v){

    if(v->window[0][0][0])free(v->window[0][0][0]);
    for(j=0;j<2;j++)
      for(k=0;k<2;k++)
	if(v->window[1][j][k])free(v->window[1][j][k]);
    if(v->pcm){
      for(i=0;i<v->pcm_channels;i++)
	if(v->pcm[i])free(v->pcm[i]);
      free(v->pcm);
      free(v->pcmret);
    }
    if(v->multipliers){
      for(i=0;i<v->envelope_channels;i++)
	if(v->multipliers[i])free(v->multipliers[i]);
      free(v->multipliers);
    }
    _ve_envelope_clear(&v->ve);
    mdct_clear(&v->vm[0]);
    mdct_clear(&v->vm[1]);
    memset(v,0,sizeof(vorbis_dsp_state));
  }
}

double **vorbis_analysis_buffer(vorbis_dsp_state *v, int vals){
  int i;

  /* Do we have enough storage space for the requested buffer? If not,
     expand the PCM (and envelope) storage */
    
  if(v->pcm_current+vals>=v->pcm_storage){
    v->pcm_storage=v->pcm_current+vals*2;
    v->envelope_storage=v->pcm_storage/v->samples_per_envelope_step;
   
    for(i=0;i<v->pcm_channels;i++){
      v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double));
    }
    for(i=0;i<v->envelope_channels;i++){
      v->multipliers[i]=realloc(v->multipliers[i],
				v->envelope_storage*sizeof(double));
    }
  }

  for(i=0;i<v->pcm_channels;i++)
    v->pcmret[i]=v->pcm[i]+v->pcm_current;
    
  return(v->pcmret);
}

/* call with val<=0 to set eof */

int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals){
  if(vals<=0){
    /* We're encoding the end of the stream.  Just make sure we have
       [at least] a full block of zeroes at the end. */

    int i;
    vorbis_analysis_buffer(v,v->block_size[1]*2);
    v->eofflag=v->pcm_current;
    v->pcm_current+=v->block_size[1]*2;
    for(i=0;i<v->pcm_channels;i++)
      memset(v->pcm[i]+v->eofflag,0,
	     (v->pcm_current-v->eofflag)*sizeof(double));
  }else{
    
    if(v->pcm_current+vals>v->pcm_storage)
      return(-1);

    v->pcm_current+=vals;
  }
  return(0);
}

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  int i;
  memset(vb,0,sizeof(vorbis_block));
  vb->pcm_storage=v->block_size[1];
  vb->pcm_channels=v->pcm_channels;
  vb->mult_storage=v->block_size[1]/v->samples_per_envelope_step;
  vb->mult_channels=v->envelope_channels;
  vb->floor_channels=v->vi.floorch;
  vb->floor_storage=v->vi.floororder;
  
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
  memset(vb,0,sizeof(vorbis_block));
  return(0);
}

/* do the deltas, envelope shaping, pre-echo and determine the size of
   the next block on which to continue analysis */
int vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb){
  int i,j;
  long beginW=v->centerW-v->block_size[v->W]/2,centerNext;
  long beginM=beginW/v->samples_per_envelope_step;

  /* check to see if we're done... */
  if(v->eofflag==-1)return(0);

  /* if we have any unfilled envelope blocks for which we have PCM
     data, fill them up in before proceeding. */

  if(v->pcm_current/v->samples_per_envelope_step>v->envelope_current){
    /* This generates the multipliers, but does not sparsify the vector.
       That's done by block before coding */
    _ve_envelope_multipliers(v);
  }

  /* By our invariant, we have lW, W and centerW set.  Search for
     the next boundary so we can determine nW (the next window size)
     which lets us compute the shape of the current block's window */

  /* overconserve for now; any block with a non-placeholder multiplier
     should be minimal size. We can be greedy and only look at nW size */
  
  if(v->vi.smallblock<v->vi.largeblock){
    
    if(v->W)
      /* this is a long window; we start the search forward of centerW
	 because that's the fastest we could react anyway */
      i=v->centerW+v->block_size[1]/4-v->block_size[0]/4;
    else
      /* short window.  Search from centerW */
      i=v->centerW;
    i/=v->samples_per_envelope_step;
    
    for(;i<v->envelope_current;i++){
      for(j=0;j<v->envelope_channels;j++)
	if(v->multipliers[j][i-1]*v->vi.preecho_thresh<  
	   v->multipliers[j][i])break;
      if(j<v->envelope_channels)break;
    }
    
    if(i<v->envelope_current){
      /* Ooo, we hit a multiplier. Is it beyond the boundary to make the
	 upcoming block large ? */
      long largebound;
      if(v->W)
	largebound=v->centerW+v->block_size[1];
      else
	largebound=v->centerW+v->block_size[0]/4+v->block_size[1]*3/4;
      largebound/=v->samples_per_envelope_step;
      
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
  
  centerNext=v->centerW+v->block_size[v->W]/4+v->block_size[v->nW]/4;

  {
    long blockbound=centerNext+v->block_size[v->nW]/2;
    if(v->pcm_current<blockbound)return(0); /* not enough data yet */    
  }

  /* fill in the block */
  vb->lW=v->lW;
  vb->W=v->W;
  vb->nW=v->nW;
  vb->vd=v;

  vb->pcmend=v->block_size[v->W];
  vb->multend=vb->pcmend / v->samples_per_envelope_step;

  if(vb->floor_channels!=v->vi.floorch ||
     vb->floor_storage!=v->vi.floororder ||
     v->pcm_channels!=vb->pcm_channels ||
     v->block_size[1]!=vb->pcm_storage ||
     v->envelope_channels!=vb->mult_channels){

    /* Storage not initialized or initilized for some other codec
       instance with different settings */

    vorbis_block_clear(vb);
    vorbis_block_init(v,vb);
  }

  /* copy the vectors */
  for(i=0;i<v->pcm_channels;i++)
    memcpy(vb->pcm[i],v->pcm[i]+beginW,v->block_size[v->W]*sizeof(double));
  for(i=0;i<v->envelope_channels;i++)
    memcpy(vb->mult[i],v->multipliers[i]+beginM,v->block_size[v->W]/
	   v->samples_per_envelope_step*sizeof(double));

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
    int new_centerNext=v->block_size[1]/2;
    int movementW=centerNext-new_centerNext;
    int movementM=movementW/v->samples_per_envelope_step;

    /* the multipliers and pcm stay synced up because the blocksizes
       must be multiples of samples_per_envelope_step (minimum
       multiple is 2) */

    for(i=0;i<v->pcm_channels;i++)
      memmove(v->pcm[i],v->pcm[i]+movementW,
	      (v->pcm_current-movementW)*sizeof(double));
    
    for(i=0;i<v->envelope_channels;i++){
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
  lpc_init(&v->vl[0],vi->smallblock/2,vi->smallblock/2,
	   vi->floororder,vi->flooroctaves,0);
  lpc_init(&v->vl[1],vi->largeblock/2,vi->largeblock/2,
	   vi->floororder,vi->flooroctaves,0);
  lpc_init(&v->vbal[0],vi->smallblock/2,256,
	   vi->balanceorder,vi->balanceoctaves,0);
  lpc_init(&v->vbal[1],vi->largeblock/2,256,
	   vi->balanceorder,vi->balanceoctaves,0);


  /* Adjust centerW to allow an easier mechanism for determining output */
  v->pcm_returned=v->centerW;
  v->centerW-= v->block_size[v->W]/4+v->block_size[v->lW]/4;
  return(0);
}

/* Unike in analysis, the window is only partially applied.  Envelope
   is previously applied (the whole envelope, if any, is shipped in
   each block) */

int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb){

  /* Shift out any PCM that we returned previously */

  if(v->pcm_returned  && v->centerW>v->block_size[1]/2){

    /* don't shift too much; we need to have a minimum PCM buffer of
       1/2 long block */

    int shift=v->centerW-v->block_size[1]/2;
    shift=(v->pcm_returned<shift?v->pcm_returned:shift);

    v->pcm_current-=shift;
    v->centerW-=shift;
    v->pcm_returned-=shift;
    
    if(shift){
      int i;
      for(i=0;i<v->pcm_channels;i++)
	memmove(v->pcm[i],v->pcm[i]+shift,
		v->pcm_current*sizeof(double));
    }
  }

  {
    int sizeW=v->block_size[vb->W];
    int centerW=v->centerW+v->block_size[vb->lW]/4+sizeW/4;
    int beginW=centerW-sizeW/2;
    int endW=beginW+sizeW;
    int beginSl;
    int endSl;
    int i,j;
    double *window;

    /* Do we have enough PCM storage for the block? */
    if(endW>v->pcm_storage){
      /* expand the PCM storage */

      v->pcm_storage=endW+v->block_size[1];
   
      for(i=0;i<v->pcm_channels;i++)
	v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double)); 
    }

    /* Overlap/add */
    switch(vb->W){
    case 0:
      beginSl=0;
      endSl=v->block_size[0]/2;
      break;
    case 1:
      beginSl=v->block_size[1]/4-v->block_size[vb->lW]/4;
      endSl=beginSl+v->block_size[vb->lW]/2;
      break;
    }

    window=v->window[vb->W][0][vb->lW]+v->block_size[vb->W]/2;

    for(j=0;j<v->pcm_channels;j++){
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
  if(v->pcm_returned<v->centerW){
    int i;
    for(i=0;i<v->pcm_channels;i++)
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

