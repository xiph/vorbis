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

 function: utility functions for loading .vqh and .vqd files
 last mod: $Id: bookutil.h,v 1.1 2000/01/05 10:14:54 xiphmont Exp $

 ********************************************************************/

#ifndef _V_BOOKUTIL_H_
#define _V_BOOKUTIL_H_

#include <stdio.h>
#include "vorbis/codebook.h"

extern void      codebook_unquantize(codebook *b);
extern char     *get_line(FILE *in);
extern int       get_line_value(FILE *in,double *value);
extern int       get_next_value(FILE *in,double *value);
extern int       get_next_ivalue(FILE *in,long *ivalue);
extern void      reset_next_value(void);
extern int       get_vector(codebook *b,FILE *in,double *a);
extern char     *find_seek_to(FILE *in,char *s);

extern codebook *codebook_load(char *filename);
extern int       codebook_entry(codebook *b,double *val);


extern long float24_pack(double val);
extern double float24_unpack(long val);

#endif

