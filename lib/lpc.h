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

extern double vorbis_curve_to_lpc(double *curve,int n,double *lpc,int m);
extern void vorbis_lpc_to_curve(double *curve,int n,double *lpc, 
				double amp,int m);
extern void vorbis_lpc_apply(double *residue,int n,double *lpc, 
			      double amp,int m);

#endif
