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

 function: single-block PCM synthesis
 last mod: $Id: synthesis.c,v 1.16 2000/06/15 09:18:34 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include "vorbis/codec.h"
#include "registry.h"
#include "bitwise.h"
#include "misc.h"

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  vorbis_dsp_state *vd=vb->vd;
  vorbis_info      *vi=vd->vi;
  oggpack_buffer   *opb=&vb->opb;
  int              type,mode,i;
 
  /* first things first.  Make sure decode is ready */
  _vorbis_block_ripcord(vb);
  _oggpack_readinit(opb,op->packet,op->bytes);

  /* Check the packet type */
  if(_oggpack_read(opb,1)!=0){
    /* Oops.  This is not an audio data packet */
    return(-1);
  }

  /* read our mode and pre/post windowsize */
  mode=_oggpack_read(opb,vd->modebits);
  if(mode==-1)return(-1);
  
  vb->mode=mode;
  vb->W=vi->mode_param[mode]->blockflag;
  if(vb->W){
    vb->lW=_oggpack_read(opb,1);
    vb->nW=_oggpack_read(opb,1);
    if(vb->nW==-1)   return(-1);
  }else{
    vb->lW=0;
    vb->nW=0;
  }
  
  /* more setup */
  vb->frameno=op->frameno;
  vb->sequence=op->packetno-3; /* first block is third packet */
  vb->eofflag=op->e_o_s;

  /* alloc pcm passback storage */
  vb->pcmend=vi->blocksizes[vb->W];
  vb->pcm=_vorbis_block_alloc(vb,sizeof(double *)*vi->channels);
  for(i=0;i<vi->channels;i++)
    vb->pcm[i]=_vorbis_block_alloc(vb,vb->pcmend*sizeof(double));

  /* unpack_header enforces range checking */
  type=vi->map_type[vi->mode_param[mode]->mapping];

  return(_mapping_P[type]->inverse(vb,vd->mode[mode]));
}


