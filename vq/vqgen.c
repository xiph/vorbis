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

 function: build a VQ codebook 
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Nov 11 1999

 ********************************************************************/

/* This code is *not* part of libvorbis.  It is used to generate
   trained codebooks offline and then spit the results into a
   pregenerated codebook that is compiled into libvorbis.  It is an
   expensive (but good) algorithm.  Run it on big iron. */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

/************************************************************************
 * The basic idea here is that a VQ codebook is like an m-dimensional
 * foam with n bubbles.  The bubbles compete for space/volume and are
 * 'pressurized' [biased] according to some metric.  The basic alg
 * iterates through allowing the bubbles to compete for space until
 * they converge (if the damping is dome properly) on a steady-state
 * solution.
 *
 * We use the ratio of local to average error as the metric to bias a
 * variable-length word codebook, and probability of occurrence within
 * that bubble as the metric to bias fixed length word
 * codebooks. Individual input points, collected from libvorbis, are
 * used to train the algorithm monte-carlo style.  */

typedef struct vqgen{
  int    elements;

  /* point cache */
  double *pointlist; 
  long   points;
  long   allocated;

  /* entries */
  double *entry_now;
  double *entry_next;
  long   *assigned;
  double *error;
  double *bias;
  long   entries;

  double (*error_func)   (struct vqgen *v,double *a,double *b);
  double (*bias_func)     (struct vqgen *v,int entry);
} vqgen;

/* internal helpers *****************************************************/
double *_point(vqgen *v,long ptr){
  return v->pointlist+(v->elements*ptr);
}

double *_now(vqgen *v,long ptr){
  return v->entry_now+(v->elements*ptr);
}

double *_next(vqgen *v,long ptr){
  return v->entry_next+(v->elements*ptr);
}

double _error_func(vqgen *v,double *a, double *b){
  int i;
  int el=v->elements;
  double acc=0.;
  for(i=0;i<el;i++)
    acc+=(a[i]-b[i])*(a[i]-b[i]);
  return acc;
}

double _vqgen_distance(vqgen *v,double *a, double *b){
  int i;
  double acc=0.;
  for(i=0;i<v->elements;i++)acc+=(a[i]-b[i])*(a[i]-b[i]);
  return sqrt(acc);
}

void vqgen_init(vqgen *v,int elements,int entries){
  memset(v,0,sizeof(vqgen));

  v->elements=elements;
  v->allocated=8192;
  v->pointlist=malloc(v->allocated*v->elements*sizeof(double));

  v->entries=entries;
  v->entry_now=malloc(v->entries*v->elements*sizeof(double));
  v->entry_next=malloc(v->entries*v->elements*sizeof(double));
  v->assigned=malloc(v->entries*sizeof(long));
  v->error=malloc(v->entries*sizeof(double));
  v->bias=malloc(v->entries*sizeof(double));
  {
    int i;
    for(i=0;i<v->entries;i++)
      v->bias[i]=0.;
  }
  
  /*v->lasterror=-1;*/

  v->error_func=_error_func;
}

void _vqgen_seed(vqgen *v){
  memcpy(v->entry_now,v->pointlist,sizeof(double)*v->entries*v->elements);
}

void vqgen_addpoint(vqgen *v, double *p){
  if(v->points>=v->allocated){
    v->allocated*=2;
    v->pointlist=realloc(v->pointlist,v->allocated*v->elements*sizeof(double));
  }
  
  memcpy(_point(v,v->points),p,sizeof(double)*v->elements);
  v->points++;
  if(v->points==v->entries)_vqgen_seed(v);
}

void vqgen_iterate(vqgen *v,int biasp){
  static int iteration=0;
  long i,j;
  double averror=0.;
  double realerror=0.;

  FILE *graph;
  FILE *err;
  FILE *bias;
  char name[80];
  

  sprintf(name,"space%d.m",iteration);
  graph=fopen(name,"w");

  sprintf(name,"err%d.m",iteration);
  err=fopen(name,"w");

  sprintf(name,"bias%d.m",iteration);
  bias=fopen(name,"w");


  /* init */
  memset(v->entry_next,0,sizeof(double)*v->elements*v->entries);
  memset(v->assigned,0,sizeof(long)*v->entries);
  memset(v->error,0,sizeof(double)*v->entries);

  /* assign all the points, accumulate error */
  for(i=0;i<v->points;i++){
    double besterror=v->error_func(v,_now(v,0),_point(v,i))+v->bias[0];
    long   bestentry=0;
    for(j=1;j<v->entries;j++){
      double thiserror=v->error_func(v,_now(v,j),_point(v,i))+v->bias[j];
      if(thiserror<besterror){
	besterror=thiserror;
	bestentry=j;
      }
    }
    
    v->error[bestentry]+=v->error_func(v,_now(v,bestentry),_point(v,i));
    realerror+=sqrt(v->error_func(v,_now(v,bestentry),_point(v,i))/v->elements);

    v->assigned[bestentry]++;
    {
      double *n=_next(v,bestentry);
      double *p=_point(v,i);
      for(j=0;j<v->elements;j++)
	n[j]+=p[j];
    }

    {
      double *n=_now(v,bestentry);
      double *p=_point(v,i);
      fprintf(graph,"%g %g\n%g %g\n\n",p[0],p[1],n[0],n[1]);
    }
  }

  /* new midpoints */
  for(i=0;i<v->entries;i++){
    double *next=_next(v,i);
    double *now=_now(v,i);
    if(v->assigned[i]){
      for(j=0;j<v->elements;j++)
	next[j]/=v->assigned[i];
    }else{
      for(j=0;j<v->elements;j++)
	next[j]=now[j];
    }
  }
  
  /* average global error */
  for(i=0;i<v->entries;i++){
    averror+=v->error[i];
    if(v->error[i]==0)fprintf(stderr,"%d ",i);
  }
  fprintf(stderr,"\n",i);

  averror/=v->entries;

  /* positive/negative 'pressure' */
  
  if(biasp){
    for(i=0;i<v->entries;i++){
      double bias=0;
      if(v->error[i]){
	bias=(averror-v->error[i])/v->assigned[i]*.2;
	v->bias[i]-=bias;
      }else{
	fprintf(stderr,"de-biasing\n");
	memset(v->bias,0,sizeof(double)*v->entries);
	break;
      }
      
      /*if(bias>.1)bias=.1;
	if(bias<-.1)bias=-.1;*/
    }
    fprintf(stderr,"\n");
  }else{
    fprintf(stderr,"de-biasing\n");
    memset(v->bias,0,sizeof(double)*v->entries);
  }

  /* dump state, report error */
  for(i=0;i<v->entries;i++){
    fprintf(err,"%g\n",v->error[i]);
    fprintf(bias,"%g\n",v->bias[i]);
  }

  fprintf(stderr,"average error: %g\n",realerror/v->points);

  fclose(err);
  fclose(bias);
  fclose(graph);
  memcpy(v->entry_now,v->entry_next,sizeof(double)*v->entries*v->elements);
  iteration++;
}

int main(int argc,char *argv[]){
  FILE *in=fopen(argv[1],"r");
  vqgen v;
  char buffer[160];
  int i;

  vqgen_init(&v,4,128);

  while(fgets(buffer,160,in)){
    double a[8];
    if(sscanf(buffer,"%lf %lf %lf %lf",
	      a,a+1,a+2,a+3)==4)
      vqgen_addpoint(&v,a);
  }
  fclose(in);

    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);

  for(i=0;i<100;i++)
    vqgen_iterate(&v,i%10);

    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);
    vqgen_iterate(&v,0);

  return(0);
}


