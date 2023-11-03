/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2009             *
 * by the Xiph.Org Foundation https://xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: random psychoacoustics (not including preecho)

 ********************************************************************/

#ifndef _V_PSY_H_
#define _V_PSY_H_
#include "smallft.h"

#include "backends.h"
#include "envelope.h"

#ifndef EHMER_MAX
#define EHMER_MAX 56
#endif

/* psychoacoustic setup ********************************************/
#define P_BANDS 17      /* 62Hz to 16kHz */
#define P_LEVELS 8      /* 30dB to 100dB */
#define P_LEVEL_0 30.    /* 30 dB */
#define P_NOISECURVES 3

#define NOISE_COMPAND_LEVELS 40
typedef struct vorbis_info_psy{
  int   blockflag;

  float ath_adjatt;
  float ath_maxatt;

  float tone_masteratt[P_NOISECURVES];
  float tone_centerboost;
  float tone_decay;
  float tone_abs_limit;
  float toneatt[P_BANDS];

  int noisemaskp;
  float noisemaxsupp;
  float noisewindowlo;
  float noisewindowhi;
  int   noisewindowlomin;
  int   noisewindowhimin;
  int   noisewindowfixed;
  float noiseoff[P_NOISECURVES][P_BANDS];
  float noisecompand[NOISE_COMPAND_LEVELS];
  float noisecompand_high[NOISE_COMPAND_LEVELS]; /* for aoTuV M5 */ 
  
  float flacint; /* for aoTuV M2 */

  float max_curve_dB;

  int normal_p;
  int normal_start;
  int normal_partition;
  double normal_thresh;
} vorbis_info_psy;

typedef struct{
  int   eighth_octave_lines;

  /* for block long/short tuning; encode only */
  float preecho_thresh[VE_BANDS];
  float postecho_thresh[VE_BANDS];
  float stretch_penalty;
  float preecho_minenergy;

  float ampmax_att_per_sec;

  /* channel coupling config */
  int   coupling_pkHz[PACKETBLOBS];
  int   coupling_pointlimit[2][PACKETBLOBS];
  int   coupling_prepointamp[PACKETBLOBS];
  int   coupling_postpointamp[PACKETBLOBS];
  int   sliding_lowpass[2][PACKETBLOBS];

} vorbis_info_psy_global;

typedef struct {
  float ampmax;
  int   channels;

  vorbis_info_psy_global *gi;
  int   coupling_pointlimit[2][P_NOISECURVES];
} vorbis_look_psy_global;


typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  float ***tonecurves;
  float **noiseoffset;

  float *ath;
  long  *octave;             /* in n.ocshift format */
  long  *bark;

  long  firstoc;
  long  shiftoc;
  int   eighth_octave_lines; /* power of two, please */
  int   total_octave_lines;
  long  rate; /* cache it */

  int m3n[4]; /* number for aoTuV M3 */
  float m_val; /* masking compensation value */
  
  int tonecomp_endp; /* for aoTuV M4 (and M3, M9) */
  float tonecomp_thres; /* threshold of aoTuV M4 */
  float *ntfix_noiseoffset; /* offset of aoTuV M7 */
  
  int min_nn_lp; /* for aoTuV M2 (and M8) */
  int endeg_start; /* for direct partial noise normalization [start] */
  int endeg_end; /* for direct partial noise normalization [end] */
  int tonefix_end; /* for aoTuV M6, M7 */
  
  int n25p; /* mdct n(25%) */
  int n33p; /* mdct n(33%) */
  int n75p; /* mdct n(75%) */
  int nn75pt; /* 75 % partition for noise  normalization */
  int nn50pt; /* half partition for noise normalization */
  int nn25pt; /* quarter partition for noise normalization */
} vorbis_look_psy;

/* typedef for aoTuV */
typedef struct{
  int tonecomp_endp;
  float tonecomp_thres;

  int min_nn_lp;
  int tonefix_end;
}aotuv_preset;

typedef struct {
  int sw;
  int mdctbuf_flag;
  float noise_rate;
  float noise_rate_low;
  float noise_center;
  float tone_rate;
} local_mod3_psy;

typedef struct {
  int start;
  int end;
  int lp_pos;
  int end_block;
  float thres;
} local_mod4_psy;

extern void   _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
                           vorbis_info_psy_global *gi,int n,long rate);
extern void   _vp_psy_clear(vorbis_look_psy *p);
extern void  *_vi_psy_dup(void *source);

extern void   _vi_psy_free(vorbis_info_psy *i);
extern vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i);

extern float  _postnoise_detection(float *pcm, int nn, int mode, int lW_mode);

extern void _vp_noisemask(const vorbis_look_psy *p,
                          const float noise_compand_level,
                          const float *logmdct,
                          const float *lastmdct,
                          float *epeak,
                          float *npeak,
                          float *logmask,
                          float poste,
                          int block_mode);

extern void _vp_tonemask(const vorbis_look_psy *p,
                         const float *logfft,
                         float *logmask,
                         const float global_specmax,
                         const float local_specmax);

extern void _vp_offset_and_mix(const vorbis_look_psy *p,
                               const float *noise,
                               const float *tone,
                               const int offset_select,
                               const int bit_managed,
                               float *logmask,
                               float *mdct,
                               float *logmdct,
                               float *lastmdct, float *tempmdct,
                               float low_compand,
                               float *npeak,
                               const int end_block,
                               const int block_mode,
                               const int nW_modenumber,
                               const int lW_block_mode,
                               const int lW_no, const int impadnum);

extern float _vp_ampmax_decay(float amp, const vorbis_dsp_state *vd);

extern void _vp_couple_quantize_normalize(int blobno,
                                          vorbis_info_psy_global *g,
                                          vorbis_look_psy *p,
                                          vorbis_info_mapping0 *vi,
                                          float **mdct,
                                          float **enpeak,
                                          float **nepeak,
                                          int   **iwork,
                                          int    *nonzero,
                                          int     sliding_lowpass,
                                          int     ch,
                                          int     lowpassr);

extern float lb_loudnoise_fix(const vorbis_look_psy *p,
                              float noise_compand_level,
                              const float *logmdct,
                              const int block_mode,
                              const int lW_block_mode);

#endif
