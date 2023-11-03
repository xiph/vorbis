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

 function: toplevel settings for 44.1/48kHz

 ********************************************************************/

#include "modes/floor_all.h"
#include "modes/residue_44.h"
#include "modes/psych_44.h"

static const float preamp_44[13]={
0.995, 0.995,
1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f
};
static const float preamp_48[13]={
1.f, 1.f,
1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 1.f
};

static const double rate_mapping_44_stereo[13]={
//  22500.,32000.,40000.,48000.,56000.,64000.,
  16000.,24000.,32000.,40000.,48000.,56000.,64000.,
  80000.,96000.,112000.,128000.,160000.,250001.
};

static const double quality_mapping_44[13]={
  -.2,-.1,.0,.1,.2,.3,.4,.5,.6,.7,.8,.9,1.0
};

static const int blocksize_short_44[12]={
  512,512,256,256,256,256,256,256,256,256,256,256
};
static const int blocksize_long_44[12]={
  4096,4096,2048,2048,2048,2048,2048,2048,2048,2048,2048,2048
};

static const double _psy_compand_short_mapping[13]={
  0., 0., 1., 1., 1.3, 1.5, 2., 2., 2., 2., 2., 2., 2.
};
static const double _psy_compand_long_mapping[13]={
  4., 4., 4., 4., 4.3, 4.5, 5., 5., 5., 5., 5., 5., 5.
};

static const double _global_mapping_44[13]={
  /* 1., 1., 1.5, 2., 2., 2.5, 2.7, 3.0, 3.5, 4., 4. */
// 0., 1., 1., 1.5, 2., 2., 2.5, 2.7, 3.0, 3.7, 4., 4.
  0., 1., 2., 2., 2.5, 3., 3., 3.5, 3.7, 4., 4., 5., 5. // low
//  0., 1., 2., 2., 2.5, 3., 3.2, 4.0, 4.0, 4.0, 4.5, 5., 5. // high
};

// short
static const int _floor_mapping_44a[12]={
  1,1,0,0,2,2,4,5,5,5,5,5
};
// long
static const int _floor_mapping_44b[12]={
  12,12,11,11,11,7,7,7,7,7,7,7
};
// LFE only
static const int _floor_mapping_44c[12]={
  10,10,10,10,10,10,10,10,10,10,10,10
};

static const int *_floor_mapping_44[]={
  _floor_mapping_44a,
  _floor_mapping_44b,
  _floor_mapping_44c,
};

static const ve_setup_data_template ve_setup_44_stereo={
//  11,
  12,
  rate_mapping_44_stereo,
  quality_mapping_44,
  preamp_44,
  2,
  40000,
  45000,

  blocksize_short_44,
  blocksize_long_44,

  _psy_tone_masteratt_44,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock, // impulse
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_impulse,
  _psy_noisebias_padding,
  _psy_noisebias_trans,
  _psy_noisebias_long,
  _psy_noise_suppress,

  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44,_noise_start_long_44},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,

  _psy_lowpass_44,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44,

  _floor_books,
  _floor,
  2,
  _floor_mapping_44,

  _mapres_template_44_stereo
};

static const ve_setup_data_template ve_setup_48_stereo={
//  11,
  12,
  rate_mapping_44_stereo,
  quality_mapping_44,
  preamp_48,
  2,
  45000,
  50000,

  blocksize_short_44,
  blocksize_long_44,

  _psy_tone_masteratt_44,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_otherblock, // impulse
  _vp_tonemask_adj_longblock,
  _vp_tonemask_adj_otherblock,

  _psy_noiseguards_44,
  _psy_noisebias_impulse,
  _psy_noisebias_padding,
  _psy_noisebias_trans,
  _psy_noisebias_long,
  _psy_noise_suppress,

  _psy_compand_44,
  _psy_compand_short_mapping,
  _psy_compand_long_mapping,

  {_noise_start_short_44,_noise_start_long_44},
  {_noise_part_short_44,_noise_part_long_44},
  _noise_thresh_44,

  _psy_ath_floater,
  _psy_ath_abs,

  _psy_lowpass_44,

  _psy_global_44,
  _global_mapping_44,
  _psy_stereo_modes_44,

  _floor_books,
  _floor,
  2,
  _floor_mapping_44,

  _mapres_template_44_stereo
};
