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

 function: libvorbis codec headers
 last mod: $Id: codec_internal.h,v 1.9.4.4 2001/10/20 03:00:09 xiphmont Exp $

 ********************************************************************/

#ifndef _V_CODECI_H_
#define _V_CODECI_H_

#include "envelope.h"
#include "codebook.h"

typedef struct vorbis_block_internal{
  float  **pcmdelay;  /* this is a pointer into local storage */ 
  float  ampmax;
} vorbis_block_internal;

typedef void vorbis_look_time;
typedef void vorbis_look_mapping;
typedef void vorbis_look_floor;
typedef void vorbis_look_residue;
typedef void vorbis_look_transform;

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

#include "psy.h"

#define BITTRACK_DIVISOR 16
typedef struct backend_lookup_state {
  /* local lookup storage */
  envelope_lookup        *ve; /* envelope lookup */    
  float                 **window[2][2][2]; /* block, leadin, leadout, type */
  vorbis_look_transform **transform[2];    /* block, type */
  codebook               *fullbooks;
  vorbis_look_psy_global *psy_g_look;

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

  /* encode side bitrate tracking */
  ogg_uint32_t  *bitrate_queue_actual;
  ogg_uint32_t  *bitrate_queue_binned;
  int            bitrate_queue_size;
  int            bitrate_queue_head;
  int            bitrate_bins;

  /* 0, -1, -4, -16, -n/16, -n/8, -n/4, -n/2 */
  long    bitrate_queue_bitacc[8];
  long    bitrate_queue_sampleacc[8];
  long    bitrate_queue_tail[8]; 
  long   *bitrate_queue_binacc;

  double bitrate_avgfloat;

} backend_lookup_state;

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
     blocksizes, different channel mappings (LR or M/A),
     different residue backends, etc.  Each mode consists of a
     blocksize flag and a mapping (along with the mapping setup */

  int        modes;
  int        maps;
  int        times;
  int        floors;
  int        residues;
  int        books;
  int        psys;     /* encode only */

  vorbis_info_mode       *mode_param[64];
  int                     map_type[64];
  vorbis_info_mapping    *map_param[64];
  int                     time_type[64];
  vorbis_info_time       *time_param[64];
  int                     floor_type[64];
  vorbis_info_floor      *floor_param[64];
  int                     residue_type[64];
  vorbis_info_residue    *residue_param[64];
  static_codebook        *book_param[256];

  vorbis_info_psy        *psy_param[64]; /* encode only */
  vorbis_info_psy_global *psy_g_param;

  /* detailed bitrate management setup */
  double bitrate_absolute_min_short;
  double bitrate_absolute_min_long;
  double bitrate_absolute_max_short;
  double bitrate_absolute_max_long;

  double bitrate_queue_time;
  double bitrate_queue_hardmin;
  double bitrate_queue_hardmax;
  double bitrate_queue_avgmin;
  double bitrate_queue_avgmax;

  double bitrate_avgfloat_initial; /* set by mode */
  double bitrate_avgfloat_minimum; /* set by mode */
  double bitrate_avgfloat_downslew_max;
  double bitrate_avgfloat_upslew_max;
  double bitrate_avgfloat_downhyst;
  double bitrate_avgfloat_uphyst;


  int    passlimit[32];     /* iteration limit per couple/quant pass */
  int    coupling_passes;
} codec_setup_info;

extern vorbis_look_psy_global *_vp_global_look(vorbis_info *vi);
extern void _vp_global_free(vorbis_look_psy_global *look);

#endif
