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

 function: build a VQ codebook decision tree 
 last mod: $Id: vqsplit.h,v 1.1 2000/01/05 10:15:00 xiphmont Exp $

 ********************************************************************/

#ifndef _VQSPL_H_
#define _VQSPL_H_

#include "vorbis/codebook.h"

extern void vqsp_book(vqgen *v,codebook *b,long *quantlist);
extern int vqenc_entry(codebook *b,double *val);

#endif





