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
 last modification date: Jul 27 1999

 Handle windowing, overlap-add, etc of the PCM vectors.  This is made
 more amusing by Vorbis' current two allowed block sizes (512 and 2048
 elements/channel).
 
 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "codec.h"

/* pcm accumulator and multipliers 
   examples (not exhaustive):

 <-------------- lW----------------->
                   <--------------- W ---------------->
:            .....|.....       _______________         |
:        .'''     |     '''_---      |       |\        |
:.....'''         |_____--- '''......|       | \_______|
:.................|__________________|_______|__|______|
                  |<------ Sl ------>|      > Sr <     |endW
                  |beginSl           |endSl  |  |endSr   
                  |beginW            |endlW  |beginSr
                  mult[0]                              mult[n]


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

 <-------------- lW----------------->
                          |<-W-->|                               
:            ..............  __  |   |                    
:        .'''             |`/  \ |   |                       
:.....'''                 |/`...\|...|                    
:.........................|__||__|___|                  
                          |Sl||Sr|endW    
                          |  ||  |endSr
                          |  ||beginSr
                          |  |endSl
			  |beginSl
			  |beginW
                          mult[0]
                                 mult[n]
*/

/* arbitrary settings and spec-mandated numbers get filled in here */
int vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi){
  memset(v,0,sizeof(vorbis_dsp_state));
  v->samples_per_envelope_step=64;
  v->block_size[0]=512; 
  v->block_size[1]=2048;
  
  /*  v->window[0][0][0]=vorbis_window(v->block_size[0],
				   v->block_size[0]/2,v->block_size[0]/2);
  v->window[1][0][0]=vorbis_window(v->block_size[1],
				   v->block_size[0]/2,v->block_size[0]/2);
  v->window[1][0][1]=vorbis_window(v->block_size[1],
				   v->block_size[0]/2,v->block_size[1]/2);
  v->window[1][1][0]=vorbis_window(v->block_size[1],
				   v->block_size[1]/2,v->block_size[0]/2);
  v->window[1][1][1]=vorbis_window(v->block_size[1],
  v->block_size[1]/2,v->block_size[1]/2);*/

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
  
  v->envelope_storage=v->pcm_storage/v->samples_per_envelope_step;
  v->envelope_channels=vi->channels;
  v->deltas=calloc(v->envelope_channels,sizeof(double *));
  v->multipliers=calloc(v->envelope_channels,sizeof(int *));
  {
    int i;
    for(i=0;i<v->envelope_channels;i++){
      v->deltas[i]=calloc(v->envelope_storage,sizeof(double));
      v->multipliers[i]=calloc(v->envelope_storage,sizeof(int));
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

void vorbis_analysis_clear(vorbis_dsp_state *v){
  int i,j,k;
  if(v){
    for(i=0;i<2;i++)
      for(j=0;j<2;j++)
	for(k=0;k<2;k++)
	  if(v->window[i][j][k])free(v->window[i][j][k]);
    if(v->pcm){
      for(i=0;i<v->pcm_channels;i++)
	if(v->pcm[i])free(v->pcm[i]);
      free(v->pcm);
      free(v->pcmret);
    }
    if(v->deltas){
      for(i=0;i<v->envelope_channels;i++)
	if(v->deltas[i])free(v->deltas[i]);
      free(v->deltas);
    }
    if(v->multipliers){
      for(i=0;i<v->envelope_channels;i++)
	if(v->multipliers[i])free(v->multipliers[i]);
      free(v->multipliers);
    }
    memset(v,0,sizeof(vorbis_dsp_state));
  }
}

/* call with val==0 to set eof */
double **vorbis_analysis_buffer(vorbis_dsp_state *v, int vals){
  int i;
  if(vals<=0){
    /* We're encoding the end of the stream.  Just make sure we have
       [at least] a full block of zeroes at the end. */
    v->eofflag=v->pcm_current;
    v->pcm_current+=v->block_size[1]*2;
    vals=0;
  }

  /* Do we have enough storage space for the requested buffer? If not,
     expand the PCM (and envelope) storage */
    
  if(v->pcm_current+vals>=v->pcm_storage){
    v->pcm_storage=v->pcm_current+vals*2;
    v->envelope_storage=v->pcm_storage/v->samples_per_envelope_step;
   
    for(i=0;i<v->pcm_channels;i++){
      v->pcm[i]=realloc(v->pcm[i],v->pcm_storage*sizeof(double));
      v->deltas[i]=realloc(v->deltas[i],v->envelope_storage*sizeof(double));
      v->multipliers[i]=realloc(v->multipliers[i],
				v->envelope_storage*sizeof(double));

    }
  }

  if(vals<=0){
    /* We're encoding the end of the stream.  Just make sure we have
       [at least] a full block of zeroes at the end. */
    
    for(i=0;i<v->pcm_channels;i++)
      memset(v->pcm[i]+v->eofflag,0,
	     (v->pcm_current-v->eofflag)*sizeof(double));
  }

  for(i=0;i<v->pcm_channels;i++)
    v->pcmret[i]=v->pcm[i]+v->pcm_current;
    
  return(v->pcmret);
}

int vorbis_analysis_wrote(vorbis_dsp_state *v, int vals){
  if(v->pcm_current+vals>v->pcm_storage)
    return(-1);

  v->pcm_current+=vals;
  return(0);
}

int vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb){
  int i;
  vb->pcm_storage=v->block_size[1];
  vb->pcm_channels=v->pcm_channels;
  vb->mult_storage=v->block_size[1]/v->samples_per_envelope_step;
  vb->mult_channels=v->envelope_channels;
  
  vb->pcm=malloc(vb->pcm_channels*sizeof(double *));
  for(i=0;i<vb->pcm_channels;i++)
    vb->pcm[i]=malloc(vb->pcm_storage*sizeof(double));
  
  vb->mult=malloc(vb->mult_channels*sizeof(int *));
  for(i=0;i<vb->mult_channels;i++)
    vb->mult[i]=malloc(vb->mult_storage*sizeof(int));
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
int vorbis_analysis_block(vorbis_dsp_state *v,vorbis_block *vb){
  int i;
  long beginW=v->centerW-v->block_size[v->W]/2,centerNext;
  long beginM=beginW/v->samples_per_envelope_step;

  /* check to see if we're done... */
  if(v->eofflag==-1)return(0);

  /* if we have any unfilled envelope blocks for which we have PCM
     data, fill them up in before proceeding. */

  if(v->pcm_current/v->samples_per_envelope_step>v->envelope_current){
    _va_envelope_deltas(v);
    _va_envelope_multipliers(v);
  }

  /* By our invariant, we have lW, W and centerW set.  Search for
     the next boundary so we can determine nW (the next window size)
     which lets us compute the shape of the current block's window */

  /* overconserve for now; any block with a non-placeholder multiplier
     should be minimal size. We can be greedy and only look at nW size */
  
  if(v->W)
    /* this is a long window; we start the search forward of centerW
       because that's the fastest we could react anyway */
    i=v->centerW+v->block_size[1]/4-v->block_size[0]/4;
  else
    /* short window.  Search from centerW */
    i=v->centerW;
  i/=v->samples_per_envelope_step;

  for(;i<v->envelope_storage;i++)if(v->multipliers[i])break;
  
  if(i<v->envelope_storage){
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

  /* Do we actually have enough data *now* for the next block? The
     reason to check is that if we had no multipliers, that could
     simply been due to running out of data.  In that case, we don;t
     know the size of the next block for sure and we need that now to
     figure out the window shape of this block */
  
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

  if(v->pcm_channels!=vb->pcm_channels ||
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
	   v->samples_per_envelope_step*sizeof(int));

  vb->frameno=v->frame;

  /* handle eof detection: eof==0 means that we've not yet received EOF
                           eof>0  marks the last 'real' sample in pcm[]
                           eof<0  'no more to do'; doesn't get here */

  if(v->eofflag){
    long endW=beginW+v->block_size[v->W];
    if(endW>=v->eofflag){
      v->eofflag=-1;
      vb->eofflag=1;
    }
  }

  /* advance storage vectors and clean up */
  {
    int new_centerNext=v->block_size[1]/2;
    int movementW=centerNext-new_centerNext;
    int movementM=movementW/v->samples_per_envelope_step;

    for(i=0;i<v->pcm_channels;i++)
      memmove(v->pcm[i],v->pcm[i]+movementW,
	      (v->pcm_current-movementW)*sizeof(double));
    
    for(i=0;i<v->envelope_channels;i++){
      memmove(v->deltas[i],v->deltas[i]+movementM,
	      (v->envelope_current-movementM)*sizeof(double));
      memmove(v->multipliers[i],v->multipliers[i]+movementM,
	      (v->envelope_current-movementM)*sizeof(int));
    }

    v->pcm_current-=movementW;
    v->envelope_current-=movementM;

    v->lW=v->W;
    v->W=v->nW;
    v->centerW=new_centerNext;

    v->frame++;
    v->samples+=movementW;
  }

  /* done */
  return(1);
}










int vorbis_analysis_packetout(vorbis_dsp_state *v, vorbis_block *vb,
			      ogg_packet *op){

  /* find block's envelope vector and apply it */


  /* the real analysis begins; forward MDCT with window */

  
  /* Noise floor, resolution floor */

  /* encode the floor into LSP; get the actual floor back for quant */

  /* use noise floor, res floor for culling, actual floor for quant */

  /* encode residue */

}




