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

  function: LPC low level routines
  last mod: $Id: lpc.h,v 1.13.2.1 2000/08/31 09:00:00 xiphmont Exp $

 ********************************************************************/

#ifndef _V_LPC_H_
#define _V_LPC_H_

#include "vorbis/codec.h"
#include "smallft.h"

typedef struct lpclook{
  /* en/decode lookups */
  drft_lookup fft;

  int ln;
  int m;

} lpc_lookup;

extern void lpc_init(lpc_lookup *l,long mapped, int m);
extern void lpc_clear(lpc_lookup *l);

/* simple linear scale LPC code */
extern float vorbis_lpc_from_data(float *data,float *lpc,int n,int m);
extern float vorbis_lpc_from_curve(float *curve,float *lpc,lpc_lookup *l);

extern void vorbis_lpc_predict(float *coeff,float *prime,int m,
			       float *data,long n);


#endif
