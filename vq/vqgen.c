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
 last modification date: Dec 10 1999

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

double *_point(vqgen *v,long ptr){
  return v->pointlist+(v->elements*ptr);
}

double *_now(vqgen *v,long ptr){
  return v->entrylist+(v->elements*ptr);
}

/* default metric; squared 'distance' from desired value. */
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

/* *must* be beefed up. */
void _vqgen_seed(vqgen *v){
  memcpy(v->entrylist,v->pointlist,sizeof(double)*v->entries*v->elements);
}

/* External calls *******************************************************/

void vqgen_init(vqgen *v,int elements,int entries,
		double (*metric)(vqgen *,double *, double *),
		int quant){
  memset(v,0,sizeof(vqgen));

  v->elements=elements;
  v->quantbits=quant;
  v->allocated=32768;
  v->pointlist=malloc(v->allocated*v->elements*sizeof(double));

  v->entries=entries;
  v->entrylist=malloc(v->entries*v->elements*sizeof(double));
  v->assigned=malloc(v->entries*sizeof(long));
  v->bias=calloc(v->entries,sizeof(double));
  if(metric)
    v->metric_func=metric;
  else
    v->metric_func=_dist_sq;
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

double vqgen_iterate(vqgen *v){
  long   i,j,k;
  double fdesired=(double)v->points/v->entries;
  long  desired=fdesired;
  double asserror=0.;
  double meterror=0.;
  double *new=malloc(sizeof(double)*v->entries*v->elements);
  long   *nearcount=malloc(v->entries*sizeof(long));
  double *nearbias=malloc(v->entries*desired*sizeof(double));

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

  fprintf(stderr,"Pass #%d... ",v->it);

  if(v->entries<2){
    fprintf(stderr,"generation requires at least two entries\n");
    exit(1);
  }

  /* fill in nearest points for entries */
  /*memset(v->bias,0,sizeof(double)*v->entries);*/
  memset(nearcount,0,sizeof(long)*v->entries);
  memset(v->assigned,0,sizeof(long)*v->entries);
  for(i=0;i<v->points;i++){
    double *ppt=_point(v,i);
    double firstmetric=v->metric_func(v,_now(v,0),ppt)+v->bias[0];
    double secondmetric=v->metric_func(v,_now(v,1),ppt)+v->bias[1];
    long   firstentry=0;
    long   secondentry=1;
    if(firstmetric>secondmetric){
      double temp=firstmetric;
      firstmetric=secondmetric;
      secondmetric=temp;
      firstentry=1;
      secondentry=0;
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
      
    j=firstentry;
    meterror+=firstmetric-v->bias[firstentry];
    /* set up midpoints for next iter */
    if(v->assigned[j]++)
      for(k=0;k<v->elements;k++)
	vN(new,j)[k]+=_point(v,i)[k];
    else
      for(k=0;k<v->elements;k++)
	vN(new,j)[k]=_point(v,i)[k];

   
#ifdef NOISY
    fprintf(cells,"%g %g\n%g %g\n\n",
	    _now(v,j)[0],_now(v,j)[1],
	    _point(v,i)[0],_point(v,i)[1]);
#endif

    for(j=0;j<v->entries;j++){
      
      double thismetric;
      double *nearbiasptr=nearbias+desired*j;
      long k=nearcount[j]-1;
      
      /* 'thismetric' is to be the bias value necessary in the current
	 arrangement for entry j to capture point i */
      if(firstentry==j){
	/* use the secondary entry as the threshhold */
	thismetric=secondmetric-v->metric_func(v,_now(v,j),_point(v,i));
      }else{
	/* use the primary entry as the threshhold */
	thismetric=firstmetric-v->metric_func(v,_now(v,j),_point(v,i));
      }
      
      if(k>=0 && thismetric>nearbiasptr[k]){
	
	/* start at the end and search backward for where this entry
	   belongs */
	
	for(;k>0;k--) if(nearbiasptr[k-1]>=thismetric)break;
	
	/* insert at k.  Shift and inject. */
	memmove(nearbiasptr+k+1,nearbiasptr+k,(desired-k-1)*sizeof(double));
	nearbiasptr[k]=thismetric;
	
	if(nearcount[j]<desired)nearcount[j]++;
	
      }else{
	if(nearcount[j]<desired){
	  /* we checked the thresh earlier.  We know this is the
	     last entry */
	  nearbiasptr[nearcount[j]++]=thismetric;
	}
      }
    }
  }
  
  /* inflate/deflate */
  for(i=0;i<v->entries;i++)
    v->bias[i]=nearbias[(i+1)*desired-1];

  /* assign midpoints */

  for(j=0;j<v->entries;j++){
    asserror+=fabs(v->assigned[j]-fdesired);
    if(v->assigned[j])
      for(k=0;k<v->elements;k++)
	_now(v,j)[k]=vN(new,j)[k]/v->assigned[j];
#ifdef NOISY
    fprintf(assig,"%ld\n",v->assigned[j]);
    fprintf(bias,"%g\n",v->bias[j]);
#endif
  }

  {
    /* midpoints must be quantized.  but we need to know the range in
       order to do so */
    double min,max;
   
    for(k=0;k<v->elements;k++){
      double delta;
      min=max=_now(v,0)[k];

      for(j=1;j<v->entries;j++){
	double val=_now(v,j)[k];
	if(val<min)min=val;
	if(val>max)max=val;
      }
    
      delta=(max-min)/((1<<v->quantbits)-1);
      for(j=0;j<v->entries;j++){
	double val=_now(v,j)[k];
	_now(v,j)[k]=min+delta*rint((val-min)/delta);
      }
    }
  }

  asserror/=(v->entries*fdesired);
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


/* Building a codebook from trained set **********************************

   The codebook in raw form is technically finished once it's trained.
   However, we want to finalize the representative codebook values for
   each entry and generate auxiliary information to optimize encoding.
   We generate the auxiliary coding tree using collected data,
   probably the same data as in the original training */

/* At each recursion, the data set is split in half.  Cells with data
   points on side A go into set A, same with set B.  The sets may
   overlap.  If the cell overlaps the deviding line only very slightly
   (provided parameter), we may choose to ignore the overlap in order
   to pare the tree down */

double *sortvals;
int els;
int iascsort(const void *a,const void *b){
  double av=sortvals[*((long *)a) * els];
  double bv=sortvals[*((long *)b) * els];
  if(av<bv)return(-1);
  return(1);
}

/* goes through the split, but just counts it and returns a metric*/
void lp_count(vqgen *v,long *entryindex,long entries, 
		long *pointindex,long points,
		long *entryA,long *entryB,
		double *n, double c, double slack,
		long *entriesA,long *entriesB,long *entriesC){
  long i,j,k;
  long A=0,B=0,C=0;

  memset(entryA,0,sizeof(long)*entries);
  memset(entryB,0,sizeof(long)*entries);

  for(i=0;i<points;i++){
    double *ppt=_point(v,pointindex[i]);
    long   firstentry=0;
    double firstmetric=_dist_sq(v,_now(v,entryindex[0]),ppt);
    double position=-c;
    
    for(j=1;j<entries;j++){
      double thismetric=_dist_sq(v,_now(v,entryindex[j]),ppt);
      if(thismetric<firstmetric){
	firstmetric=thismetric;
	firstentry=j;
      }
    }

    /* count point split */
    for(k=0;k<v->elements;k++)
      position+=ppt[k]*n[k];
    if(position>0.){
      entryA[firstentry]++;
    }else{
      entryB[firstentry]++;
    }
  }

  /* look to see if entries are in the slack zone */
  /* The entry splitting isn't total, so that storage has to be
     allocated for recursion.  Reuse the entryA/entryB vectors */
  for(j=0;j<entries;j++){
    long total=entryA[j]+entryB[j];
    if((double)entryA[j]/total<slack){
      entryA[j]=0;
    }else if((double)entryB[j]/total<slack){
      entryB[j]=0;
    }
    if(entryA[j] && entryB[j])C++;
    if(entryA[j])entryA[A++]=entryindex[j];
    if(entryB[j])entryB[B++]=entryindex[j];
  }
  *entriesA=A;
  *entriesB=B;
  *entriesC=C;
}

void pq_in_out(vqgen *v,double *n,double *c,double *p,double *q){
  int k;
  *c=0.;
  for(k=0;k<v->elements;k++){
    double center=(p[k]+q[k])/2.;
    n[k]=(center-q[k])*2.;
    *c+=center*n[k];
  }
}

void pq_center_out(vqgen *v,double *n,double *c,double *center,double *q){
  int k;
  *c=0.;
  for(k=0;k<v->elements;k++){
    n[k]=(center[k]-q[k])*2.;
    *c+=center[k]*n[k];
  }
}

int lp_split(vqgen *v,vqbook *b,
	     long *entryindex,long entries, 
	     long *pointindex,long points,
	     long depth,double slack){

  /* The encoder, regardless of book, will be using a straight
     euclidian distance-to-point metric to determine closest point.
     Thus we split the cells using the same (we've already trained the
     codebook set spacing and distribution using special metrics and
     even a midpoint division won't disturb the basic properties) */

  long ret;
  double *p;
  double *q;
  double *n;
  double c;
  long *entryA=calloc(entries,sizeof(long));
  long *entryB=calloc(entries,sizeof(long));
  long entriesA=0;
  long entriesB=0;
  long entriesC=0;
  long pointsA=0;
  long i,j,k;

  p=alloca(sizeof(double)*v->elements);
  q=alloca(sizeof(double)*v->elements);
  n=alloca(sizeof(double)*v->elements);
  memset(p,0,sizeof(double)*v->elements);

  /* We need to find the dividing hyperplane. find the median of each
     axis as the centerpoint and the normal facing farthest point */

  /* more than one way to do this part.  For small sets, we can brute
     force it. */

  if(entries<32){
    /* try every pair possibility */
    double best=0;
    long   besti=0;
    long   bestj=0;
    double this;
    for(i=0;i<entries-1;i++){
      for(j=i+1;j<entries;j++){
	pq_in_out(v,n,&c,_now(v,entryindex[i]),_now(v,entryindex[j]));
	lp_count(v,entryindex,entries, 
		 pointindex,points,
		 entryA,entryB,
		 n, c, slack,
		 &entriesA,&entriesB,&entriesC);
	this=(entriesA-entriesC)*(entriesB-entriesC);

	if(this>best){
	  best=this;
	  besti=i;
	  bestj=j;
	}
      }
    }
    pq_in_out(v,n,&c,_now(v,entryindex[besti]),_now(v,entryindex[bestj]));
  }else{
    double best=0.;
    long   bestj=0;

    /* try COG/normal and furthest pairs */
    /* medianpoint */
    for(k=0;k<v->elements;k++){
      /* just sort the index array */
      sortvals=v->pointlist+k;
      els=v->elements;
      qsort(pointindex,points,sizeof(long),iascsort);
      if(points&0x1){
	p[k]=v->pointlist[(pointindex[points/2])*v->elements+k];
      }else{
	p[k]=(v->pointlist[(pointindex[points/2])*v->elements+k]+
	      v->pointlist[(pointindex[points/2-1])*v->elements+k])/2.;
      }
    }
    
    /* try every normal, but just for distance */
    for(j=0;j<entries;j++){
      double *ppj=_now(v,entryindex[j]);
      double this=_dist_sq(v,p,ppj);
      if(this>best){
	best=this;
	bestj=j;
      }
    }

    pq_center_out(v,n,&c,p,_point(v,pointindex[bestj]));


  }

  /* find cells enclosing points */
  /* count A/B points */

  lp_count(v,entryindex,entries, 
	   pointindex,points,
	   entryA,entryB,
	   n, c, slack,
	   &entriesA,&entriesB,&entriesC);

  /* the point index is split evenly, so we do an Order n
     rearrangement into A first/B last and just pass it on */
  {
    long Aptr=0;
    long Bptr=points-1;
    while(Aptr<=Bptr){
      while(Aptr<=Bptr){
	double position=-c;
	for(k=0;k<v->elements;k++)
	  position+=_point(v,pointindex[Aptr])[k]*n[k];
	if(position<=0.)break; /* not in A */
	Aptr++;
      }
      while(Aptr<=Bptr){
	double position=-c;
	for(k=0;k<v->elements;k++)
	  position+=_point(v,pointindex[Bptr])[k]*n[k];
	if(position>0.)break; /* not in B */
	Bptr--;
      }
      if(Aptr<Bptr){
	long temp=pointindex[Aptr];
	pointindex[Aptr]=pointindex[Bptr];
	pointindex[Bptr]=temp;
      }
      pointsA=Aptr;
    }
  }

  fprintf(stderr,"split: total=%ld depth=%ld set A=%ld:%ld:%ld=B\n",
	  entries,depth,entriesA-entriesC,entriesC,entriesB-entriesC);
  {
    long thisaux=b->aux++;
    if(b->aux>=b->alloc){
      b->alloc*=2;
      b->ptr0=realloc(b->ptr0,sizeof(long)*b->alloc);
      b->ptr1=realloc(b->ptr1,sizeof(long)*b->alloc);
      b->n=realloc(b->n,sizeof(double)*b->elements*b->alloc);
      b->c=realloc(b->c,sizeof(double)*b->alloc);
    }
    
    memcpy(b->n+b->elements*thisaux,n,sizeof(double)*v->elements);
    b->c[thisaux]=c;

    if(entriesA==1){
      ret=1;
      b->ptr0[thisaux]=entryA[0];
    }else{
      b->ptr0[thisaux]= -b->aux;
      ret=lp_split(v,b,entryA,entriesA,pointindex,pointsA,depth+1,slack); 
    }
    if(entriesB==1){
      ret++;
      b->ptr1[thisaux]=entryB[0];
    }else{
      b->ptr1[thisaux]= -b->aux;
      ret+=lp_split(v,b,entryB,entriesB,pointindex+pointsA,points-pointsA,
		   depth+1,slack); 
    }
  }
  free(entryA);
  free(entryB);
  return(ret);
}

int vqenc_entry(vqbook *b,double *val){
  int ptr=0,k;
  while(1){
    double c= -b->c[ptr];
    double *nptr=b->n+b->elements*ptr;
    for(k=0;k<b->elements;k++)
      c+=nptr[k]*val[k];
    if(c>0.) /* in A */
      ptr= -b->ptr0[ptr];
    else     /* in B */
      ptr= -b->ptr1[ptr];
    if(ptr<=0)break;
  }
  return(-ptr);
}

void vqgen_book(vqgen *v,vqbook *b){
  long i;
  long *entryindex=malloc(sizeof(double)*v->entries);
  long *pointindex=malloc(sizeof(double)*v->points);

  memset(b,0,sizeof(vqbook));
  for(i=0;i<v->entries;i++)entryindex[i]=i;
  for(i=0;i<v->points;i++)pointindex[i]=i;
  b->elements=v->elements;
  b->entries=v->entries;
  b->alloc=4096;
  b->ptr0=malloc(sizeof(long)*b->alloc);
  b->ptr1=malloc(sizeof(long)*b->alloc);
  b->n=malloc(sizeof(double)*b->elements*b->alloc);
  b->c=malloc(sizeof(double)*b->alloc);

  b->valuelist=malloc(sizeof(double)*b->elements*b->entries);
  b->codelist=malloc(sizeof(long)*b->entries);
  b->lengthlist=malloc(b->entries*sizeof(long));
  
  /* first, generate the encoding decision heirarchy */
  fprintf(stderr,"Total leaves: %ld\n",
	  lp_split(v,b,entryindex,v->entries, pointindex,v->points,0,0));
  
  /* run all training points through the decision tree to get a final
     probability count */
  {
    long *probability=calloc(b->entries*2,sizeof(long));
    for(i=0;i<v->points;i++){
      int ret=vqenc_entry(b,v->pointlist+i*v->elements);
      probability[ret]++;
    }

    for(i=0;i<b->entries;i++){
      fprintf(stderr,"point %ld: %ld\n",i,probability[i]);
    }

  }

  /* generate the codewords (short to long) */

  free(entryindex);
  free(pointindex);
}














