/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: registry for time, floor, res backends and channel mappings
 last mod: $Id: registry.h,v 1.11 2001/12/20 01:00:29 segher Exp $

 ********************************************************************/

#ifndef _V_REG_H_
#define _V_REG_H_

#define VI_TRANSFORMB 1
#define VI_WINDOWB 1
#define VI_TIMEB 1
#define VI_FLOORB 2
#define VI_RESB 3
#define VI_MAPB 1

#if defined(_WIN32) && defined(VORBISDLL_IMPORT)
# define EXTERN __declspec(dllimport) extern
#else
# define EXTERN extern
#endif

EXTERN vorbis_func_time      *_time_P[];
EXTERN vorbis_func_floor     *_floor_P[];
EXTERN vorbis_func_residue   *_residue_P[];
EXTERN vorbis_func_mapping   *_mapping_P[];

#endif
