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

 function: random psychoacoustics (not including preecho)
 last mod: $Id: psy.h,v 1.19 2001/02/26 03:50:43 xiphmont Exp $

 ********************************************************************/

#ifndef _V_PSY_H_
#define _V_PSY_H_
#include "smallft.h"

#ifndef EHMER_MAX
#define EHMER_MAX 56
#endif

/* psychoacoustic setup ********************************************/
#define MAX_BARK 27
#define P_BANDS 17
#define P_LEVELS 11
typedef struct vorbis_info_psy{
  int    athp;
  int    decayp;

  float  ath_adjatt;
  float  ath_maxatt;

  int   eighth_octave_lines;

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  int tonemaskp;
  float toneatt[P_BANDS][P_LEVELS];

  int peakattp;
  float peakatt[P_BANDS][P_LEVELS];

  int noisemaskp;
  float noisemaxsupp;
  float noisewindowlo;
  float noisewindowhi;
  int   noisewindowlomin;
  int   noisewindowhimin;
  float noisemedian[P_BANDS*2];

  float max_curve_dB;
  float bound_att_dB;

  /* decay setup */
  float attack_coeff;
  float decay_coeff;
} vorbis_info_psy;

typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  float ***tonecurves;
  float **peakatt;
  float *noisemedian;
  float *noiseoffset;

  float *ath;
  long  *octave;             /* in n.ocshift format */
  float *bark;

  long  firstoc;
  long  shiftoc;
  int   eighth_octave_lines; /* power of two, please */
  int   total_octave_lines;  

} vorbis_look_psy;

extern void   _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate);
extern void   _vp_psy_clear(vorbis_look_psy *p);
extern void  *_vi_psy_dup(void *source);

extern void   _vi_psy_free(vorbis_info_psy *i);
extern vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i);

extern float  _vp_compute_mask(vorbis_look_psy *p,
			       float *fft, 
			       float *mdct, 
			       float *floor,
			       float *decay,
			       float prev_maxamp);
extern void _vp_apply_floor(vorbis_look_psy *p,float *f,float *flr);
extern float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd);

#endif


