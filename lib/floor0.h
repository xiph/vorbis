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

 function: floor backend 0 implementation
 last mod: $Id: floor0.h,v 1.1 2000/01/20 04:42:55 xiphmont Exp $

 ********************************************************************/

typedef struct vorbis_info_floor0{
  int   order;
  long  rate;
  long  barkmap;

  int   stages;
  int  *books;
} vorbis_info_floor0;

extern _vi_info_floor *_vorbis_floor0_dup(_vi_info_floor *source);
extern void            _vorbis_floor0_free(_vi_info_floor *vi);
extern void            _vorbis_floor0_pack(oggpack_buffer *opb,
					   _vi_info_floor *vi);
extern _vi_info_floor *_vorbis_floor0_unpack(vorbis_info *vi,
					     oggpack_buffer *opb);

