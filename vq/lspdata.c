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
 last modification date: Dec 15 1999

 ********************************************************************/

#include <math.h>
#include <stdio.h>
#include "vqgen.h"
#include "vqext.h"

char *vqext_booktype="LSPdata";

/* LSP training metric.  We weight error proportional to distance
   *between* LSP vector values.  The idea of this metric is not to set
   final cells, but get the midpoint spacing into a form conducive to
   what we want, which is weighting toward preserving narrower
   features. */

double global_maxdel=M_PI;
#define FUDGE ((global_maxdel*1.0)-testdist)

                            /* candidate,actual */
double vqext_metric(vqgen *v,double *b, double *a){
  int i;
  int el=v->elements;
  double acc=0.;
  double lasta=0.;
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

/* LSP quantizes all absolute values, but the book encodes distance
   between values (which has a smaller range).  Thus the desired
   quantibits apply to the encoded (delta) values, not abs
   positions. This requires minor additional trickery. */

quant_return vqext_quantize(vqgen *v,int quantbits){
  quant_return q;
  double maxval=0.;
  double maxdel;
  double mindel;
  double delta;
  double fullrangevals;
  double maxquant=((1<<quantbits)-1);
  int j,k;

  mindel=maxdel=_now(v,0)[0];
  
  for(j=0;j<v->entries;j++){
    double last=0.;
    for(k=0;k<v->elements;k++){
      if(mindel>_now(v,j)[k]-last)mindel=_now(v,j)[k]-last;
      if(maxdel<_now(v,j)[k]-last)maxdel=_now(v,j)[k]-last;
      if(maxval<_now(v,j)[k])maxval=_now(v,j)[k];
      last=_now(v,j)[k];
    }
  }

  q.minval=0.;
  delta=(maxdel-mindel)/((1<<quantbits)-2);
  fullrangevals=floor(maxval/delta);
  q.delt=delta=maxval/fullrangevals;
  q.addtoquant=floor(mindel/delta);

  for(j=0;j<v->entries;j++){
    double last=0.;
    for(k=0;k<v->elements;k++){
      double val=_now(v,j)[k];
      double now=rint(val/delta);
      double test=_now(v,j)[k]=now-last-q.addtoquant;

      if(test<0){
	fprintf(stderr,"fault; quantized value<0\n");
	exit(1);
      }

      if(test>maxquant){
	fprintf(stderr,"fault; quantized value>max\n");
	exit(1);
      }
      last=now;
    }
  }
  return(q);
}

/* much easier :-) */
void vqext_unquantize(vqgen *v,quant_return *q){
  long j,k;
  if(global_maxdel==M_PI)global_maxdel=0.;
  for(j=0;j<v->entries;j++){
    double last=0.;
    for(k=0;k<v->elements;k++){
      double del=(_now(v,j)[k]+q->addtoquant)*q->delt+q->minval;
      last+=del;
      _now(v,j)[k]=last;
      if(del>global_maxdel)global_maxdel=del;
    }
  }
}
