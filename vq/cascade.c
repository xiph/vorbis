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
 last mod: $Id: cascade.c,v 1.5 2000/01/21 13:42:37 xiphmont Exp $

 ********************************************************************/

/* this one outputs residue to stdout. */

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "bookutil.h"

/* set up metrics */

double count=0.;
int dim=-1;
double *work=NULL;

void process_preprocess(codebook **bs,char *basename){
  while(*bs){
    codebook *b=*bs;
    if(dim==-1){
      dim=b->c->dim;
      work=malloc(sizeof(double)*dim);
    }else{
      if(dim!=b->c->dim){
	fprintf(stderr,"Each codebook in a cascade must have the same dimensional order\n");
	exit(1);
      }
    }
    bs++;
  }
}

void process_postprocess(codebook **b,char *basename){
  fprintf(stderr,"Done.                      \n");
}

void process_vector(codebook **bs,double *a){
  int i;
  memcpy(work,a,dim*sizeof(double));

  while(*bs){
    codebook *b=*bs;
    int entry=codebook_entry(b,work);
    double *e=b->valuelist+b->c->dim*entry;

    for(i=0;i<b->c->dim;i++)work[i]-=e[i];
    bs++;
  }

  for(i=0;i<dim;i++)
    fprintf(stdout,"%f, ",work[i]);
  fprintf(stdout,"\n");
  
  if((long)(count++)%100)spinnit("working.... lines: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqcascade book.vqh [book.vqh]... datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  residual error data sent to\n"
	  "       stdout.\n\n");

}
