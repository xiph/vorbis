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

 function: floor backend 0 implementation
 last mod: $Id: floor0.c,v 1.1 2000/01/20 04:42:55 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"
#include "floor0.h"

extern _vi_info_floor *_vorbis_floor0_dup(_vi_info_floor *source){
  vorbis_info_floor0 *d=malloc(sizeof(vorbis_info_floor0));
  memcpy(d,source,sizeof(vorbis_info_floor0));
  if(d->stages){
    d->books=malloc(sizeof(int)*d->stages);
    memcpy(d->books,((vorbis_info_floor0 *)source)->books,
	   sizeof(int)*d->stages);
  }
  return(d);
}

extern void _vorbis_floor0_free(_vi_info_floor *i){
  vorbis_info_floor0 *d=(vorbis_info_floor0 *)i;
  if(d){
    if(d->books)free(d->books);
    memset(i,0,sizeof(vorbis_info_floor0));
  }
}

extern void _vorbis_floor0_pack(oggpack_buffer *opb,_vi_info_floor *vi){
  vorbis_info_floor0 *d=(vorbis_info_floor0 *)vi;
  int i;
  _oggpack_write(opb,d->order,8);
  _oggpack_write(opb,d->rate,16);
  _oggpack_write(opb,d->barkmap,16);
  _oggpack_write(opb,d->stages,8);
  for(i=0;i<d->stages;i++)
    _oggpack_write(opb,d->books[i],8);
}

extern _vi_info_floor *_vorbis_floor0_unpack(vorbis_info *vi,
					     oggpack_buffer *opb){
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
  for(i=0;i<d.stages;i++){
    d.books[i]=_oggpack_read(opb,8);
    if(d.books[i]<0 || d.books[i]>=vi->books)return(NULL);
  }
  return(_vorbis_floor0_dup(&d));
}
