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

 function: registry for time, floor, res backends and channel mappings
 last mod: $Id: registry.c,v 1.1 2000/01/20 04:43:04 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "time0.h"
#include "floor0.h"
#include "res0.h"
#include "mapping0.h"
#include "registry.h"

/* time backend registry */
void *(*vorbis_time_dup_P[])(void *)={ 
  &_vorbis_time0_dup,
};

void (*vorbis_time_free_P[])(void *)={ 
  &_vorbis_time0_free,
};

void (*vorbis_time_pack_P[])(oggpack_buffer *,void *)={ 
  &_vorbis_time0_pack,
};

void *(*vorbis_time_unpack_P[])(oggpack_buffer *)={ 
  &_vorbis_time0_unpack,
};

/* floor backend registry */
void *(*vorbis_floor_dup_P[])(void *)={ 
  &_vorbis_floor0_dup,
};

void (*vorbis_floor_free_P[])(void *)={ 
  &_vorbis_floor0_free,
};

void (*vorbis_floor_pack_P[])(oggpack_buffer *,void *)={ 
  &_vorbis_floor0_pack,
};

void *(*vorbis_floor_unpack_P[])(oggpack_buffer *)={ 
  &_vorbis_floor0_unpack,
};

/* residue backend registry */
void *(*vorbis_res_dup_P[])(void *)={ 
  &_vorbis_res0_dup,
};

void (*vorbis_res_free_P[])(void *)={ 
  &_vorbis_res0_free,
};

void (*vorbis_res_pack_P[])(oggpack_buffer *,void *)={ 
  &_vorbis_res0_pack,
};

void *(*vorbis_res_unpack_P[])(oggpack_buffer *)={ 
  &_vorbis_res0_unpack,
};

/* channel mapping registry */
void *(*vorbis_map_dup_P[])(void *)={ 
  &_vorbis_map0_dup,
};

void (*vorbis_map_free_P[])(void *)={ 
  &_vorbis_map0_free,
};

void (*vorbis_map_pack_P[])(oggpack_buffer *,void *)={ 
  &_vorbis_map0_pack,
};

void *(*vorbis_map_unpack_P[])(oggpack_buffer *)={ 
  &_vorbis_map0_unpack,
};

void *(*vorbis_map_analysis_P[])(vorbis_block *vb,void *,ogg_packet *)={ 
  &_vorbis_map0_analysis,
};

void *(*vorbis_map_synthesis_P[])(vorbis_block *vb,void *,ogg_packet *)={ 
  &_vorbis_map0_synthesis,
};
