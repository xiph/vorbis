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
 last modification date: Jun 26 1999

 Handle windowing, overlap-add, etc of the original (and synthesized)
 PCM vectors.  This is made more amusing by Vorbis' current two allowed
 block sizes (512 and 2048 elements/channel).
 
 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdlib.h>

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

typedef struct vorbis_state{
  int samples_per_envelope_step;
  int block_size[2];
  double *window[2][2][2]; /* windowsize, leadin, leadout */

  double **pcm;
  int      pcm_storage;
  int      pcm_channels;
  int      pcm_current;

  double **deltas;
  int    **multipliers;
  int      envelope_storage;
  int      envelope_channels;
  int      envelope_current;

  int  initflag;

  long lW;
  long W;
  long Sl;
  long Sr;

  long beginW;
  long endW;
  long beginSl;
  long endSl;
  long beginSr;
  long endSr;

  long frame;
  long samples;

} vorbis_state;

/* arbitrary settings and spec-mandated numbers get filled in here */
void vorbis_init_state(vorbis_state *v,int channels,int mode){
  memset(v,0,sizeof(vorbis_state));
  v->samples_per_envelope_step=64;
  v->block_size[0]=512; 
  v->block_size[1]=2048;
  
  v->window[0][0][0]=vorbis_window(v->block_size[0],
				   v->block_size[0]/2,v->block_size[0]/2);
  v->window[1][0][0]=vorbis_window(v->block_size[1],
				   v->block_size[0]/2,v->block_size[0]/2);
  v->window[1][0][1]=vorbis_window(v->block_size[1],
				   v->block_size[0]/2,v->block_size[1]/2);
  v->window[1][1][0]=vorbis_window(v->block_size[1],
				   v->block_size[1]/2,v->block_size[0]/2);
  v->window[1][1][1]=vorbis_window(v->block_size[1],
				   v->block_size[1]/2,v->block_size[1]/2);

  /* initialize the storage vectors to a decent size greater than the
     minimum */
  
  v->pcm_storage=8192; /* 8k samples.  we'll assume later that we have
			  a minimum of twice the blocksize (2k) of
			  accumulated samples in analysis */
  v->pcm_channels=channels;
  v->pcm=malloc(channels*sizeof(double *));
  {
    int i;
    for(i=0;i<channels;i++)
      v->pcm[i]=calloc(v->pcm_storage,sizeof(double));
  }

  /* Initialize the envelope multiplier storage */
  
  v->envelope_storage=v->pcmstorage/v->samples_per_envelope_step+1;
  v->envelope_channels=channels;
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
  /*v->lW=0; previous window size */
  /*v->W=0;  determined during analysis */
  /*v->Sl=0; previous Sr */
  /*v->Sr=0; determined during analysis */

  /* all vector indexes; multiples of samples_per_envelope_step */
  /*v->beginW=0;  determined during analysis */
  /*v->endW=0;    determined during analysis */
    v->beginSl=v->block_size[1]/4-v->block_size[0]/4;
    v->endSl=v->beginSl+v->block_size[0]/2;
  /*v->beginSr=0; determined during analysis */
  /*v->endSr=0;   determined during analysis */

  /*v->frame=0;*/
  /*v->samples=0;*/

  v->pcm_current=v->endSl;
  v->last_multiplier=v->endSl/v->samples_per_envelope_step+1;

  v->initflag=1;
}

void vorbis_free_state(vorbis_state *v){
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
    free(v);
  }
}

int vorbis_analysis(vorbis_state *v, double **pcm, int vals){
  int i;

  /* vorbis encode state initialization */
  if(!v->initflag)
    vorbis_init_settings(v);

  /* first we need to handle incoming data (if any) */
  
  if(vals>0){
    /* Do we have enough storage space for the incoming data? If not,
       expand the PCM storage */

    if(v->pcm_current+vals>=pcm_storage){
      for(i=0;i<v->pcm_channels;i++)
	v->pcm[i]=realloc(v->pcm[i],
			  (v->pcm_current+vals*2)*sizeof(double));
      v->pcm_storage=v->pcm_current+vals*2;
    }

    /* If we're encoding the end of the stream and we're handing in
       padding, vals will be set, but the passed in buffer will be
       NULL; just add in zeroes */

    for(i=0;i<v->pcm_channels;i++)
      if(pcm==NULL)
	memset(v->pcm[i]+v->pcm_current,0,vals*sizeof(double));
      else
	memcpy(v->pcm[i]+v->pcm_current,pcm[i],vals*sizeof(double));

    v->pcm_current+=vals;
  }

  /* Do we definately have enough for a frame? We assume we have more
     than actually necessary to encode the current block to make some
     analysis easier. */

  if(v->pcm_current-v->endSl<v->blocksize[1]*2)
    return(0);

  /* we have enough. begin analysis */
  /* complete the envelope analysis vectors */

  

  /* decide the blocksize of this frame */


  /* algebra to set the rest of the window alignment vectors; many are
     just derived, but they make the process clearer for the time
     being */



  /* the real analysis begins; forward MDCT with window */

  
  /* Noise floor, resolution floor */

  /* encode the floor into LSP; get the actual floor back for quant */

  /* use noise floor, res floor for culling, actual floor for quant */

  /* encode residue */

  /* advance storage vectors and clean up */
  /* center the window leadout on blocksize[1]/4 */
  {
    int new_beginSr,new_endSr,movement,emove;

    /* first do the pcm storage */
    if(v->Sr){
      new_beginSl=0;
      new_endSl=v->blocksize[1]/2;
    }else{
      new_beginSl=v->blocksize[1]/4-v->blocksize[0]/4;
      new_endSl=new_beginSr+v->blocksize[0]/2;
    }
    movement=v->beginSr-new_beginSl;

    for(i=0;i<v->pcm_channels;i++)
      memmove(v->pcm[i],v->pcm[i]+movement,
	      (v->pcm_current-movement)*sizeof(double));
    v->pcm_current-=movement;
    v->lW=W;
    v->Sl=v->Sr;
    v->beginSl=new_beginSl;
    v->endSl=new_endSl;
    v->frame++;
    v->samples+=movement;

    /* now advance the multipliers */
    emove=movement/samples_per_envelope_step; 
    for(i=0;i<v->envelope_channels;i++){
      memmove(v->deltas[i],v->deltas[i]+emove,
	      (v->envelope_current-emove)*sizeof(double));
      memmove(v->multipliers[i],v->multipliers[i]+emove,
	      (v->envelope_current-emove)*sizeof(int));
    }
    v->envelope_current-=emove;
  }

  /* done */
  return(1);
}




