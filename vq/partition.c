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

 function: function call to do data partitioning
 last mod: $Id: partition.c,v 1.1 2000/01/06 13:57:13 xiphmont Exp $

 ********************************************************************/

/* make n files, one for each cell.  partition the input data into the
   N cells */

#include <stdlib.h>
#include <stdio.h>
#include "bookutil.h"

/* open all the partitioning files */
FILE **out;
long count;

void process_preprocess(codebook *b,char *basename){
  int i;
  char *buffer=alloca(strlen(basename)+80);

  out=malloc(sizeof(FILE *)*b->entries);
  for(i=0;i<b->entries;i++){
    sprintf(buffer,"%s-%dp.vqd",basename,i);
    out[i]=fopen(buffer,"w");
    if(out[i]==NULL){
      fprintf(stderr,"Could not open file %s\n",buffer);
      exit(1);
    }
  }
}

void process_postprocess(codebook *b,char *basename){
  int i;
  for(i=0;i<b->entries;i++)
    fclose(out[i]);
  fprintf(stderr,"Done.                      \n");
}

/* just redirect this entry to the appropriate partition file */
void process_vector(codebook *b,double *a){
  int entry=codebook_entry(b,a);
  int i;

  for(i=0;i<b->dim;i++)
    fprintf(out[entry],"%f, ",a[i]);
  fprintf(out[entry],"\n");
  if(count++%100)spinnit("working.... lines: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqpartition <codebook>.vqh datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  partitioning data is written to\n"
  
	  "       files named <basename>-<n>p.vqh.\n\n");

}
