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

 function: train a VQ codebook 
 last mod: $Id: vqgen.c,v 1.28 2000/01/06 13:57:13 xiphmont Exp $

 ********************************************************************/

/* This code is *not* part of libvorbis.  It is used to generate
   trained codebooks offline and then spit the results into a
   pregenerated codebook that is compiled into libvorbis.  It is an
   expensive (but good) algorithm.  Run it on big iron. */

/* There are so many optimizations to explore in *both* stages that
   considering the undertaking is almost withering.  For now, we brute
   force it all */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#include "vqgen.h"
#include "bookutil.h"

/* Codebook generation happens in two steps: 

   1) Train the codebook with data collected from the encoder: We use
   one of a few error metrics (which represent the distance between a
   given data point and a candidate point in the training set) to
   divide the training set up into cells representing roughly equal
   probability of occurring. 

   2) Generate the codebook and auxiliary data from the trained data set
*/

/* Codebook training ****************************************************
 *
 * The basic idea here is that a VQ codebook is like an m-dimensional
 * foam with n bubbles.  The bubbles compete for space/volume and are
 * 'pressurized' [biased] according to some metric.  The basic alg
 * iterates through allowing the bubbles to compete for space until
 * they converge (if the damping is dome properly) on a steady-state
 * solution. Individual input points, collected from libvorbis, are
 * used to train the algorithm monte-carlo style.  */

/* internal helpers *****************************************************/
#define vN(data,i) (data+v->elements*i)

/* default metric; squared 'distance' from desired value. */
double _dist(vqgen *v,double *a, double *b){
  int i;
  int el=v->elements;
  double acc=0.;
  for(i=0;i<el;i++){
    double val=(a[i]-b[i]);
    acc+=val*val;
  }
  return sqrt(acc);
}

double *_weight_null(vqgen *v,double *a){
  return a;
}

/* *must* be beefed up. */
void _vqgen_seed(vqgen *v){
  long i;
  for(i=0;i<v->entries;i++)
    memcpy(_now(v,i),_point(v,i),sizeof(double)*v->elements);
}

/* External calls *******************************************************/

/* We have two forms of quantization; in the first, each vector
   element in the codebook entry is orthogonal.  Residues would use this
   quantization for example.

   In the second, we have a sequence of monotonically increasing
   values that we wish to quantize as deltas (to save space).  We
   still need to quantize so that absolute values are accurate. For
   example, LSP quantizes all absolute values, but the book encodes
   distance between values because each successive value is larger
   than the preceeding value.  Thus the desired quantibits apply to
   the encoded (delta) values, not abs positions. This requires minor
   additional encode-side trickery. */

void vqgen_quantize(vqgen *v,quant_meta *q){

  double maxdel;
  double mindel;

  double delta;
  double maxquant=((1<<q->quant)-1);

  int j,k;

  mindel=maxdel=_now(v,0)[0];
  
  for(j=0;j<v->entries;j++){
    double last=0.;
    for(k=0;k<v->elements;k++){
      if(mindel>_now(v,j)[k]-last)mindel=_now(v,j)[k]-last;
      if(maxdel<_now(v,j)[k]-last)maxdel=_now(v,j)[k]-last;
      if(q->sequencep)last=_now(v,j)[k];
    }
  }


  /* first find the basic delta amount from the maximum span to be
     encoded.  Loosen the delta slightly to allow for additional error
     during sequence quantization */

  delta=(maxdel-mindel)/((1<<q->quant)-1.5);

  q->min=float24_pack(mindel);
  q->delta=float24_pack(delta);

  for(j=0;j<v->entries;j++){
    double last=0;
    for(k=0;k<v->elements;k++){
      double val=_now(v,j)[k];
      double now=rint((val-last-mindel)/delta);
      
      _now(v,j)[k]=now;
      if(now<0){
	/* be paranoid; this should be impossible */
	fprintf(stderr,"fault; quantized value<0\n");
	exit(1);
      }

      if(now>maxquant){
	/* be paranoid; this should be impossible */
	fprintf(stderr,"fault; quantized value>max\n");
	exit(1);
      }
      if(q->sequencep)last=(now*delta)+mindel+last;
    }
  }
}

/* much easier :-) */
void vqgen_unquantize(vqgen *v,quant_meta *q){
  long j,k;
  double mindel=float24_unpack(q->min);
  double delta=float24_unpack(q->delta);

  for(j=0;j<v->entries;j++){
    double last=0.;
    for(k=0;k<v->elements;k++){
      double now=_now(v,j)[k]*delta+last+mindel;
      _now(v,j)[k]=now;
      if(q->sequencep)last=now;

    }
  }
}

void vqgen_init(vqgen *v,int elements,int aux,int entries,
		double  (*metric)(vqgen *,double *, double *),
		double *(*weight)(vqgen *,double *)){
  memset(v,0,sizeof(vqgen));

  v->elements=elements;
  v->aux=aux;
  v->allocated=32768;
  v->pointlist=malloc(v->allocated*(v->elements+v->aux)*sizeof(double));

  v->entries=entries;
  v->entrylist=malloc(v->entries*v->elements*sizeof(double));
  v->assigned=malloc(v->entries*sizeof(long));
  v->bias=calloc(v->entries,sizeof(double));
  if(metric)
    v->metric_func=metric;
  else
    v->metric_func=_dist;
  if(weight)
    v->weight_func=weight;
  else
    v->weight_func=_weight_null;

}

void vqgen_addpoint(vqgen *v, double *p,double *a){
  if(v->points>=v->allocated){
    v->allocated*=2;
    v->pointlist=realloc(v->pointlist,v->allocated*(v->elements+v->aux)*
			 sizeof(double));
  }
  
  memcpy(_point(v,v->points),p,sizeof(double)*v->elements);
  if(v->aux)memcpy(_point(v,v->points)+v->elements,a,sizeof(double)*v->aux);
  v->points++;
  if(v->points==v->entries)_vqgen_seed(v);
}

int directdsort(const void *a, const void *b){
  double av=*((double *)a);
  double bv=*((double *)b);
  if(av>bv)return(-1);
  return(1);
}

double vqgen_iterate(vqgen *v){
  long   i,j,k;
  double fdesired=(double)v->points/v->entries;
  long  desired=fdesired;
  long  desired2=desired*2;
  double asserror=0.;
  double meterror=0.;
  double *new=malloc(sizeof(double)*v->entries*v->elements);
  long   *nearcount=malloc(v->entries*sizeof(long));
  double *nearbias=malloc(v->entries*desired2*sizeof(double));
 #ifdef NOISY
   char buff[80];
   FILE *assig;
   FILE *bias;
   FILE *cells;
   sprintf(buff,"cells%d.m",v->it);
   cells=fopen(buff,"w");
   sprintf(buff,"assig%d.m",v->it);
   assig=fopen(buff,"w");
   sprintf(buff,"bias%d.m",v->it);
   bias=fopen(buff,"w");
 #endif
 

  if(v->entries<2){
    fprintf(stderr,"generation requires at least two entries\n");
    exit(1);
  }

  /* fill in nearest points for entry biasing */
  /*memset(v->bias,0,sizeof(double)*v->entries);*/
  memset(nearcount,0,sizeof(long)*v->entries);
  memset(v->assigned,0,sizeof(long)*v->entries);
  for(i=0;i<v->points;i++){
    double *ppt=v->weight_func(v,_point(v,i));
    double firstmetric=v->metric_func(v,_now(v,0),ppt)+v->bias[0];
    double secondmetric=v->metric_func(v,_now(v,1),ppt)+v->bias[1];
    long   firstentry=0;
    long   secondentry=1;

    if(i%100)spinnit("biasing... ",v->points+v->points+v->entries-i);

    if(firstmetric>secondmetric){
      double temp=firstmetric;
      firstmetric=secondmetric;
      secondmetric=temp;
      firstentry=1;
      secondentry=0;
    }
    
    for(j=2;j<v->entries;j++){
      double thismetric=v->metric_func(v,_now(v,j),ppt)+v->bias[j];
      if(thismetric<secondmetric){
	if(thismetric<firstmetric){
	  secondmetric=firstmetric;
	  secondentry=firstentry;
	  firstmetric=thismetric;
	  firstentry=j;
	}else{
	  secondmetric=thismetric;
	  secondentry=j;
	}
      }
    }

    j=firstentry;
    for(j=0;j<v->entries;j++){
      
      double thismetric;
      double *nearbiasptr=nearbias+desired2*j;
      long k=nearcount[j];
      
      /* 'thismetric' is to be the bias value necessary in the current
	 arrangement for entry j to capture point i */
      if(firstentry==j){
	/* use the secondary entry as the threshhold */
	thismetric=secondmetric-v->metric_func(v,_now(v,j),ppt);
      }else{
	/* use the primary entry as the threshhold */
	thismetric=firstmetric-v->metric_func(v,_now(v,j),ppt);
      }

      /* a cute two-stage delayed sorting hack */
      if(k<desired){
	nearbiasptr[k]=thismetric;
	k++;
	if(k==desired){
	  spinnit("biasing... ",v->points+v->points+v->entries-i);
	  qsort(nearbiasptr,desired,sizeof(double),directdsort);
	}
	
      }else if(thismetric>nearbiasptr[desired-1]){
	nearbiasptr[k]=thismetric;
	k++;
	if(k==desired2){
	  spinnit("biasing... ",v->points+v->points+v->entries-i);
	  qsort(nearbiasptr,desired2,sizeof(double),directdsort);
	  k=desired;
	}
      }
      nearcount[j]=k;
    }
  }
  
  /* inflate/deflate */
  for(i=0;i<v->entries;i++){
    double *nearbiasptr=nearbias+desired2*i;

    spinnit("biasing... ",v->points+v->entries-i);

    /* due to the delayed sorting, we likely need to finish it off....*/
    if(nearcount[i]>desired)
      qsort(nearbiasptr,nearcount[i],sizeof(double),directdsort);

    v->bias[i]=nearbiasptr[desired-1];
  }

  /* Now assign with new bias and find new midpoints */
  for(i=0;i<v->points;i++){
    double *ppt=v->weight_func(v,_point(v,i));
    double firstmetric=v->metric_func(v,_now(v,0),ppt)+v->bias[0];
    long   firstentry=0;

    if(i%100)spinnit("centering... ",v->points-i);

    for(j=0;j<v->entries;j++){
      double thismetric=v->metric_func(v,_now(v,j),ppt)+v->bias[j];
      if(thismetric<firstmetric){
	firstmetric=thismetric;
	firstentry=j;
      }
    }

    j=firstentry;
      
#ifdef NOISY
    fprintf(cells,"%g %g\n%g %g\n\n",
          _now(v,j)[0],_now(v,j)[1],
          ppt[0],ppt[1]);
#endif

    meterror+=firstmetric-v->bias[firstentry];
    /* set up midpoints for next iter */
    if(v->assigned[j]++)
      for(k=0;k<v->elements;k++)
	vN(new,j)[k]+=ppt[k];
    else
      for(k=0;k<v->elements;k++)
	vN(new,j)[k]=ppt[k];

  }

  /* assign midpoints */

  for(j=0;j<v->entries;j++){
#ifdef NOISY
    fprintf(assig,"%ld\n",v->assigned[j]);
    fprintf(bias,"%g\n",v->bias[j]);
#endif
    asserror+=fabs(v->assigned[j]-fdesired);
    if(v->assigned[j])
      for(k=0;k<v->elements;k++)
	_now(v,j)[k]=vN(new,j)[k]/v->assigned[j];
  }

  asserror/=(v->entries*fdesired);

  fprintf(stderr,"Pass #%d... ",v->it);
  fprintf(stderr,": dist %g(%g) metric error=%g \n",
	  asserror,fdesired,meterror/v->points);
  v->it++;
  
  free(new);
  free(nearcount);
  free(nearbias);
#ifdef NOISY
  fclose(assig);
  fclose(bias);
  fclose(cells);
#endif
  return(asserror);
}

