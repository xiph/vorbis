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

 function: pack/unpack/dup/clear the various floor backend setups
 last mod: $Id: infofloor.c,v 1.1 2000/01/19 08:57:56 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"

/* calls for floor backend 0 *********************************************/
static void *_vorbis_floor0_dup(void *source){
  vorbis_info_floor0 *d=malloc(sizeof(vorbis_info_floor0));
  memcpy(d,source,sizeof(vorbis_info_floor0));
  if(d->stages){
    d->books=malloc(sizeof(int)*d->stages);
    memcpy(d->books,((vorbis_info_floor0 *)source)->books,
	   sizeof(int)*d->stages);
  }
  return(d);
}

static void _vorbis_floor0_free(void *i){
  vorbis_info_floor0 *d=(vorbis_info_floor0 *)i;
  if(d){
    if(d->books)free(d->books);
    memset(i,0,sizeof(vorbis_info_floor0));
  }
}

static void _vorbis_floor0_pack(oggpack_buffer *opb,void *vi){
  vorbis_info_floor0 *d=(vorbis_info_floor0 *)vi;
  int i;
  _oggpack_write(opb,d->order,8);
  _oggpack_write(opb,d->rate,16);
  _oggpack_write(opb,d->barkmap,16);
  _oggpack_write(opb,d->stages,8);
  for(i=0;i<d->stages;i++)
    _oggpack_write(opb,d->books[i],8);
}

/* type is read earlier (so that we know to call this type) */
static void *_vorbis_floor0_unpack(oggpack_buffer *opb){
  vorbis_info_floor0 d;
  int i;
  d.order=_oggpack_read(opb,8);
  d.rate=_oggpack_read(opb,16);
  d.barkmap=_oggpack_read(opb,16);
  d.stages=_oggpack_read(opb,8);
  
  if(d.order<1)return(NULL);
  if(d.rate<1)return(NULL);
  if(d.barkmap<1)return(NULL);
  if(d.stages<1)return(NULL);

  d.books=alloca(sizeof(int)*d.stages);
  for(i=0;i<d.stages;i++)
    d.books[i]=_oggpack_read(opb,8);
  if(d.books[d.stages-1]<0)return(NULL);
  return(_vorbis_floor0_dup(&d));
}

/* stuff em into arrays ************************************************/
#define VI_FLOORB 1

static void *(*vorbis_floor_dup_P[])(void *)={ 
  _vorbis_floor0_dup,
};

static void (*vorbis_floor_free_P[])(void *)={ 
  _vorbis_floor0_free,
};

static void (*vorbis_floor_pack_P[])(oggpack_buffer *,void *)={ 
  _vorbis_floor0_pack,
};

static void *(*vorbis_floor_unpack_P[])(oggpack_buffer *)={ 
  _vorbis_floor0_unpack,
};

