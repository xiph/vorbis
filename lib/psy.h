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

 function: random psychoacoustics (not including preecho)
 
 ********************************************************************/

#ifndef _V_PSY_H_
#define _V_PSY_H_

extern void _vp_psy_init(psy_lookup *p,vorbis_info *vi,int n);
extern void _vp_psy_clear(psy_lookup *p);

extern void _vp_noise_floor(psy_lookup *p, double *f, double *m);
extern void _vp_mask_floor(psy_lookup *p,double *f, double *m);


extern double _vp_balance_compute(double *A, double *B, double *lpc,
			   lpc_lookup *vb);
extern void _vp_balance_apply(double *A, double *B, double *lpc, double amp,
			      lpc_lookup *vb,int divp);


#endif
