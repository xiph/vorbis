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

 function: pack/unpack/dup/clear the various residue backend setups
 last mod: $Id: infores.c,v 1.1 2000/01/19 08:57:56 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"

/* handlers for residue backend 0 ***********************************/

/* unfinished as of 20000118 */
static void *_vorbis_res0_dup(void *source){
  vorbis_info_res0 *d=malloc(sizeof(vorbis_info_res0));
  memcpy(d,source,sizeof(vorbis_info_res0));
  d->books=malloc(sizeof(int)*d->stages);
  memcpy(d->books,((vorbis_info_res0 *)source)->books,
	 sizeof(int)*d->stages);
  return(d); 
}

static void _vorbis_res0_free(void *i){
  vorbis_info_res0 *d=(vorbis_info_res0 *)i;
  if(d){
    if(d->books)free(d->books);
    memset(d,0,sizeof(vorbis_info_res0));
    free(d);
  }
}

/* not yet */
static void _vorbis_res0_pack(oggpack_buffer *opb, void *vi){
}

static void *_vorbis_res0_unpack(oggpack_buffer *opb){
  vorbis_info_floor0 d;
  memset(&d,0,sizeof(d));
  return(_vorbis_res0_dup(&d));
}

/* stuff em into arrays ************************************************/
#define VI_RESB 1

static void *(*vorbis_res_dup_P[])(void *)={ 
  _vorbis_res0_dup,
};

static void (*vorbis_res_free_P[])(void *)={ 
  _vorbis_res0_free,
};

static void (*vorbis_res_pack_P[])(oggpack_buffer *,void *)={ 
  _vorbis_res0_pack,
};

static void *(*vorbis_res_unpack_P[])(oggpack_buffer *)={ 
  _vorbis_res0_unpack,
};

