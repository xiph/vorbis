/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

  function: LPC low level routines

 ********************************************************************/

#ifndef _V_LPC_H_
#define _V_LPC_H_

#include "codec.h"

extern void lpc_init(lpc_lookup *l,int n, int mapped,
		       int m, int oct, int encode_p);
extern void lpc_clear(lpc_lookup *l);

/* simple linear scale LPC code */
extern double vorbis_gen_lpc(double *curve,double *lpc,lpc_lookup *l);
extern double vorbis_lpc_magnitude(double w,double *lpc, int m);

/* log scale layer */
extern double vorbis_curve_to_lpc(double *curve,double *lpc,lpc_lookup *l);
extern void vorbis_lpc_to_curve(double *curve,double *lpc, double amp,
				lpc_lookup *l);
extern void vorbis_lpc_apply(double *residue,double *lpc, double amp,
			     lpc_lookup *l);

#endif
