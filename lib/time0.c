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

 function: time backend 0 (dummy)
 last mod: $Id: time0.c,v 1.13 2002/03/30 14:11:53 msmith Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "misc.h"

static void time0_pack (vorbis_info_time *i,oggpack_buffer *opb){
}
static vorbis_info_time *time0_unpack (vorbis_info *vi,oggpack_buffer *opb){
  return "";

}
static vorbis_info_time *time0_copy_info (vorbis_info_time *vi){
  return "";
}
static vorbis_look_time *time0_look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_time *i){
  return "";
}
static void time0_free_info(vorbis_info_time *i){
}
static void time0_free_look(vorbis_look_time *i){
}
static int time0_forward(vorbis_block *vb,vorbis_look_time *i,
		    float *in,float *out){
  return(0);
}
static int time0_inverse(vorbis_block *vb,vorbis_look_time *i,
		    float *in,float *out){
  return(0);
}

/* export hooks */
vorbis_func_time time0_exportbundle={
  &time0_pack,&time0_unpack,&time0_look,&time0_copy_info,&time0_free_info,
  &time0_free_look,&time0_forward,&time0_inverse
};
