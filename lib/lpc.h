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
  last mod: $Id: lpc.h,v 1.9.4.1 2000/04/06 15:59:37 xiphmont Exp $

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
extern double vorbis_lpc_from_data(double *data,double *lpc,int n,int m);
extern double vorbis_lpc_from_curve(double *curve,double *lpc,lpc_lookup *l);
extern void vorbis_lpc_to_curve(double *curve,double *lpc,double amp,
				lpc_lookup *l);

/* standard lpc stuff */
extern void vorbis_lpc_residue(double *coeff,double *prime,int m,
			double *data,long n);
extern void vorbis_lpc_predict(double *coeff,double *prime,int m,
			double *data,long n);

#endif
