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

 function: residue backend 0 implementation
 last mod: $Id: res0.h,v 1.1 2000/01/20 04:43:04 xiphmont Exp $

 ********************************************************************/

typedef struct vorbis_info_res0{
/* block-partitioned VQ coded straight residue */
  long  begin;
  long  end;

  /* way unfinished, just so you know while poking around CVS ;-) */
  int   stages;
  int  *books;
} vorbis_info_res0;

extern _vi_info_res *_vorbis_res0_dup   (_vi_info_res *source);
extern void          _vorbis_res0_free  (_vi_info_res *i);
extern void          _vorbis_res0_pack  (oggpack_buffer *opb, 
					 _vi_info_res *vi);
extern _vi_info_res *_vorbis_res0_unpack(vorbis_info *vi,
					 oggpack_buffer *opb);
