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

 function: utility main for loading and operating on codebooks
 last mod: $Id: run.c,v 1.5 2000/01/05 15:04:59 xiphmont Exp $

 ********************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "bookutil.h"

/* command line:
   utilname input_book.vqh input_data.vqd [input_data.vqd]

   produces output data on stdout
   (may also take input data from stdin)

 */

extern void process_preprocess(codebook *b,char *basename);
extern void process_postprocess(codebook *b,char *basename);
extern void process_vector(codebook *b,double *a);
extern void process_usage(void);

int main(int argc,char *argv[]){
  char *name;
  char *basename;
  double *a=NULL;
  codebook *b=NULL;
  argv++;

  if(*argv==NULL){
    process_usage();
    exit(1);
  }

  name=strdup(*argv);
  b=codebook_load(name);
  a=alloca(sizeof(double)*b->dim);
  argv=argv++;

  {
    char *dot;
    basename=strrchr(name,'/');
    if(basename)
      basename=strdup(basename);
    else
      basename=strdup(name);
    dot=strchr(basename,'.');
    if(dot)*dot='\0';
  }

  process_preprocess(b,basename);
  
  while(*argv){
    /* only input files */
    char *file=strdup(*argv++);
    FILE *in=fopen(file,"r");
    reset_next_value();

    while(get_vector(b,in,a)!=-1)
      process_vector(b,a);

    fclose(in);
  }

  /* take any data from stdin */
  {
    struct stat st;
    if(fstat(STDIN_FILENO,&st)==-1){
      fprintf(stderr,"Could not stat STDIN\n");
      exit(1);
    }
    if((S_IFIFO|S_IFREG|S_IFSOCK)&st.st_mode){
      reset_next_value();
      while(get_vector(b,stdin,a)!=-1)
	process_vector(b,a);
    }
  }

  process_postprocess(b,basename);

  return 0;
}
