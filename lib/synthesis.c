/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: single-block PCM synthesis
 last mod: $Id: synthesis.c,v 1.20 2001/02/02 03:51:58 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "registry.h"
#include "misc.h"
#include "os.h"

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  vorbis_dsp_state     *vd=vb->vd;
  backend_lookup_state *b=vd->backend_state;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  oggpack_buffer       *opb=&vb->opb;
  int                   type,mode,i;
 
  /* first things first.  Make sure decode is ready */
  _vorbis_block_ripcord(vb);
  oggpack_readinit(opb,op->packet,op->bytes);

  /* Check the packet type */
  if(oggpack_read(opb,1)!=0){
    /* Oops.  This is not an audio data packet */
    return(OV_ENOTAUDIO);
  }

  /* read our mode and pre/post windowsize */
  mode=oggpack_read(opb,b->modebits);
  if(mode==-1)return(OV_EBADPACKET);
  
  vb->mode=mode;
  vb->W=ci->mode_param[mode]->blockflag;
  if(vb->W){
    vb->lW=oggpack_read(opb,1);
    vb->nW=oggpack_read(opb,1);
    if(vb->nW==-1)   return(OV_EBADPACKET);
  }else{
    vb->lW=0;
    vb->nW=0;
  }
  
  /* more setup */
  vb->granulepos=op->granulepos;
  vb->sequence=op->packetno-3; /* first block is third packet */
  vb->eofflag=op->e_o_s;

  /* alloc pcm passback storage */
  vb->pcmend=ci->blocksizes[vb->W];
  vb->pcm=_vorbis_block_alloc(vb,sizeof(float *)*vi->channels);
  for(i=0;i<vi->channels;i++)
    vb->pcm[i]=_vorbis_block_alloc(vb,vb->pcmend*sizeof(float));

  /* unpack_header enforces range checking */
  type=ci->map_type[ci->mode_param[mode]->mapping];

  return(_mapping_P[type]->inverse(vb,b->mode[mode]));
}


