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
#include <sys/time.h>

/* Codebook generation happens in two steps: 

   1) Train the codebook with data collected from the encoder: We use
   one of a few error metrics (which represent the distance between a
   given data point and a candidate point in the training set) to
   divide the training set up into cells representing roughly equal
   probability of occurring. 

   2) Generate the codebook and auxiliary data from the trained data set
*/

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

long *isortvals;
int iascsort(const void *a,const void *b){
  long av=isortvals[*((long *)a)];
  long bv=isortvals[*((long *)b)];
  return(av-bv);
}

extern double _dist_sq(vqgen *v,double *a, double *b);

/* goes through the split, but just counts it and returns a metric*/
void vqsp_count(vqgen *v,long *membership,
		long *entryindex,long entries, 
		long *pointindex,long points,
		long *entryA,long *entryB,
		double *n, double c,
		long *entriesA,long *entriesB,long *entriesC){
  long i,j,k;
  long A=0,B=0,C=0;

  memset(entryA,0,sizeof(long)*entries);
  memset(entryB,0,sizeof(long)*entries);

  /* Do the points belonging to this cell occur on sideA, sideB or
     both? */

  for(i=0;i<points;i++){
    double *ppt=_point(v,pointindex[i]);
    long   firstentry=membership[i];
    double position=-c;

    if(!entryA[firstentry] || !entryB[firstentry]){
      for(k=0;k<v->elements;k++)
      position+=ppt[k]*n[k];
      if(position>0.)
	entryA[firstentry]=1;
      else
	entryB[firstentry]=1;
    }
  }

  /* The entry splitting isn't total, so that storage has to be
     allocated for recursion.  Reuse the entryA/entryB vectors */
  for(j=0;j<entries;j++){
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

static void spinnit(void){
  static int p=0;
  static long lasttime=0;
  long test;
  struct timeval thistime;

  gettimeofday(&thistime,NULL);
  test=thistime.tv_sec*10+thistime.tv_usec/100000;
  if(lasttime!=test){
    lasttime=test;

    p++;if(p>3)p=0;
    switch(p){
    case 0:
      fprintf(stderr,"|\b");
      break;
    case 1:
      fprintf(stderr,"/\b");
      break;
    case 2:
      fprintf(stderr,"-\b");
      break;
    case 3:
      fprintf(stderr,"\\\b");
      break;
    }
    fflush(stderr);
  }
}

int lp_split(vqgen *v,vqbook *b,
	     long *entryindex,long entries, 
	     long *pointindex,long points,
	     long depth){

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

  long *membership=malloc(sizeof(long)*points);
  
  /* which cells do points belong to?  Do this before n^2 best pair chooser. */

  for(i=0;i<points;i++){
    double *ppt=_point(v,pointindex[i]);
    long   firstentry=0;
    double firstmetric=_dist_sq(v,_now(v,entryindex[0]),ppt);
    
    if(points*entries>64*1024)spinnit();

    for(j=1;j<entries;j++){
      double thismetric=_dist_sq(v,_now(v,entryindex[j]),ppt);
      if(thismetric<firstmetric){
	firstmetric=thismetric;
	firstentry=j;
      }
    }
    
    membership[i]=firstentry;
  }

  p=alloca(sizeof(double)*v->elements);
  q=alloca(sizeof(double)*v->elements);
  n=alloca(sizeof(double)*v->elements);
  memset(p,0,sizeof(double)*v->elements);

  /* We need to find the dividing hyperplane. find the median of each
     axis as the centerpoint and the normal facing farthest point */

  /* more than one way to do this part.  For small sets, we can brute
     force it. */

  if(entries<8 || points*entries*entries<128*1024*1024){
    /* try every pair possibility */
    double best=0;
    long   besti=0;
    long   bestj=0;
    double this;
    for(i=0;i<entries-1;i++){
      for(j=i+1;j<entries;j++){
	spinnit();
	pq_in_out(v,n,&c,_now(v,entryindex[i]),_now(v,entryindex[j]));
	vqsp_count(v,membership,
		   entryindex,entries, 
		   pointindex,points,
		   entryA,entryB,
		   n, c, 
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
      spinnit();
      
      p[k]=0.;
      for(j=0;j<entries;j++)
	p[k]+=v->entrylist[entryindex[j]*v->elements+k];
      p[k]/=entries;

    }
    
    /* try every normal */
    for(j=0;j<entries;j++){
      double *ppj=_now(v,entryindex[j]);
      double this;
      spinnit();

      pq_center_out(v,n,&c,p,_now(v,entryindex[j]));
      vqsp_count(v,membership,
		 entryindex,entries, 
		 pointindex,points,
		 entryA,entryB,
		 n, c, 
		 &entriesA,&entriesB,&entriesC);
      this=(entriesA-entriesC)*(entriesB-entriesC);
      
      if(this>best){
	best=this;
	bestj=j;
      }
    }
    
    pq_center_out(v,n,&c,p,_now(v,entryindex[bestj]));
  }
  
  /* find cells enclosing points */
  /* count A/B points */

  vqsp_count(v,membership,
	     entryindex,entries, 
	     pointindex,points,
	     entryA,entryB,
	     n, c,
	     &entriesA,&entriesB,&entriesC);

  free(membership);

  /* the point index is split, so we do an Order n rearrangement into
     A first/B last and just pass it on */
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
      b->n=realloc(b->n,sizeof(double)*b->dim*b->alloc);
      b->c=realloc(b->c,sizeof(double)*b->alloc);
    }
    
    memcpy(b->n+b->dim*thisaux,n,sizeof(double)*v->elements);
    b->c[thisaux]=c;

    if(entriesA==1){
      ret=1;
      b->ptr0[thisaux]=entryA[0];
    }else{
      b->ptr0[thisaux]= -b->aux;
      ret=lp_split(v,b,entryA,entriesA,pointindex,pointsA,depth+1); 
    }
    if(entriesB==1){
      ret++;
      b->ptr1[thisaux]=entryB[0];
    }else{
      b->ptr1[thisaux]= -b->aux;
      ret+=lp_split(v,b,entryB,entriesB,pointindex+pointsA,points-pointsA,
		    depth+1); 
    }
  }
  free(entryA);
  free(entryB);
  return(ret);
}

void vqsp_book(vqgen *v, vqbook *b, long *quantlist){
  long *entryindex=malloc(sizeof(long)*v->entries);
  long *pointindex=malloc(sizeof(long)*v->points);
  long *membership=malloc(sizeof(long)*v->points);
  long i,j;

  memset(b,0,sizeof(vqbook));
  for(i=0;i<v->entries;i++)entryindex[i]=i;
  for(i=0;i<v->points;i++)pointindex[i]=i;
  
  b->dim=v->elements;
  b->entries=v->entries;
  b->alloc=4096;

  b->ptr0=malloc(sizeof(long)*b->alloc);
  b->ptr1=malloc(sizeof(long)*b->alloc);
  b->n=malloc(sizeof(double)*b->dim*b->alloc);
  b->c=malloc(sizeof(double)*b->alloc);
  b->lengthlist=calloc(b->entries,sizeof(long));
  
  /* which cells do points belong to?  Only do this once. */

  for(i=0;i<v->points;i++){
    double *ppt=_point(v,i);
    long   firstentry=0;
    double firstmetric=_dist_sq(v,_now(v,0),ppt);
    
    for(j=1;j<v->entries;j++){
      double thismetric=_dist_sq(v,_now(v,j),ppt);
      if(thismetric<firstmetric){
	firstmetric=thismetric;
	firstentry=j;
      }
    }
    
    membership[i]=firstentry;
  }


  /* first, generate the encoding decision heirarchy */
  fprintf(stderr,"Total leaves: %d\n",
	  lp_split(v,b,entryindex,v->entries, 
		   pointindex,v->points,0));
  
  free(entryindex);
  free(pointindex);

  /* run all training points through the decision tree to get a final
     probability count */
  {
    long *probability=calloc(b->entries,sizeof(long));
    long *membership=malloc(b->entries*sizeof(long));

    for(i=0;i<v->points;i++){
      int ret=vqenc_entry(b,v->pointlist+i*v->elements);
      probability[ret]++;
    }
    for(i=0;i<v->entries;i++)membership[i]=i;

    /* find codeword lengths */
    /* much more elegant means exist.  Brute force n^2, minimum thought */
    for(i=v->entries;i>1;i--){
      int first=-1,second=-1;
      long least=v->points+1;

      /* find the two nodes to join */
      for(j=0;j<v->entries;j++)
	if(probability[j]<least){
	  least=probability[j];
	  first=membership[j];
	}
      least=v->points+1;
      for(j=0;j<v->entries;j++)
	if(probability[j]<least && membership[j]!=first){
	  least=probability[j];
	  second=membership[j];
	}
      if(first==-1 || second==-1){
	fprintf(stderr,"huffman fault; no free branch\n");
	exit(1);
      }

      /* join them */
      least=probability[first]+probability[second];
      for(j=0;j<v->entries;j++)
	if(membership[j]==first || membership[j]==second){
	  membership[j]=first;
	  probability[j]=least;
	  b->lengthlist[j]++;
	}
    }
    for(i=0;i<v->entries-1;i++)
      if(membership[i]!=membership[i+1]){
	fprintf(stderr,"huffman fault; failed to build single tree\n");
	exit(1);
      }

    /* unneccessary metric */
    memset(probability,0,sizeof(long)*v->entries);
    for(i=0;i<v->points;i++){
      int ret=vqenc_entry(b,v->pointlist+i*v->elements);
      probability[ret]++;
    }
    
    /* print the results */
    for(i=0;i<v->entries;i++)
      fprintf(stderr,"%ld: count=%ld codelength=%ld\n",i,probability[i],b->lengthlist[i]);

    free(probability);
    free(membership);
  }

  /* Sort the entries by codeword length, short to long (eases
     assignment and packing to do it now) */
  {
    long *wordlen=b->lengthlist;
    long *index=malloc(b->entries*sizeof(long));
    long *revindex=malloc(b->entries*sizeof(long));
    int k;
    for(i=0;i<b->entries;i++)index[i]=i;
    isortvals=b->lengthlist;
    qsort(index,b->entries,sizeof(long),iascsort);

    /* rearrange storage; ptr0/1 first as it needs a reverse index */
    /* n and c stay unchanged */
    for(i=0;i<b->entries;i++)revindex[index[i]]=i;
    for(i=0;i<b->aux;i++){
      if(b->ptr0[i]>=0)b->ptr0[i]=revindex[b->ptr0[i]];
      if(b->ptr1[i]>=0)b->ptr1[i]=revindex[b->ptr1[i]];
    }
    free(revindex);

    /* map lengthlist and vallist with index */
    b->lengthlist=calloc(b->entries,sizeof(long));
    b->valuelist=malloc(sizeof(double)*b->entries*b->dim);
    b->quantlist=malloc(sizeof(long)*b->entries*b->dim);
    for(i=0;i<b->entries;i++){
      long e=index[i];
      for(k=0;k<b->dim;k++){
	b->valuelist[i*b->dim+k]=v->entrylist[e*b->dim+k];
	b->quantlist[i*b->dim+k]=quantlist[e*b->dim+k];
      }
      b->lengthlist[i]=wordlen[e];
    }

    free(wordlen);
  }

  /* generate the codewords (short to long) */
  {
    long current=0;
    long length=0;
    b->codelist=malloc(sizeof(long)*b->entries);
    for(i=0;i<b->entries;i++){
      if(length != b->lengthlist[i]){
	current<<=(b->lengthlist[i]-length);
	length=b->lengthlist[i];
      }
      b->codelist[i]=current;
      fprintf(stderr,"codeword %ld: %ld (length %d)\n",
	      i, current, b->lengthlist[i]);
      current++;
    }
  }

  /* sanity check the codewords */
  for(i=0;i<b->entries;i++){
    for(j=i+1;j<b->entries;j++){
      if(b->codelist[i]==b->codelist[j]){
	fprintf(stderr,"Error; codewords for %ld and %ld both equal %lx\n",
		i, j, b->codelist[i]);
	exit(1);
      }
    }
  }

}

int vqenc_entry(vqbook *b,double *val){
  int ptr=0,k;
  while(1){
    double c= -b->c[ptr];
    double *nptr=b->n+b->dim*ptr;
    for(k=0;k<b->dim;k++)
      c+=nptr[k]*val[k];
    if(c>0.) /* in A */
      ptr= -b->ptr0[ptr];
    else     /* in B */
      ptr= -b->ptr1[ptr];
    if(ptr<=0)break;
  }
  return(-ptr);
}




