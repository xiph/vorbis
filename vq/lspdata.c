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

#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"

char *vqext_booktype="LSPdata";  
quant_meta q={0,0,0,1};          /* set sequence data */

/* LSP training metric.  We weight error proportional to distance
   *between* LSP vector values.  The idea of this metric is not to set
   final cells, but get the midpoint spacing into a form conducive to
   what we want, which is weighting toward preserving narrower
   features. */

double global_maxdel=M_PI;
#define FUDGE ((global_maxdel*2.0)-testdist)

                            /* candidate,actual */
double vqext_metric(vqgen *v,double *b, double *a){
  int i;
  int el=v->elements;
  double acc=0.;
  /*double lasta=0.;*/
  double lastb=0.;
  for(i=0;i<el;i++){

    /*    double needdist=(a[i]-lastb);
	  double actualdist=(a[i]-lasta);*/
    double testdist=(b[i]-lastb);

    double val=(a[i]-b[i])*FUDGE;

    acc+=val*val;

    /*lasta=a[i];*/
    lastb=b[i];
  }
  return acc;
}

/* Data files are line-vectors, starting with zero.  If we want to
   train on a subvector starting in the middle, we need to adjust the
   data as if it was starting at zero */

void vqext_adjdata(double *b,int start,int dim){
  if(start>0){
    int i;
    double base=b[start-1];
    for(i=start;i<start+dim;i++)b[i]-=base;
  }
}

/* we just need to calc the global_maxdel from the training set */
void vqext_preprocess(vqgen *v){
  long j,k;

  global_maxdel=0.;
  for(j=0;j<v->entries;j++){
    double last=0.;
    for(k=0;k<v->elements;k++){
      double now=_now(v,j)[k];
      if(now-last>global_maxdel)global_maxdel=now-last;
      last=now;
    }
  }
}
