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

 function: single-block PCM analysis mode dispatch
 last mod: $Id: analysis.c,v 1.21 2000/01/20 04:42:51 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"

/* decides between modes, dispatches to the appropriate mapping. */
int vorbis_analysis(vorbis_block *vb,ogg_packet *op){
  vorbis_dsp_state *vd=vb->vd;
  vorbis_info      *vi=vd->vi;
  int              type;
  int              mode=0;

  vb->glue_bits=0;
  vb->time_bits=0;
  vb->floor_bits=0;
  vb->res_bits=0;

  /* first things first.  Make sure encode is ready */
  _oggpack_reset(&vb->opb);
  /* Encode the packet type */
  _oggpack_write(&vb->opb,0,1);

  /* currently lazy.  Short block dispatches to 0, long to 1. */

  if(vb->W &&vi->modes>1)mode=1;
  type=vi->mappingtypes[mode];

  /* Encode frame mode and dispatch */
  _oggpack_write(&vb->opb,mode,vd->modebits);
  if(vorbis_map_analysis_P[type](vb,vi->modelist[mode],op))
    return(-1);

  /* set up the packet wrapper */

  op->packet=_oggpack_buffer(&vb->opb);
  op->bytes=_oggpack_bytes(&vb->opb);
  op->b_o_s=0;
  op->e_o_s=vb->eofflag;
  op->frameno=vb->frameno;
  op->packetno=vb->sequence; /* for sake of completeness */

  return(0);
}
