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

 function: basic codebook pack/unpack/code/decode operations
 last mod: $Id: bookinternal.h,v 1.4 2000/01/22 13:28:16 xiphmont Exp $

 ********************************************************************/

#ifndef _V_INT_CODEBOOK_H_
#define _V_INT_CODEBOOK_H_

#include "vorbis/codebook.h"
#include "bitwise.h"

extern void vorbis_book_finish(codebook *dest,const static_codebook *source);
extern void vorbis_book_clear(codebook *b);
extern int vorbis_book_pack(codebook *c,oggpack_buffer *b);
extern int vorbis_book_unpack(oggpack_buffer *b,codebook *c);

extern int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);
extern int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b);
extern long vorbis_book_decode(codebook *book, oggpack_buffer *b);
extern long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b);

#endif
