/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: vorbis encode-engine setup
 last mod: $Id: vorbisenc.h,v 1.8.2.1 2002/01/01 02:27:21 xiphmont Exp $

 ********************************************************************/

#ifndef _OV_ENC_H_
#define _OV_ENC_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "codec.h"

extern int vorbis_encode_init(vorbis_info *vi,
			      long channels,
			      long rate,
			      
			      long max_bitrate,
			      long nominal_bitrate,
			      long min_bitrate);

extern int vorbis_encode_setup_managed(vorbis_info *vi,
				       long channels,
				       long rate,
				       
				       long max_bitrate,
				       long nominal_bitrate,
				       long min_bitrate);
  
extern int vorbis_encode_setup_vbr(vorbis_info *vi,
				  long channels,
				  long rate,
				  
				  float /* quality level from 0. (lo) to 1. (hi) */
				  );

extern int vorbis_encode_init_vbr(vorbis_info *vi,
				  long channels,
				  long rate,
				  
				  float base_quality /* quality level from 0. (lo) to 1. (hi) */
				  );

extern int vorbis_encode_setup_init(vorbis_info *vi);

extern int vorbis_encode_ctl(vorbis_info *vi,int number,int setp,void *arg);

  typedef struct {
    int short_block_p;
    int long_block_p;
    int impulse_block_p;
  } vectl_block_arg;

#define VECTL_BLOCK                   0x1

  typedef struct {
    double short_lowpass_kHz;
    double long_lowpass_kHz;
  } vectl_lowpass_arg;

#define VECTL_PSY_LOWPASS             0x2

  typedef struct {
    int stereo_couple_p;
    int stereo_point_dB_mode;
    double stereo_point_kHz_short;
    double stereo_point_kHz_long;
  } vectl_stereo_arg;

#define VECTL_PSY_STEREO              0x3

  typedef struct {
    double ath_float_dB;
    double ath_fixed_dB;
  } vectl_ath_arg;

#define VECTL_PSY_ATH                 0x4

  typedef struct {
    double maxdB_track_decay;
  } vectl_amp_arg;

#define VECTL_PSY_AMPTRACK            0x5

  typedef struct {
    double trigger_q;
    double tonemask_q[4];
    double tonepeak_q[4];
    double noise_q[4];
  } vectl_mask_arg;

#define VECTL_PSY_MASK_Q              0x6

  typedef struct {
    int    noise_normalize_p;
    double noise_normalize_weight;
    double noise_normalize_thresh;
  } vectl_noisenorm_arg;

#define VECTL_PSY_NOISENORM           0x7

typedef struct {
  double avg_min;
  double avg_max;
  double avg_window_time;
  double avg_window_center;
  double avg_slew_downmax;
  double avg_slew_upmax;
  int    avg_noisetrack_p;

  double limit_min;
  double limit_max;
  double limit_window_time;

  int    stereo_backfill_p;
  int    residue_backfill_p;

} vectl_bitrate_arg;

#define VECTL_BITRATE                 0x100


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif


