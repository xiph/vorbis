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

 function: random psychoacoustics (not including preecho)
 last mod: $Id: psy.h,v 1.16 2000/11/06 00:07:02 xiphmont Exp $

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

typedef struct {
  int n;
  struct vorbis_info_psy *vi;

  float ***tonecurves;
  float **peakatt;
  float **noiseatt;

  float *ath;
  int    *octave;
  float *bark;

} vorbis_look_psy;

extern void   _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate);
extern void   _vp_psy_clear(vorbis_look_psy *p);
extern void  *_vi_psy_dup(void *source);

extern void   _vi_psy_free(vorbis_info_psy *i);
extern vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i);

extern void   _vp_compute_mask(vorbis_look_psy *p,float *f, 
			       float *floor,
			       float *decay);
extern void _vp_apply_floor(vorbis_look_psy *p,float *f,float *flr);

#endif


