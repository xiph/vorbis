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

 function: fft transform
 last mod: $Id: smallft.h,v 1.6 2000/01/22 13:28:32 xiphmont Exp $

********************************************************************/

#ifndef _V_SMFT_H_
#define _V_SMFT_H_

#include "vorbis/codec.h"

typedef struct {
  int n;
  double *trigcache;
  int *splitcache;
} drft_lookup;

extern void drft_forward(drft_lookup *l,double *data);
extern void drft_backward(drft_lookup *l,double *data);
extern void drft_init(drft_lookup *l,int n);
extern void drft_clear(drft_lookup *l);

#endif
