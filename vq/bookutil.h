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
 last mod: $Id: bookutil.h,v 1.6.4.1 2000/08/31 09:00:02 xiphmont Exp $

 ********************************************************************/

#ifndef _V_BOOKUTIL_H_
#define _V_BOOKUTIL_H_

#include <stdio.h>
#include <sys/time.h>

#include "vorbis/codebook.h"

extern char     *get_line(FILE *in);
extern char     *setup_line(FILE *in);
extern int       get_line_value(FILE *in,float *value);
extern int       get_next_value(FILE *in,float *value);
extern int       get_next_ivalue(FILE *in,long *ivalue);
extern void      reset_next_value(void);
extern int       get_vector(codebook *b,FILE *in,int start,int num,float *a);
extern char     *find_seek_to(FILE *in,char *s);

extern codebook *codebook_load(char *filename);
extern void write_codebook(FILE *out,char *name,const static_codebook *c);

extern void spinnit(char *s,int n);
extern void build_tree_from_lengths(int vals, long *hist, long *lengths);
extern void build_tree_from_lengths0(int vals, long *hist, long *lengths);

#endif

