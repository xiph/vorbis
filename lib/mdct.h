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

 function: modified discrete cosine transform prototypes
 last mod: $Id: mdct.h,v 1.12.2.1 2000/11/04 06:21:45 xiphmont Exp $

 ********************************************************************/

#ifndef _OGG_mdct_H_
#define _OGG_mdct_H_

#include "vorbis/codec.h"

typedef struct {
  int n;
  int log2n;
  
  float *trig;
  int    *bitrev;

} mdct_lookup;

extern void mdct_init(mdct_lookup *lookup,int n);
extern void mdct_clear(mdct_lookup *l);
extern void mdct_forward(mdct_lookup *init, float *in, float *out);
extern void mdct_backward(mdct_lookup *init, float *in, float *out);

#endif












