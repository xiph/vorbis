/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: registry for time, floor, res backends and channel mappings
 last mod: $Id: registry.c,v 1.10 2001/09/07 08:42:30 cwolf Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"


/* seems like major overkill now; the backend numbers will grow into
   the infrastructure soon enough */

extern vorbis_func_time      time0_exportbundle;
extern vorbis_func_floor     floor0_exportbundle;
extern vorbis_func_floor     floor1_exportbundle;
extern vorbis_func_residue   residue0_exportbundle;
extern vorbis_func_residue   residue1_exportbundle;
extern vorbis_func_residue   residue2_exportbundle;
extern vorbis_func_mapping   mapping0_exportbundle;

vorbis_func_time      *_time_P[]={
  &time0_exportbundle,
};

vorbis_func_floor     *_floor_P[]={
  &floor0_exportbundle,
  &floor1_exportbundle,
};

vorbis_func_residue   *_residue_P[]={
  &residue0_exportbundle,
  &residue1_exportbundle,
  &residue2_exportbundle,
};

vorbis_func_mapping   *_mapping_P[]={
  &mapping0_exportbundle,
};

  /*
   * For win32 only, the following code needs to be appended to this file 
   * because the "sizeof" operator can only evaluate the sizes
   * of statically initialized arrays in the same compilation unit.
   */ 
#if defined(_MSC_VER) && defined(STANDALONE_VORBISENC_DLL)
# include "shmmap_c.h"
#endif
