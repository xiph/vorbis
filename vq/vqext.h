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

 function: prototypes for extermal metrics specific to data type

 ********************************************************************/

#ifndef _V_VQEXT_
#define _V_VQEXT_

#include "vqgen.h"

extern char *vqext_booktype;
extern quant_meta q;
extern int vqext_aux;

extern double vqext_metric(vqgen *v,double *e, double *p);
extern double *vqext_weight(vqgen *v,double *p);
extern void vqext_addpoint_adj(vqgen *v,double *b,int start,int dim,int cols);
extern void vqext_preprocess(vqgen *v);

#endif
