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
 last mod: $Id: cascade.c,v 1.5.4.3 2000/04/21 16:35:40 xiphmont Exp $

 ********************************************************************/

/* this one outputs residue to stdout. */

#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "vorbis/codebook.h"
#include "../lib/sharedbook.h"
#include "bookutil.h"

/* set up metrics */

double count=0.;


void process_preprocess(codebook **bs,char *basename){
}

void process_postprocess(codebook **b,char *basename){
  fprintf(stderr,"Done.                      \n");
}

void process_vector(codebook **bs,int *addmul,int inter,double *a,int n){
  int i;
  int booknum=0;

  while(*bs){
    codebook *b=*bs;
    int dim=b->dim;

    if(inter){
      for(i=0;i<n/dim;i++)
	vorbis_book_besterror(b,a+i,n/dim,addmul[booknum]);
    }else{
      for(i=0;i<=n-dim;i+=dim)
	vorbis_book_besterror(b,a+i,1,addmul[booknum]);
    }

    bs++;
    booknum++;
  }

  for(i=0;i<n;i++)
    fprintf(stdout,"%f, ",a[i]);
  fprintf(stdout,"\n");
  
  if((long)(count++)%100)spinnit("working.... lines: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqcascade [-i] +|*<codebook>.vqh [ +|*<codebook.vqh> ]... \n"
	  "                 datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  residual error data sent to\n"
	  "       stdout.\n\n");

}
