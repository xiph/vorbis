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
 last mod: $Id: registry.c,v 1.3 2000/03/10 13:21:18 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "registry.h"
#include "misc.h"

/* seems like major overkill now; the backend numbers will grow into
   the infrastructure soon enough */

extern vorbis_func_time      time0_exportbundle;
extern vorbis_func_floor     floor0_exportbundle;
extern vorbis_func_residue   residue0_exportbundle;
extern vorbis_func_mapping   mapping0_exportbundle;

vorbis_func_time      *_time_P[]={
  &time0_exportbundle,
};

vorbis_func_floor     *_floor_P[]={
  &floor0_exportbundle,
};

vorbis_func_residue   *_residue_P[]={
  &residue0_exportbundle,
};

vorbis_func_mapping   *_mapping_P[]={
  &mapping0_exportbundle,
};

