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

 function: metrics and quantization code for residue VQ codebooks
 last mod: $Id: residuedata.c,v 1.3 2000/05/08 20:49:51 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "vqgen.h"
#include "bookutil.h"
#include "../lib/sharedbook.h"
#include "../lib/scales.h"
#include "vqext.h"

float scalequant=3.;
char *vqext_booktype="RESdata";  
quant_meta q={0,0,0,0};          /* set sequence data */
int vqext_aux=0;

static double *quant_save=NULL;

double *vqext_weight(vqgen *v,double *p){
  return p;
}

/* quantize aligned on unit boundaries.  Because our grid is likely
   very coarse, play 'shuffle the blocks'; don't allow multiple
   entries to fill the same spot as is nearly certain to happen. */

void vqext_quantize(vqgen *v,quant_meta *q){
  int j,k;
  long dim=v->elements;
  long n=v->entries;
  double max=-1;
  double *test=alloca(sizeof(double)*dim);
  int moved=0;

  
  /* allow movement only to unoccupied coordinates on the coarse grid */
  for(j=0;j<n;j++){
    for(k=0;k<dim;k++){
      double val=_now(v,j)[k];
      double norm=rint(fabs(val)/scalequant);
      if(norm>max)max=norm;
      test[k]=norm;
    }

    /* allow move only if unoccupied */
    if(quant_save){
      for(k=0;k<n;k++)
	if(j!=k && memcmp(test,quant_save+dim*k,dim*sizeof(double))==0)
	  break;
      if(k==n){
	if(memcmp(test,quant_save+dim*j,dim*sizeof(double)))	
	  moved++;
	memcpy(quant_save+dim*j,test,sizeof(double)*dim);
      }
    }else{
      memcpy(_now(v,j),test,sizeof(double)*dim);
    }
  }

  /* unlike the other trainers, we fill in our quantization
     information (as we know granularity beforehand and don't need to
     maximize it) */

  q->min=_float32_pack(0.);
  q->delta=_float32_pack(scalequant);
  q->quant=_ilog(max);

  if(quant_save){
    memcpy(_now(v,0),quant_save,sizeof(double)*dim*n);
    fprintf(stderr,"cells shifted this iteration: %d\n",moved);
  }
}

                            /* candidate,actual */
double vqext_metric(vqgen *v,double *e, double *p){
  int i;
  double acc=0.;
  for(i=0;i<v->elements;i++){
    double val=p[i]-e[i];
    acc+=val*val;
  }
  return sqrt(acc);
}

/* We don't interleave here; we assume that the interleave is provided
   for us by residuesplit in vorbis/huff/ */
void vqext_addpoint_adj(vqgen *v,double *b,int start,int dim,int cols,int num){
  vqgen_addpoint(v,b+start,NULL);
}

/* need to reseed because of the coarse quantization we tend to use on
   residuals (which causes lots & lots of dupes) */
void vqext_preprocess(vqgen *v){
  long i,j,k,l;
  double *test=alloca(sizeof(double)*v->elements);
  scalequant=q.quant;

  vqext_quantize(v,&q);
  vqgen_unquantize(v,&q);

  /* if there are any dupes, reseed */
  for(k=0;k<v->entries;k++){
    for(l=0;l<k;l++){
      if(memcmp(_now(v,k),_now(v,l),sizeof(double)*v->elements)==0)
	break;
    }
    if(l<k)break;
  }

  if(k<v->entries){
    fprintf(stderr,"reseeding with quantization....\n");

    /* seed the inputs to input points, but points on unit boundaries,
     ignoring quantbits for now, making sure each seed is unique */
    
    for(i=0,j=0;i<v->points && j<v->entries;i++){
      for(k=0;k<v->elements;k++){
	double val=_point(v,i)[k];
	test[k]=rint(val/scalequant)*scalequant;
      }
      
      for(l=0;l<j;l++){
	for(k=0;k<v->elements;k++)
	  if(test[k]!=_now(v,l)[k])
	    break;
	if(k==v->elements)break;
      }
      if(l==j){
	memcpy(_now(v,j),test,v->elements*sizeof(double));
	j++;
      }
    }
    
    if(j<v->elements){
      fprintf(stderr,"Not enough unique entries after prequantization\n");
      exit(1);
    }
  }  
  vqext_quantize(v,&q);
  quant_save=malloc(sizeof(double)*v->elements*v->entries);
  memcpy(quant_save,_now(v,0),sizeof(double)*v->elements*v->entries);
  vqgen_unquantize(v,&q);

}

