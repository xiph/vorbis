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

 function: flexible, delayed bitpacking abstraction
 last mod: $Id: bitbuffer.h,v 1.1 2000/11/17 11:57:49 xiphmont Exp $

 ********************************************************************/

#ifndef _V_BITBUF_
#define _V_BITBUF_

#include "codebook.h"

#define _VBB_ALLOCSIZE 128
typedef struct vorbis_bitbuffer_chain{
  ogg_uint32_t             words[_VBB_ALLOCSIZE];
  int                      bits[_VBB_ALLOCSIZE];
  struct vorbis_bitbuffer_chain *next;
} vorbis_bitbuffer_chain;

typedef struct vorbis_bitbuffer{
  long                    ptr;
  vorbis_bitbuffer_chain *first;
  vorbis_bitbuffer_chain *last;
  vorbis_block           *vb;
} vorbis_bitbuffer;

void bitbuf_init(vorbis_bitbuffer *vbb,vorbis_block *vb);
extern void bitbuf_write(vorbis_bitbuffer *vbb,unsigned long word,int length);
extern void bitbuf_pack(oggpack_buffer *dest,vorbis_bitbuffer *source);


extern int vorbis_book_bufencode(codebook *book, int a, vorbis_bitbuffer *b);

#endif
