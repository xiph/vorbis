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

  function: LSP (also called LSF) conversion routines
  last mod: $Id: lsp.h,v 1.4.2.1 2000/08/31 09:00:00 xiphmont Exp $

 ********************************************************************/


#ifndef _V_LSP_H_
#define _V_LSP_H_

extern void vorbis_lpc_to_lsp(float *lpc,float *lsp,int m);
extern void vorbis_lsp_to_curve(float *curve,int n,
				float *lsp,int m,float amp,
				float *w);
  
#endif
