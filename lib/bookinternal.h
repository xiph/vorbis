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
 last mod: $Id: bookinternal.h,v 1.1 2000/01/12 02:03:56 xiphmont Exp $

 ********************************************************************/

#ifndef _V_INT_CODEBOOK_H_
#define _V_INT_CODEBOOK_H_

#include "vorbis/codebook.h"
#include "bitwise.h"

/* some elements in the codebook structure are assumed to be pointers
   to static/shared storage (the pointers are duped, and free below
   does not touch them.  The fields are unused by decode):

   quantlist,
   lengthlist,
   encode_tree

*/ 

codebook *vorbis_book_dup(const codebook *b);
void vorbis_book_free(codebook *b);
int vorbis_book_pack(codebook *c,oggpack_buffer *b);
codebook *vorbis_book_unpack(oggpack_buffer *b);

int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b);
int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b);
long vorbis_book_decode(codebook *book, oggpack_buffer *b);
long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b);

#endif
