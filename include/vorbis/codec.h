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

 function: libvorbis codec headers
 last mod: $Id: codec.h,v 1.31 2000/10/12 04:26:39 jack Exp $

 ********************************************************************/

#ifndef _vorbis_codec_h_
#define _vorbis_codec_h_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define MAX_BARK 27

#include <ogg/ogg.h>
#include "vorbis/codebook.h"
#include "vorbis/backends.h"

typedef void vorbis_look_transform;
typedef void vorbis_info_time;
typedef void vorbis_look_time;
typedef void vorbis_info_floor;
typedef void vorbis_look_floor;
typedef void vorbis_echstate_floor;
typedef void vorbis_info_residue;
typedef void vorbis_look_residue;
typedef void vorbis_info_mapping;
typedef void vorbis_look_mapping;

/* mode ************************************************************/
typedef struct {
  int blockflag;
  int windowtype;
  int transformtype;
  int mapping;
} vorbis_info_mode;

/* psychoacoustic setup ********************************************/
#define P_BANDS 17
#define P_LEVELS 11
typedef struct vorbis_info_psy{
  int    athp;
  int    decayp;
  int    smoothp;

  int    noisecullp;
  float noisecull_barkwidth;

  float ath_adjatt;
  float ath_maxatt;

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  int tonemaskp;
  float toneatt[P_BANDS][P_LEVELS];

  int peakattp;
  float peakatt[P_BANDS][P_LEVELS];

  int noisemaskp;
  float noiseatt[P_BANDS][P_LEVELS];

  float max_curve_dB;

  /* decay setup */
  float attack_coeff;
  float decay_coeff;
} vorbis_info_psy;

/* vorbis_info contains all the setup information specific to the
   specific compression/decompression mode in progress (eg,
   psychoacoustic settings, channel setup, options, codebook
   etc).  
*********************************************************************/

typedef struct vorbis_info{
  int version;
  int channels;
  long rate;

  /* The below bitrate declarations are *hints*.
     Combinations of the three values carry the following implications:
     
     all three set to the same value: 
       implies a fixed rate bitstream
     only nominal set: 
       implies a VBR stream that averages the nominal bitrate.  No hard 
       upper/lower limit
     upper and or lower set: 
       implies a VBR bitstream that obeys the bitrate limits. nominal 
       may also be set to give a nominal rate.
     none set:
       the coder does not care to speculate.
  */

  long bitrate_upper;
  long bitrate_nominal;
  long bitrate_lower;

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
} vorbis_info;
 
/* vorbis_dsp_state buffers the current vorbis audio
   analysis/synthesis state.  The DSP state belongs to a specific
   logical bitstream ****************************************************/
typedef struct vorbis_dsp_state{
  int analysisp;
  vorbis_info *vi;
  int    modebits;

  float **pcm;
  float **pcmret;
  int      pcm_storage;
  int      pcm_current;
  int      pcm_returned;

  int  preextrapolate;
  int  eofflag;

  long lW;
  long W;
  long nW;
  long centerW;

  ogg_int64_t granulepos;
  ogg_int64_t sequence;

  ogg_int64_t glue_bits;
  ogg_int64_t time_bits;
  ogg_int64_t floor_bits;
  ogg_int64_t res_bits;

  /* local lookup storage */
  void                   *ve; /* envelope lookup */    
  float                **window[2][2][2]; /* block, leadin, leadout, type */
  vorbis_look_transform **transform[2];    /* block, type */
  codebook               *fullbooks;
  /* backend lookups are tied to the mode, not the backend or naked mapping */
  vorbis_look_mapping   **mode;

  /* local storage, only used on the encoding side.  This way the
     application does not need to worry about freeing some packets'
     memory and not others'; packet storage is always tracked.
     Cleared next call to a _dsp_ function */
  unsigned char *header;
  unsigned char *header1;
  unsigned char *header2;

} vorbis_dsp_state;

/* vorbis_block is a single block of data to be processed as part of
the analysis/synthesis stream; it belongs to a specific logical
bitstream, but is independant from other vorbis_blocks belonging to
that logical bitstream. *************************************************/

struct alloc_chain{
  void *ptr;
  struct alloc_chain *next;
};

typedef struct vorbis_block{
  /* necessary stream state for linking to the framing abstraction */
  float  **pcm;       /* this is a pointer into local storage */ 
  oggpack_buffer opb;
  
  long  lW;
  long  W;
  long  nW;
  int   pcmend;
  int   mode;

  int eofflag;
  ogg_int64_t granulepos;
  ogg_int64_t sequence;
  vorbis_dsp_state *vd; /* For read-only access of configuration */

  /* local storage to avoid remallocing; it's up to the mapping to
     structure it */
  void               *localstore;
  long                localtop;
  long                localalloc;
  long                totaluse;
  struct alloc_chain *reap;

  /* bitmetrics for the frame */
  long glue_bits;
  long time_bits;
  long floor_bits;
  long res_bits;

} vorbis_block;


/* vorbis_info contains all the setup information specific to the
   specific compression/decompression mode in progress (eg,
   psychoacoustic settings, channel setup, options, codebook
   etc). vorbis_info and substructures are in backends.h.
*********************************************************************/

/* the comments are not part of vorbis_info so that vorbis_info can be
   static storage */
typedef struct vorbis_comment{
  /* unlimited user comment fields.  libvorbis writes 'libvorbis'
     whatever vendor is set to in encode */
  char **user_comments;
  int   *comment_lengths;
  int    comments;
  char  *vendor;

} vorbis_comment;


/* libvorbis encodes in two abstraction layers; first we perform DSP
   and produce a packet (see docs/analysis.txt).  The packet is then
   coded into a framed OggSquish bitstream by the second layer (see
   docs/framing.txt).  Decode is the reverse process; we sync/frame
   the bitstream and extract individual packets, then decode the
   packet back into PCM audio.

   The extra framing/packetizing is used in streaming formats, such as
   files.  Over the net (such as with UDP), the framing and
   packetization aren't necessary as they're provided by the transport
   and the streaming layer is not used */

/* Vorbis PRIMITIVES: general ***************************************/

extern void     vorbis_info_init(vorbis_info *vi);
extern void     vorbis_info_clear(vorbis_info *vi);
extern void     vorbis_comment_init(vorbis_comment *vc);
extern void     vorbis_comment_add(vorbis_comment *vc, char *comment); 
extern void     vorbis_comment_add_tag(vorbis_comment *vc, 
				       char *tag, char *contents);
extern char    *vorbis_comment_query(vorbis_comment *vc, char *tag, int count);
extern int      vorbis_comment_query_count(vorbis_comment *vc, char *tag);
extern void     vorbis_comment_clear(vorbis_comment *vc);

extern int      vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb);
extern int      vorbis_block_clear(vorbis_block *vb);
extern void     vorbis_dsp_clear(vorbis_dsp_state *v);

/* Vorbis PRIMITIVES: analysis/DSP layer ****************************/

extern int      vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi);
extern int      vorbis_analysis_headerout(vorbis_dsp_state *v,
					  vorbis_comment *vc,
					  ogg_packet *op,
					  ogg_packet *op_comm,
					  ogg_packet *op_code);
extern float  **vorbis_analysis_buffer(vorbis_dsp_state *v,int vals);
extern int      vorbis_analysis_wrote(vorbis_dsp_state *v,int vals);
extern int      vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb);
extern int      vorbis_analysis(vorbis_block *vb,ogg_packet *op);

/* Vorbis PRIMITIVES: synthesis layer *******************************/
extern int      vorbis_synthesis_headerin(vorbis_info *vi,vorbis_comment *vc,
					  ogg_packet *op);

extern int      vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi);
extern int      vorbis_synthesis(vorbis_block *vb,ogg_packet *op);
extern int      vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb);
extern int      vorbis_synthesis_pcmout(vorbis_dsp_state *v,float ***pcm);
extern int      vorbis_synthesis_read(vorbis_dsp_state *v,int samples);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

