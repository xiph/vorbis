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

 function: time backend 0 (dummy)
 last mod: $Id: time0.c,v 1.1 2000/01/20 04:43:05 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "registry.h"
#include "time0.h"

extern _vi_info_time *_vorbis_time0_dup(_vi_info_time *source){
  return(calloc(1,sizeof(vorbis_info_time0)));
}

extern void _vorbis_time0_free(_vi_info_time *i){
  if(i)free(i);
}

extern void _vorbis_time0_pack(oggpack_buffer *opb,_vi_info_time *i){
}  

extern _vi_info_time *_vorbis_time0_unpack(vorbis_info *vi,
					   oggpack_buffer *opb){
  return(_vorbis_time0_dup(NULL));
}
