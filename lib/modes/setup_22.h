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

 function: 22kHz settings 

 ********************************************************************/

static const float preamp_22[5]={
0.848, 0.850, 0.910, 0.963, 0.995
};

static const double rate_mapping_22[5]={
//  15000.,20000.,44000.,86000.
  14000.,16000.,20000.,44000.,86000.
};

static const double rate_mapping_22_uncoupled[5]={
//  16000.,28000.,50000.,90000.
  22000.,24000.,28000.,50000.,90000.
};

static const double _psy_lowpass_22[5]={8.5,9.5,11.,30.,99.};

static const ve_setup_data_template ve_setup_22_stereo={
//  3,
  4,
  rate_mapping_22,
  quality_mapping_16,
  preamp_22,
  2,
  19000,
  26000,

  blocksize_16_short,
  blocksize_16_long,

  _psy_tone_masteratt_16,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_16,
  _vp_tonemask_adj_16,
  _vp_tonemask_adj_16,

  _psy_noiseguards_16,
  _psy_noisebias_16_impulse,
  _psy_noisebias_16_short,
  _psy_noisebias_16_short,
  _psy_noisebias_16,
  _psy_noise_suppress,

  _psy_compand_8,
  _psy_compand_16_mapping,
  _psy_compand_16_mapping,

  {_noise_start_16,_noise_start_16},
  { _noise_part_16, _noise_part_16},
  _noise_thresh_16,

  _psy_ath_floater_16,
  _psy_ath_abs_16,

  _psy_lowpass_22,

  _psy_global_44,
  _global_mapping_16,
  _psy_stereo_modes_16,

  _floor_books,
  _floor,
  2,
  _floor_mapping_16,

  _mapres_template_16_stereo
};

static const ve_setup_data_template ve_setup_22_uncoupled={
//  3,
  4,
  rate_mapping_22_uncoupled,
  quality_mapping_16,
  preamp_22,
  -1,
  19000,
  26000,

  blocksize_16_short,
  blocksize_16_long,

  _psy_tone_masteratt_16,
  _psy_tone_0dB,
  _psy_tone_suppress,

  _vp_tonemask_adj_16,
  _vp_tonemask_adj_16,
  _vp_tonemask_adj_16,

  _psy_noiseguards_16,
  _psy_noisebias_16_impulse,
  _psy_noisebias_16_short,
  _psy_noisebias_16_short,
  _psy_noisebias_16,
  _psy_noise_suppress,

  _psy_compand_8,
  _psy_compand_16_mapping,
  _psy_compand_16_mapping,

  {_noise_start_16,_noise_start_16},
  { _noise_part_16, _noise_part_16},
  _noise_thresh_16,

  _psy_ath_floater_16,
  _psy_ath_abs_16,

  _psy_lowpass_22,

  _psy_global_44,
  _global_mapping_16,
  _psy_stereo_modes_16,

  _floor_books,
  _floor,
  2,
  _floor_mapping_16,

  _mapres_template_16_uncoupled
};
