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

 function: basic shared codebook operations
 last mod: $Id: sharedbook.h,v 1.2.8.1 2000/08/31 09:00:02 xiphmont Exp $

 ********************************************************************/

#ifndef _V_INT_SHCODEBOOK_H_
#define _V_INT_SHCODEBOOK_H_

#include "vorbis/codebook.h"

extern void vorbis_staticbook_clear(static_codebook *b);
extern int vorbis_book_init_encode(codebook *dest,const static_codebook *source);
extern int vorbis_book_init_decode(codebook *dest,const static_codebook *source);
extern void vorbis_book_clear(codebook *b);

extern float *_book_unquantize(const static_codebook *b);
extern float *_book_logdist(const static_codebook *b,float *vals);
extern float _float32_unpack(long val);
extern long   _float32_pack(float val);
extern int  _best(codebook *book, float *a, int step);
extern int _ilog(unsigned int v);
extern long _book_maptype1_quantvals(const static_codebook *b);

extern int vorbis_book_besterror(codebook *book,float *a,int step,int addmul);
extern long vorbis_book_codeword(codebook *book,int entry);
extern long vorbis_book_codelen(codebook *book,int entry);



#endif
