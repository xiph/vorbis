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

 function: time backend 0 (dummy)
 last mod: $Id: time0.c,v 1.5 2000/03/10 13:21:18 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "registry.h"
#include "misc.h"

static void pack (vorbis_info_time *i,oggpack_buffer *opb){
}
static vorbis_info_time *unpack (vorbis_info *vi,oggpack_buffer *opb){
  return "";

}
static vorbis_look_time *look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_time *i){
  return "";
}
static void free_info(vorbis_info_time *i){
}
static void free_look(vorbis_look_time *i){
}
static int forward(vorbis_block *vb,vorbis_look_time *i,
		    double *in,double *out){
  return(0);
}
static int inverse(vorbis_block *vb,vorbis_look_time *i,
		    double *in,double *out){
  return(0);
}

/* export hooks */
vorbis_func_time time0_exportbundle={
  &pack,&unpack,&look,&free_info,&free_look,&forward,&inverse
};
