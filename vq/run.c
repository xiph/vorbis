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
 last mod: $Id: run.c,v 1.9 2000/02/23 09:10:11 xiphmont Exp $

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

extern void process_preprocess(codebook **b,char *basename);
extern void process_postprocess(codebook **b,char *basename);
extern void process_vector(codebook **b,double *a);
extern void process_usage(void);

int main(int argc,char *argv[]){
  char *basename;
  double *a=NULL;
  codebook **b=calloc(1,sizeof(codebook *));
  int books=0;
  int input=0;

  int start=0;
  int num=-1;
  argv++;

  if(*argv==NULL){
    process_usage();
    exit(1);
  }

  /* yes, this is evil.  However, it's very convenient to parse file
     extentions */

  while(*argv){
    if(*argv[0]=='-'){
      /* option */
      if(argv[0][1]=='s'){
	/* subvector */
	if(sscanf(argv[1],"%d,%d",&start,&num)!=2){
	  num= -1;
	  if(sscanf(argv[1],"%d",&start)!=1){
	    fprintf(stderr,"Syntax error using -s\n");
	    exit(1);
	  }
	}
	argv+=2;
      }
    }else{
      /* input file.  What kind? */
      char *dot;
      char *ext=NULL;
      char *name=strdup(*argv++);
      dot=strrchr(name,'.');
      if(dot)
	ext=dot+1;
      else
	ext="";

      /* codebook */
      if(!strcmp(ext,"vqh")){
	if(input){
	  fprintf(stderr,"specify all input data (.vqd) files following\n"
		  "codebook header (.vqh) files\n");
	  exit(1);
	}

	basename=strrchr(name,'/');
	if(basename)
	  basename=strdup(basename)+1;
	else
	  basename=strdup(name);
	dot=strrchr(basename,'.');
	if(dot)*dot='\0';

	b=realloc(b,sizeof(codebook *)*(books+2));
	b[books++]=codebook_load(name);
	b[books]=NULL;
	if(!a)a=malloc(sizeof(double)*b[books-1]->c->dim);
      }

      /* data file */
      if(!strcmp(ext,"vqd")){
	FILE *in=fopen(name,"r");
	if(!in){
	  fprintf(stderr,"Could not open input file %s\n",name);
	  exit(1);
	}

	if(!input){
	  process_preprocess(b,basename);
	  input++;
	}

	reset_next_value();

	while(get_vector(*b,in,start,num,a)!=-1)
	  process_vector(b,a);

	fclose(in);
      }
    }
  }

  /* take any data from stdin */
  {
    struct stat st;
    if(fstat(STDIN_FILENO,&st)==-1){
      fprintf(stderr,"Could not stat STDIN\n");
      exit(1);
    }
    if((S_IFIFO|S_IFREG|S_IFSOCK)&st.st_mode){
      if(!input){
	process_preprocess(b,basename);
	input++;
      }
      
      reset_next_value();
      while(get_vector(*b,stdin,start,num,a)!=-1)
	process_vector(b,a);
    }
  }

  process_postprocess(b,basename);

  return 0;
}
