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
 last mod: $Id: misc.h,v 1.3 2000/03/10 13:21:18 xiphmont Exp $

 ********************************************************************/

#ifndef _V_RANDOM_H_
#define _V_RANDOM_H_
#include "vorbis/codec.h"

extern void *_vorbis_block_alloc(vorbis_block *vb,long bytes);
extern void _vorbis_block_ripcord(vorbis_block *vb);
extern void _analysis_output(char *base,int i,double *v,int n);

#ifdef DEBUG_LEAKS
extern void *_VDBG_malloc(void *ptr,long bytes,char *file,long line); 
extern void _VDBG_free(void *ptr,char *file,long line); 

#ifndef MISC_C 
#undef malloc
#undef calloc
#undef realloc
#undef free

#define malloc(x) _VDBG_malloc(NULL,(x),__FILE__,__LINE__)
#define calloc(x,y) _VDBG_malloc(NULL,(x)*(y),__FILE__,__LINE__)
#define realloc(x,y) _VDBG_malloc((x),(y),__FILE__,__LINE__)
#define free(x) _VDBG_free((x),__FILE__,__LINE__)
#endif
#endif

#endif
