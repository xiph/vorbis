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

 function: miscellaneous prototypes
 last mod: $Id: misc.h,v 1.1 2000/01/22 13:28:27 xiphmont Exp $

 ********************************************************************/

#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_

extern void *_vorbis_block_alloc(vorbis_block *vb,long bytes);
extern void _vorbis_block_ripcord(vorbis_block *vb);

#endif
