/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: metrics and quantization code for LSP VQ codebooks
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Dec 24 1999

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"

char *vqext_booktype="LSPdata";  
quant_meta q={0,0,0,1};          /* set sequence data */
int vqext_aux=1;

/* LSP training metric.  We weight error proportional to distance
   *between* LSP vector values.  The idea of this metric is not to set
   final cells, but get the midpoint spacing into a form conducive to
   what we want, which is weighting toward preserving narrower
   features. */

double global_maxdel=M_PI;
#define FUDGE ((global_maxdel*2.0)-weight[i])
double *weight=NULL;

double *vqext_weight(vqgen *v,double *p){
  int i;
  int el=v->elements;
  double lastp=0.;
  for(i=0;i<el;i++){
    double predist=(p[i]-lastp);
    double postdist=(p[i+1]-p[i]);
    weight[i]=(predist<postdist?predist:postdist);
    lastp=p[i];
  }
  return p;
}

                            /* candidate,actual */
double vqext_metric(vqgen *v,double *e, double *p){
  int i;
  int el=v->elements;
  double acc=0.;
  for(i=0;i<el;i++){
    double val=(p[i]-e[i])*FUDGE;
    acc+=val*val;
  }
  return acc;
}

/* Data files are line-vectors, starting with zero.  If we want to
   train on a subvector starting in the middle, we need to adjust the
   data as if it was starting at zero.  we also need to add the 'aux'
   value, which is an extra point at the end so we have leading and
   trailing space */

/* assume vqext_aux==1 */
void vqext_addpoint_adj(vqgen *v,double *b,int start,int dim,int cols){
  double *a=alloca(sizeof(double)*(dim+1)); /* +aux */
  double base=0;
  int i;

  if(start>0)base=b[start-1];
  for(i=0;i<dim;i++)a[i]=b[i+start]-base;
  if(start+dim+1>cols) /* +aux */
    a[i]=M_PI-base;
  else
    a[i]=b[i+start]-base;
  
  vqgen_addpoint(v,a,a+dim);
}

/* we just need to calc the global_maxdel from the training set */
void vqext_preprocess(vqgen *v){
  long j,k;

  global_maxdel=0.;
  for(j=0;j<v->points;j++){
    double last=0.;
    for(k=0;k<v->elements+v->aux;k++){
      double p=_point(v,j)[k];
      if(p-last>global_maxdel)global_maxdel=p-last;
      last=p;
    }
  }

  weight=malloc(sizeof(double)*v->elements);
}

