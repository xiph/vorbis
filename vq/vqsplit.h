/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: build a VQ codebook decision tree 
 last mod: $Id: vqsplit.h,v 1.4 2000/11/06 00:07:26 xiphmont Exp $

 ********************************************************************/

#ifndef _VQSPL_H_
#define _VQSPL_H_

#include "vorbis/codebook.h"

extern void vqsp_book(vqgen *v,codebook *b,long *quantlist);
extern int vqenc_entry(codebook *b,float *val);
extern int lp_split(float *pointlist,long totalpoints,
		    codebook *b,
		    long *entryindex,long entries, 
		    long *pointindex,long points,
		    long *membership,long *reventry,
		    long depth, long *pointsofar);

#endif





