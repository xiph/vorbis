/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: registry for time, floor, res backends and channel mappings
 last mod: $Id: registry.c,v 1.4 2000/11/06 00:07:02 xiphmont Exp $

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

