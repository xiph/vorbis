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
 last modification date: Nov 18 1999

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
 * We use the ratio of local to average 'error' as the metric to bias a
 * variable-length word codebook, and probability of occurrence within
 * that bubble as the metric to bias fixed length word
 * codebooks. Individual input points, collected from libvorbis, are
 * used to train the algorithm monte-carlo style.  */

typedef struct vqgen{
  int    elements;
  double errspread;

  /* point cache */
  double *pointlist; 
  long   *first;
  long   *second;
  long   points;
  long   allocated;

  /* entries */
  double *entry_now;
  double *entry_next;
  long   *assigned;
  double *metric;
  double *bias;
  long   entries;

  double (*metric_func)   (struct vqgen *v,double *a,double *b);
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

double _dist_sq(vqgen *v,double *a, double *b){
  int i;
  int el=v->elements;
  double acc=0.;
  for(i=0;i<el;i++){
    double val=(a[i]-b[i]);
    acc+=val*val;
  }
  return acc;
}

void vqgen_init(vqgen *v,int elements,int entries,
		double (*metric)(vqgen *,double *, double *),
		double spread){
  memset(v,0,sizeof(vqgen));

  v->elements=elements;
  v->errspread=spread;
  v->allocated=32768;
  v->pointlist=malloc(v->allocated*v->elements*sizeof(double));
  v->first=malloc(v->allocated*sizeof(long));
  v->second=malloc(v->allocated*sizeof(long));

  v->entries=entries;
  v->entry_now=malloc(v->entries*v->elements*sizeof(double));
  v->entry_next=malloc(v->entries*v->elements*sizeof(double));
  v->assigned=malloc(v->entries*sizeof(long));
  v->metric=malloc(v->entries*sizeof(double));
  v->bias=calloc(v->entries,sizeof(double));
  if(metric)
    v->metric_func=metric;
  else
    v->metric_func=_dist_sq;
}

/* *must* be beefed up.  Perhaps a Floyd-Steinberg like scattering? */
void _vqgen_seed(vqgen *v){
  memcpy(v->entry_now,v->pointlist,sizeof(double)*v->entries*v->elements);
}

void vqgen_addpoint(vqgen *v, double *p){
  if(v->points>=v->allocated){
    v->allocated*=2;
    v->pointlist=realloc(v->pointlist,v->allocated*v->elements*sizeof(double));
    v->first=realloc(v->first,v->allocated*sizeof(long));
    v->second=realloc(v->second,v->allocated*sizeof(long));
  }
  
  memcpy(_point(v,v->points),p,sizeof(double)*v->elements);
  v->points++;
  if(v->points==v->entries)_vqgen_seed(v);
}

double *sort_t_vals;
int sort_t_compare(const void *ap,const void *bp){
  long a=*(long *)ap;
  long b=*(long *)bp;
  double val=sort_t_vals[a]-sort_t_vals[b];
  if(val<0)return(1);
  if(val>0)return(-1);
  return(0);
}

void vqgen_iterate(vqgen *v,int biasp){
  static int iteration=0;
  long i,j;
  double avmetric=0.;

  FILE *as;
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

  sprintf(name,"as%d.m",iteration);
  as=fopen(name,"w");


  /* init */
  memset(v->entry_next,0,sizeof(double)*v->elements*v->entries);
  memset(v->assigned,0,sizeof(long)*v->entries);
  memset(v->metric,0,sizeof(double)*v->entries);

  if(v->entries<2)exit(1);

  /* assign all the points, accumulate metric */
  for(i=0;i<v->points;i++){
    double firstmetric=v->metric_func(v,_now(v,0),_point(v,i))+v->bias[0];
    double secondmetric=v->metric_func(v,_now(v,1),_point(v,i))+v->bias[1];
    long   firstentry=0;
    long   secondentry=0;
    
    if(firstmetric>secondmetric){
      double tempmetric=firstmetric;
      firstmetric=secondmetric;
      secondmetric=tempmetric;
      firstentry=1;
      secondentry=1;
    }

    for(j=2;j<v->entries;j++){
      double thismetric=v->metric_func(v,_now(v,j),_point(v,i))+v->bias[j];
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
    
    v->first[i]=firstentry;
    v->second[i]=secondentry;
    v->metric[firstentry]+=firstmetric-v->bias[firstentry];
    v->assigned[firstentry]++;

    {
      double *no=_now(v,firstentry);
      double *ne=_next(v,firstentry);
      double *p=_point(v,i);
      for(j=0;j<v->elements;j++)
	ne[j]+=p[j];
      fprintf(graph,"%g %g\n%g %g\n\n",p[0],p[1],no[0],no[1]);
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
  
  /* average global metric */
  for(i=0;i<v->entries;i++){
    avmetric+=v->metric[i];
  }
  avmetric/=v->entries;
  fprintf(stderr,"%ld... ",iteration);

  /* positive/negative 'pressure' */  
  if(biasp){
    /* Another round of n*m. Above we had to index by point.  Now, we
       have to index by entry */
    long   *index=alloca(v->points*sizeof(long));
    double *vals =alloca(v->points*sizeof(double));
    double *met  =alloca(v->points*sizeof(double));
    for(i=0;i<v->entries;i++){

      /* sort all n-thousand points by effective metric */

      for(j=0;j<v->points;j++){
	int threshentry;
	index[j]=j;
	if(v->first[j]==i){
	  /* use the secondary entry as the threshhold */
	  threshentry=v->second[j];
	}else{
	  /* use the primary entry as the threshhold */
	  threshentry=v->first[j];
	}
	
	/* bias value right at the threshhold to grab this point */
	met[j]=v->metric_func(v,_now(v,i),_point(v,j));
	vals[j]=
	  v->metric_func(v,_now(v,threshentry),_point(v,j))+
	  v->bias[threshentry]-
	  v->metric_func(v,_now(v,i),_point(v,j));
      }
      
      sort_t_vals=vals;
      qsort(index,v->points,sizeof(long),sort_t_compare);

      {
	/* find the desired number of points and level of bias.  We
	   nominally want all entries to represent the same number of
	   points, but we may also have a metric range restriction */
	int desired=(v->points+v->entries-1)/v->entries;
	
	if(v->errspread>=1.){
	  double acc=0.;
	  for(j=0;j<desired;j++){
	    acc+=met[index[j]];
	    if(acc*v->errspread>avmetric)break;
	  }
	}else
	  j=desired;
	v->bias[i]=vals[index[j-1]];
      }
    }
  }else{
    memset(v->bias,0,sizeof(double)*v->entries);
  }

  /* dump state, report metric */
  for(i=0;i<v->entries;i++){
    for(j=0;j<v->assigned[i];j++)
      fprintf(err,"%g\n",v->metric[i]/v->assigned[i]);
    fprintf(bias,"%g\n",v->bias[i]);
    fprintf(as,"%ld\n",v->assigned[i]);
  }

  fprintf(stderr,"average metric: %g\n",avmetric/v->elements/v->points*v->entries);

  fclose(as);
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

  vqgen_init(&v,4,24,_dist_sq,1.);

  while(fgets(buffer,160,in)){
    double a[8];
    if(sscanf(buffer,"%lf %lf %lf %lf",
	      a,a+1,a+2,a+3)==4)
      vqgen_addpoint(&v,a);
  }
  fclose(in);

  for(i=0;i<20;i++)
    vqgen_iterate(&v,0);

  for(i=0;i<100;i++)
    vqgen_iterate(&v,1);

  for(i=0;i<5;i++)
    vqgen_iterate(&v,0);

  return(0);
}


