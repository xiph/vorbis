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

 function: function call to do simple data cascading
 last mod: $Id: cascade.c,v 1.2 2000/01/05 15:04:56 xiphmont Exp $

 ********************************************************************/

/* this one just outputs to stdout */

#include "bookutil.h"

void process_preprocess(codebook *b,char *basename){
}
void process_postprocess(codebook *b,char *basename){
}

void process_vector(codebook *b,double *a){
  int entry=codebook_entry(b,a);
  double *e=b->valuelist+b->dim*entry;
  int i;

  for(i=0;i<b->dim;i++)
    fprintf(stdout,"%f, ",a[i]-e[i]);
  fprintf(stdout,"\n");
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqcascade <codebook>.vqh datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  residual error data sent to\n"
	  "       stdout.\n\n");

}
