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
 last mod: $Id: residuedata.c,v 1.1 2000/02/16 16:18:38 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "bookutil.h"
#include "vqext.h"

char *vqext_booktype="RESdata";  
quant_meta q={0,0,0,0};          /* set sequence data */
int vqext_aux=0;

double vqext_mindist=.7; /* minimum desired cell radius */

/* LSP training metric.  We weight error proportional to distance
   *between* LSP vector values.  The idea of this metric is not to set
   final cells, but get the midpoint spacing into a form conducive to
   what we want, which is weighting toward preserving narrower
   features. */

double *vqext_weight(vqgen *v,double *p){
  return p;
}

/* quantize aligned on unit boundaries */
void vqext_quantize(vqgen *v,quant_meta *q){
  int i,j,k;
  double min,max,delta=1.;
  int vals=(1<<q->quant);

  min=max=_now(v,0)[0];
  for(i=0;i<v->entries;i++)
    for(j=0;j<v->elements;j++){
      if(max<_now(v,i)[j])max=_now(v,i)[j];
      if(min>_now(v,i)[j])min=_now(v,i)[j];
    }

  /* roll the delta */
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
      q->min=float24_pack(qmin*delta);
      q->delta=float24_pack(delta);
      break;
    }
  }

  for(j=0;j<v->entries;j++){
    for(k=0;k<v->elements;k++){
      double val=_now(v,j)[k];
      double now=rint((val-min)/delta);
      _now(v,j)[k]=now;
    }
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

void vqext_preprocess(vqgen *v){
}
