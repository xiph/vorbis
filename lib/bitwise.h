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

  function: packing variable sized words into an octet stream
  last mod: $Id: bitwise.h,v 1.6 2000/07/07 01:41:43 xiphmont Exp $

 ********************************************************************/

#ifndef _V_BITW_H_
#define _V_BITW_H_

#include "vorbis/codec.h"

extern void  _oggpack_writeinit(oggpack_buffer *b);
extern void  _oggpack_reset(oggpack_buffer *b);
extern void  _oggpack_writeclear(oggpack_buffer *b);
extern void  _oggpack_readinit(oggpack_buffer *b,unsigned char *buf,int bytes);
extern void  _oggpack_write(oggpack_buffer *b,unsigned long value,int bits);
extern long  _oggpack_look(oggpack_buffer *b,int bits);
extern long  _oggpack_look1(oggpack_buffer *b);
extern void  _oggpack_adv(oggpack_buffer *b,int bits);
extern void  _oggpack_adv1(oggpack_buffer *b);
extern long  _oggpack_read(oggpack_buffer *b,int bits);
extern long  _oggpack_read1(oggpack_buffer *b);
extern long  _oggpack_bytes(oggpack_buffer *b);
extern long  _oggpack_bits(oggpack_buffer *b);
extern unsigned char *_oggpack_buffer(oggpack_buffer *b);

#endif
