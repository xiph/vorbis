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

 function: pack/unpack/dup/clear the various time info storage
 last mod: $Id: infotime.c,v 1.1 2000/01/19 08:57:56 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"

/* hadlers for time backend 0 (dummy) *********************************/
static void *_vorbis_time0_dup(void *source){
  return(calloc(1,sizeof(vorbis_info_time0)));
}

static void _vorbis_time0_free(void *i){
  if(i)free(i);
}

static void _vorbis_time0_pack(oggpack_buffer *opb,void *i){
}  

static void *_vorbis_time0_unpack(oggpack_buffer *opb){
  return(_vorbis_time0_dup(NULL));
}

/* stuff em into arrays ************************************************/
#define VI_TIMEB 1

static void *(*vorbis_time_dup_P[])(void *)={ 
  &_vorbis_time0_dup,
};

static void (*vorbis_time_free_P[])(void *)={ 
  &_vorbis_time0_free,
};

static void (*vorbis_time_pack_P[])(oggpack_buffer *,void *)={ 
  &_vorbis_time0_pack,
};

static void *(*vorbis_time_unpack_P[])(oggpack_buffer *)={ 
  &_vorbis_time0_unpack,
};

