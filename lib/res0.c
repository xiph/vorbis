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

 function: residue backend 0 implementation
 last mod: $Id: res0.c,v 1.1 2000/01/20 04:43:04 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"
#include "res0.h"

/* unfinished as of 20000118 */
extern _vi_info_res *_vorbis_res0_dup(_vi_info_res *source){
  vorbis_info_res0 *d=malloc(sizeof(vorbis_info_res0));
  memcpy(d,source,sizeof(vorbis_info_res0));
  d->books=malloc(sizeof(int)*d->stages);
  memcpy(d->books,((vorbis_info_res0 *)source)->books,
	 sizeof(int)*d->stages);
  return(d); 
}

extern void _vorbis_res0_free(_vi_info_res *i){
  vorbis_info_res0 *d=(vorbis_info_res0 *)i;
  if(d){
    if(d->books)free(d->books);
    memset(d,0,sizeof(vorbis_info_res0));
    free(d);
  }
}

/* not yet */
extern void _vorbis_res0_pack(oggpack_buffer *opb, _vi_info_res *vi){
}

extern _vi_info_res *_vorbis_res0_unpack(vorbis_info *vi,
					 oggpack_buffer *opb){
  vorbis_info_res0 d;
  memset(&d,0,sizeof(d));
  return(_vorbis_res0_dup(&d));
}
