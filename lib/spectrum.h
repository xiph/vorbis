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

 function: spectrum envelope and residue code/decode
 last mod: $Id: spectrum.h,v 1.3 1999/12/30 07:26:52 xiphmont Exp $

 ********************************************************************/

#ifndef _V_SPECT_H_
#define _V_SPECT_H_

extern int  _vs_spectrum_encode(vorbis_block *vb,double amp,double *lsp);
extern int  _vs_spectrum_decode(vorbis_block *vb,double *amp,double *lsp);
extern void _vs_residue_quantize(double *data,double *curve,
				 vorbis_info *vi,int n);
extern int  _vs_residue_encode(vorbis_block *vb,double *data);
extern int  _vs_residue_decode(vorbis_block *vb,double *data);

#endif
