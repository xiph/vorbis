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

 function: Defines the structure of the shared memory segment.

           Any new globals added to vorbis.dll which need to accessed
           by other DLLs (vorbisenc.dll) should be added here.

 created:  06-Sep-2001, Chris Wolf 

 last mod: $Id: shmmap.h,v 1.1 2001/09/11 20:16:11 cwolf Exp $
 ********************************************************************/
#ifndef _shmmap_h_
# define _shmmap_h_

#include <codec_internal.h>

#define NUMELEMENTS(x) (sizeof(x)/sizeof(x[0]))

#pragma pack(push, shared_map, 4)  // use a known structure alignment
typedef struct shared_map
{
  vorbis_func_time      **p_time_P;
  vorbis_func_floor     **p_floor_P;
  vorbis_func_residue   **p_residue_P;
  vorbis_func_mapping   **p_mapping_P;
} SHARED_MAP;
#pragma pack(pop, shared_map)

extern SHARED_MAP* g_shared_map;
extern SHARED_MAP* table_map2mem(int *len);
#endif /* _shmmap_h_ */
