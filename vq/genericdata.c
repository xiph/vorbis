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

 function: generic euclidian distance metric for VQ codebooks
 last mod: $Id: genericdata.c,v 1.5 2000/05/08 20:49:50 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"

char *vqext_booktype="GENERICdata";  
int vqext_aux=0;                
quant_meta q={0,0,0,0};          /* non sequence data; each scalar 
				    independent */

void vqext_quantize(vqgen *v,quant_meta *q){
  vqgen_quantize(v,q);
}

double *vqext_weight(vqgen *v,double *p){
  /*noop*/
  return(p);
}

                            /* candidate,actual */
double vqext_metric(vqgen *v,double *e, double *p){
  int i;
  double acc=0.;
  for(i=0;i<v->elements;i++){
    double val=p[i]-e[i];
    acc+=val*val;
  }
  return sqrt(acc/v->elements);
}

void vqext_addpoint_adj(vqgen *v,double *b,int start,int dim,int cols,int num){
  vqgen_addpoint(v,b+start,NULL);
}

void vqext_preprocess(vqgen *v){
  /* noop */
}






