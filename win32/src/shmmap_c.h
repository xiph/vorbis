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

 function: Creates an image of strutures and mappings in registry.c
           to be copied into shared memory segment via DllMain().
           See: dllmain.c

 created:  06-Sep-2001, Chris Wolf 

 last mod: $Id: shmmap_c.h,v 1.1 2001/09/11 20:16:11 cwolf Exp $

 This module gets conditionally appended to the end of registry.c
 ********************************************************************/
#include <malloc.h>
#include <memory.h>

#include <shmmap.h>

/**
 * Create image of mappings defined in regsitry.c.
 *
 * PARAM  len: pointer to callers length indicator, will indicate
 *             size of shared map.
 * RETURN SHARED_MAP: pointer to allocated mapping image, NULL on failure.
 */
SHARED_MAP* table_map2mem(int *len)
{
  SHARED_MAP* map;
  int p;
  int size;

  if ((map = calloc(1, sizeof(SHARED_MAP))) == (SHARED_MAP*)0)
    return (SHARED_MAP*)0;

  size =         sizeof(SHARED_MAP);

  if ((map->p_time_P = calloc(NUMELEMENTS(_time_P), 
                 sizeof(vorbis_func_time*))) == (vorbis_func_time**)0)
    return (SHARED_MAP*)0;

  size += sizeof(_time_P);
  
  for (p=0; p<NUMELEMENTS(_time_P); p++)
  {
    if ((map->p_time_P[p] = calloc(NUMELEMENTS(_time_P), 
                 sizeof(vorbis_func_time))) == (vorbis_func_time*)0)
      return (SHARED_MAP*)0;

    (void)memcpy((void *)map->p_time_P[p], 
                 (const void *)_time_P[p], 
                 sizeof(vorbis_func_time));

    size +=      sizeof(vorbis_func_time);
  }

  if ((map->p_floor_P = calloc(NUMELEMENTS(_floor_P), 
                 sizeof(vorbis_func_floor*))) == (vorbis_func_floor**)0)
    return (SHARED_MAP*)0;

  size +=        sizeof(_floor_P);
  
  for (p=0; p<NUMELEMENTS(_floor_P); p++)
  {
    if ((map->p_floor_P[p] = calloc(1, 
                 sizeof(vorbis_func_floor))) == (vorbis_func_floor*)0)
      return (SHARED_MAP*)0;

    (void)memcpy((void *)map->p_floor_P[p], 
                 (const void *)_floor_P[p], 
                 sizeof(vorbis_func_floor));

    size +=      sizeof(vorbis_func_floor);
  }

  if ((map->p_residue_P = calloc(NUMELEMENTS(_residue_P), 
                 sizeof(vorbis_func_residue*))) == (vorbis_func_residue**)0)
  size +=        sizeof(_residue_P);

  for (p=0; p<NUMELEMENTS(_residue_P); p++)
  {
    if ((map->p_residue_P[p] = calloc(1, 
                 sizeof(vorbis_func_residue))) == (vorbis_func_residue*)0)
      return (SHARED_MAP*)0;

    (void)memcpy((void *)map->p_residue_P[p], 
                 (const void *)_residue_P[p], 
                 sizeof(vorbis_func_residue));

    size +=      sizeof(vorbis_func_residue);
  }

  if ((map->p_mapping_P = calloc(NUMELEMENTS(_mapping_P), 
                 sizeof(vorbis_func_mapping*))) == (vorbis_func_mapping**)0)
      return (SHARED_MAP*)0;

  size +=        sizeof(_mapping_P);

  for (p=0; p<NUMELEMENTS(_mapping_P); p++)
  {
    if ((map->p_mapping_P[p] = calloc(1, 
                 sizeof(vorbis_func_mapping))) == (vorbis_func_mapping*)0)
      return (SHARED_MAP*)0;

    (void)memcpy((void *)map->p_mapping_P[p], 
                 (const void *)_mapping_P[p], 
                 sizeof(vorbis_func_mapping));

    size +=      sizeof(vorbis_func_mapping);
  }

  *len = size;

  return (map);
}
