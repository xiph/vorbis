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

 function: pack/unpack/dup/clear the various channel mapping setups
 last mod: $Id: infomap.c,v 1.1 2000/01/19 08:57:56 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"

/* these modules were split out only to make this file more readable.
   I don't want to expose the symbols */
#include "infotime.c"
#include "infofloor.c"
#include "infores.c"
#include "infopsy.c"

/* Handlers for mapping 0 *******************************************/
static void *_vorbis_map0_dup(void *source){
  vorbis_info_mapping0 *d=malloc(sizeof(vorbis_info_mapping0));
  vorbis_info_mapping0 *s=(vorbis_info_mapping0 *)source;
  memcpy(d,s,sizeof(vorbis_info_mapping0));

  if(d->timetype<0  || d->timetype>=VI_TIMEB)goto err_out;
  if(d->floortype<0 || d->floortype>=VI_FLOORB)goto err_out;
  if(d->restype<0   || d->restype>=VI_RESB)goto err_out;
      
  d->time=vorbis_time_dup_P[d->timetype](s->time);
  d->floor=vorbis_floor_dup_P[d->floortype](s->floor);
  d->res=vorbis_res_dup_P[d->restype](s->res);
  d->psy=vorbis_psy_dup(s->psy);

  return(d);
err_out:
  memset(d,0,sizeof(vorbis_info_mapping0));
  free(d);
  return(NULL);
}

static void _vorbis_map0_free(void *i){
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)i;

  if(d){
    if(d->timetype>=0  && d->timetype<VI_TIMEB)
      vorbis_time_free_P[d->timetype](d->time);
    if(d->floortype>=0 && d->floortype<VI_FLOORB)
      vorbis_floor_free_P[d->floortype](d->floor);
    if(d->restype>=0   && d->restype<VI_RESB)
      vorbis_res_free_P[d->restype](d->res);
    vorbis_psy_free(d->psy);
   
    memset(d,0,sizeof(vorbis_info_mapping0));
    free(d);
  }
}

static void _vorbis_map0_pack(oggpack_buffer *opb,void *i){
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)i;
  _oggpack_write(opb,d->timetype,8);
  _oggpack_write(opb,d->floortype,8);
  _oggpack_write(opb,d->restype,8);
  
  vorbis_time_pack_P[d->timetype](opb,d->time);
  vorbis_floor_pack_P[d->floortype](opb,d->floor);
  vorbis_res_pack_P[d->restype](opb,d->res);
}

static void *_vorbis_map0_unpack(oggpack_buffer *opb){
  vorbis_info_mapping0 d;
  memset(&d,0,sizeof(d));

  d.timetype=_oggpack_read(opb,8);
  d.floortype=_oggpack_read(opb,8);
  d.restype=_oggpack_read(opb,8);

  if(d.timetype<0  || d.timetype>=VI_TIMEB)goto err_out;
  if(d.floortype<0 || d.floortype>=VI_FLOORB)goto err_out;
  if(d.restype<0   || d.restype>=VI_RESB)goto err_out;

  d.time=vorbis_time_unpack_P[d.timetype](opb);
  d.floor=vorbis_floor_unpack_P[d.floortype](opb);
  d.res=vorbis_res_unpack_P[d.restype](opb);
  d.psy=NULL;

  return _vorbis_map0_dup(&d);

 err_out:
  /* the null check protects against type out of range */
  if(d.time)vorbis_time_free_P[d.timetype](d.time);
  if(d.floor)vorbis_floor_free_P[d.floortype](d.floor);
  if(d.res)vorbis_res_free_P[d.restype](d.res);
  
  return(NULL);
}

/* stuff em into arrays ************************************************/
#define VI_MAPB 1

static void *(*vorbis_map_dup_P[])(void *)={ 
  _vorbis_map0_dup,
};

static void (*vorbis_map_free_P[])(void *)={ 
  _vorbis_map0_free,
};

static void (*vorbis_map_pack_P[])(oggpack_buffer *,void *)={ 
  _vorbis_map0_pack,
};

static void *(*vorbis_map_unpack_P[])(oggpack_buffer *)={ 
  _vorbis_map0_unpack,
};

