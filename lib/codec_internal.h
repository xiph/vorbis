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

 function: libvorbis codec headers
 last mod: $Id: codec_internal.h,v 1.3 2000/11/17 11:47:18 xiphmont Exp $

 ********************************************************************/

#ifndef _V_CODECI_H_
#define _V_CODECI_H_

#include "envelope.h"
#include "codebook.h"
#include "psy.h"
#include "bitbuffer.h"

typedef void vorbis_look_time;
typedef void vorbis_look_mapping;
typedef void vorbis_look_floor;
typedef void vorbis_look_residue;
typedef void vorbis_look_transform;

typedef struct backend_lookup_state {
  /* local lookup storage */
  envelope_lookup        *ve; /* envelope lookup */    
  float                 **window[2][2][2]; /* block, leadin, leadout, type */
  vorbis_look_transform **transform[2];    /* block, type */
  codebook               *fullbooks;

  /* backend lookups are tied to the mode, not the backend or naked mapping */
  int                     modebits;
  vorbis_look_mapping   **mode;

  /* local storage, only used on the encoding side.  This way the
     application does not need to worry about freeing some packets'
     memory and not others'; packet storage is always tracked.
     Cleared next call to a _dsp_ function */
  unsigned char *header;
  unsigned char *header1;
  unsigned char *header2;

} backend_lookup_state;

/* mode ************************************************************/
typedef struct {
  int blockflag;
  int windowtype;
  int transformtype;
  int mapping;
} vorbis_info_mode;

typedef void vorbis_info_time;
typedef void vorbis_info_floor;
typedef void vorbis_info_residue;
typedef void vorbis_info_mapping;

/* vorbis_info contains all the setup information specific to the
   specific compression/decompression mode in progress (eg,
   psychoacoustic settings, channel setup, options, codebook
   etc).  
*********************************************************************/

typedef struct codec_setup_info {

  /* Vorbis supports only short and long blocks, but allows the
     encoder to choose the sizes */

  long blocksizes[2];

  /* modes are the primary means of supporting on-the-fly different
     blocksizes, different channel mappings (LR or mid-side),
     different residue backends, etc.  Each mode consists of a
     blocksize flag and a mapping (along with the mapping setup */

  int        modes;
  int        maps;
  int        times;
  int        floors;
  int        residues;
  int        books;
  int        psys;     /* encode only */

  vorbis_info_mode    *mode_param[64];
  int                  map_type[64];
  vorbis_info_mapping *map_param[64];
  int                  time_type[64];
  vorbis_info_time    *time_param[64];
  int                  floor_type[64];
  vorbis_info_floor   *floor_param[64];
  int                  residue_type[64];
  vorbis_info_residue *residue_param[64];
  static_codebook     *book_param[256];
  vorbis_info_psy     *psy_param[64]; /* encode only */
  
  /* for block long/sort tuning; encode only */
  int        envelopesa;
  float     preecho_thresh;
  float     preecho_clamp;
  float     preecho_minenergy;

} codec_setup_info;

#endif
