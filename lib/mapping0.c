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

 function: channel mapping 0 implementation
 last mod: $Id: mapping0.c,v 1.2 2000/01/22 13:28:23 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"
#include "registry.h"
#include "mapping0.h"

_vi_info_map *_vorbis_map0_dup(vorbis_info *vi,_vi_info_map *source){
  vorbis_info_mapping0 *d=malloc(sizeof(vorbis_info_mapping0));
  vorbis_info_mapping0 *s=(vorbis_info_mapping0 *)source;
  memcpy(d,s,sizeof(vorbis_info_mapping0));

  d->floorsubmap=malloc(sizeof(int)*vi->channels);
  d->residuesubmap=malloc(sizeof(int)*vi->channels);
  d->psysubmap=malloc(sizeof(int)*vi->channels);
  memcpy(d->floorsubmap,s->floorsubmap,sizeof(int)*vi->channels);
  memcpy(d->residuesubmap,s->residuesubmap,sizeof(int)*vi->channels);
  memcpy(d->psysubmap,s->psysubmap,sizeof(int)*vi->channels);

  return(d);
}

void _vorbis_map0_free(_vi_info_map *i){
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)i;

  if(d){
    if(d->floorsubmap)free(d->floorsubmap);
    if(d->residuesubmap)free(d->residuesubmap);
    if(d->psysubmap)free(d->psysubmap);
    memset(d,0,sizeof(vorbis_info_mapping0));
    free(d);
  }
}

void _vorbis_map0_pack(vorbis_info *vi,oggpack_buffer *opb,
			      _vi_info_map *source){
  int i;
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)source;

  _oggpack_write(opb,d->timesubmap,8);

  /* we abbreviate the channel submappings if they all map to the same
     floor/res/psy */
  for(i=1;i<vi->channels;i++)if(d->floorsubmap[i]!=d->floorsubmap[i-1])break;
  if(i==vi->channels){
    _oggpack_write(opb,0,1);
    _oggpack_write(opb,d->floorsubmap[0],8);
  }else{
    _oggpack_write(opb,1,1);
    for(i=0;i<vi->channels;i++)
      _oggpack_write(opb,d->floorsubmap[i],8);
  }

  for(i=1;i<vi->channels;i++)
    if(d->residuesubmap[i]!=d->residuesubmap[i-1])break;
  if(i==vi->channels){
    /* no channel submapping */
    _oggpack_write(opb,0,1);
    _oggpack_write(opb,d->residuesubmap[0],8);
  }else{
    /* channel submapping */
    _oggpack_write(opb,1,1);
    for(i=0;i<vi->channels;i++)
      _oggpack_write(opb,d->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
extern _vi_info_map *_vorbis_map0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *d=calloc(1,sizeof(vorbis_info_mapping0));
  memset(d,0,sizeof(vorbis_info_mapping0));

  d->timesubmap=_oggpack_read(opb,8);
  if(d->timesubmap>=vi->times)goto err_out;

  d->floorsubmap=malloc(sizeof(int)*vi->channels);
  if(_oggpack_read(opb,1)){
    /* channel submap */
    for(i=0;i<vi->channels;i++)
      d->floorsubmap[i]=_oggpack_read(opb,8);
  }else{
    int temp=_oggpack_read(opb,8);
    for(i=0;i<vi->channels;i++)
      d->floorsubmap[i]=temp;
  }

  d->residuesubmap=malloc(sizeof(int)*vi->channels);
  if(_oggpack_read(opb,1)){
    /* channel submap */
    for(i=0;i<vi->channels;i++)
      d->residuesubmap[i]=_oggpack_read(opb,8);
  }else{
    int temp=_oggpack_read(opb,8);
    for(i=0;i<vi->channels;i++)
      d->residuesubmap[i]=temp;
  }

  for(i=0;i<vi->channels;i++){
    if(d->floorsubmap[i]<0 || d->floorsubmap[i]>=vi->floors)goto err_out;
    if(d->residuesubmap[i]<0 || d->residuesubmap[i]>=vi->residues)
      goto err_out;
  }

  return d;

 err_out:
  _vorbis_map0_free(d);
  return(NULL);
}

#include <stdio.h>
#include "os.h"
#include "lpc.h"
#include "lsp.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"
#include "bitwise.h"
#include "spectrum.h"

/* no time mapping implementation for now */
int _vorbis_map0_analysis(int mode,vorbis_block *vb){
  vorbis_dsp_state     *vd=vb->vd;
  oggpack_buffer       *opb=&vb->opb;
  vorbis_info          *vi=vd->vi;
  vorbis_info_mapping0 *map=(vorbis_info_mapping0 *)(vi->modelist[mode]);
  int                   n=vb->pcmend;
  int i,j;

  /* time domain pre-window: NONE IMPLEMENTED */

  /* window the PCM data: takes PCM vector, vb; modifies PCM vector */

  {
    double *window=vd->window[vb->W][vb->lW][vb->nW][vi->windowtypes[mode]];
    for(i=0;i<vi->channels;i++){
      double *pcm=vb->pcm[i];
      for(j=0;j<n;j++)
	pcm[j]*=window[j];
    }
  }
	    
  /* time-domain post-window: NONE IMPLEMENTED */

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  {
    for(i=0;i<vi->channels;i++){
      double *pcm=vb->pcm[i];
      mdct_forward(vd->transform[vb->W][0],pcm,pcm);
    }
  }

  {
    double *decfloor=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    double *floor=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    double *mask=_vorbis_block_alloc(vb,n*sizeof(double)/2);

    for(i=0;i<vi->channels;i++){
      double *pcm=vb->pcm[i];
      double *pcmaux=vb->pcm[i]+n/2;
      int floorsub=map->floorsubmap[i];
      int ressub=map->residuesubmap[i];
      int psysub=map->psysubmap[i];
      
      /* perform psychoacoustics; takes PCM vector; 
	 returns two curves: the desired transform floor and the masking curve */
      memset(floor,0,sizeof(double)*n/2);
      memset(mask,0,sizeof(double)*n/2);
      _vp_mask_floor(&vd->psy[psysub],pcm,mask,floor);
 
      /* perform floor encoding; takes transform floor, returns decoded floor */
      vorbis_floor_encode_P[vi->floortypes[floorsub]]
	(vi->floorlist[floorsub],vd->floor[floorsub],pcm,floor,decfloor);

      /* perform residue prequantization */
      _vp_quantize(&vd->psy[psysub],pcm,mask,decfloor,pcmaux);

    }
  }

  /* perform residue encoding with residue mapping; this is multiplexed */
  /* multiplexing works like this: The first 

  vorbis_res_encode_P[vi->restypes[ressub]]
	(vi->reslist[ressub],vd->residue[ressub],pcm,mask,decfloor);



  return(0);
}

int _vorbis_map0_synthesis(int mode,vorbis_block *vb){



  window=vb->vd->window[vb->W][vb->lW][vb->nW];

  n=vb->pcmend=vi->blocksize[vb->W];
  
  /* No envelope encoding yet */
  _oggpack_read(opb,0,1);

  for(i=0;i<vi->channels;i++){
    double *lpc=vb->lpc[i];
    double *lsp=vb->lsp[i];
    
    /* recover the spectral envelope */
    if(_vs_spectrum_decode(vb,&vb->amp[i],lsp)<0)return(-1);
    
    /* recover the spectral residue */  
    if(_vs_residue_decode(vb,vb->pcm[i])<0)return(-1);

    /* LSP->LPC */
    vorbis_lsp_to_lpc(lsp,lpc,vl->m); 

    /* apply envelope to residue */
    
    vorbis_lpc_apply(vb->pcm[i],lpc,vb->amp[i],vl);
      

    /* MDCT->time */
    mdct_backward(&vb->vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);
    
  }
  return(0);
}


