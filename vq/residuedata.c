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
 last mod: $Id: residuedata.c,v 1.2 2000/02/21 01:12:57 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "vqgen.h"
#include "bookutil.h"
#include "vqext.h"

char *vqext_booktype="RESdata";  
quant_meta q={0,0,0,0};          /* set sequence data */
int vqext_aux=0;

static double *quant_save=NULL;

/* LSP training metric.  We weight error proportional to distance
   *between* LSP vector values.  The idea of this metric is not to set
   final cells, but get the midpoint spacing into a form conducive to
   what we want, which is weighting toward preserving narrower
   features. */

double *vqext_weight(vqgen *v,double *p){
  return p;
}

/* quantize aligned on unit boundaries */

static double _delta(double min,double max,int bits){
  double delta=1.;
  int vals=(1<<bits);
  while(1){
    double qmax=rint(max/delta);
    double qmin=rint(min/delta);
    int qvals=rint(qmax-qmin)+1;

    if(qvals>vals){
      delta*=2;
    }else if(qvals*2<=vals){
      delta*=.5;
    }else{
      min=qmin*delta;
      break;
    }
  }
  return(delta);
}

void vqext_quantize(vqgen *v,quant_meta *q){
  int i,j,k;
  long dim=v->elements;
  long n=v->entries;
  double min,vals,delta=1.;
  double *test=alloca(sizeof(double)*dim);
  int moved=0;

  min=-((1<<q->quant)/2-1);
  vals=1<<q->quant;
  q->min=float24_pack(rint(min/delta)*delta);
  q->delta=float24_pack(delta);
  
  /* allow movement only to unoccupied coordinates on the coarse grid */
  for(j=0;j<n;j++){
    for(k=0;k<dim;k++){
      double val=_now(v,j)[k];
      val=rint((val-min)/delta);
      if(val<0)val=0;
      if(val>=vals)val=vals-1;
      test[k]=val;
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

  if(quant_save){
    memcpy(_now(v,0),quant_save,sizeof(double)*dim*n);
    fprintf(stderr,"cells shifted this iteration: %d\n",moved);
  }
}

/* this should probably go to a x^6/4 error metric */

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

void vqext_addpoint_adj(vqgen *v,double *b,int start,int dim,int cols){
  vqgen_addpoint(v,b+start,NULL);
}

/* need to reseed because of the coarse quantization we tend to use on
   residuals (which causes lots & lots of dupes) */
void vqext_preprocess(vqgen *v){
  long i,j,k,l,min,max;
  double *test=alloca(sizeof(double)*v->elements);

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
    min=-((1<<q.quant)/2-1);
    max=min+(1<<q.quant)-1;

    /* seed the inputs to input points, but points on unit boundaries,
     ignoring quantbits for now, making sure each seed is unique */
    
    for(i=0,j=0;i<v->points && j<v->entries;i++){
      for(k=0;k<v->elements;k++){
	test[k]=rint(_point(v,i)[k]);
	if(test[k]<min)test[k]=min;
	if(test[k]>max)test[k]=max;
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
