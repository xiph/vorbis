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

 function: build a VQ codebook and the encoding decision 'tree'
 last mod: $Id: vqsplit.c,v 1.18 2000/02/23 09:10:13 xiphmont Exp $

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
#include <sys/time.h>

#include "vqgen.h"
#include "vqsplit.h"
#include "bookutil.h"

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

/* grab it from vqgen.c */
extern double _dist(vqgen *v,double *a, double *b);

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
  /* keep the entries in ascending order (relative to the original
     list); we rely on that stability when ordering p/q choice */
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

int lp_split(vqgen *v,codebook *b,
	     long *entryindex,long entries, 
	     long *pointindex,long points,
	     long depth, long *pointsofar){

  encode_aux *t=b->c->encode_tree;

  /* The encoder, regardless of book, will be using a straight
     euclidian distance-to-point metric to determine closest point.
     Thus we split the cells using the same (we've already trained the
     codebook set spacing and distribution using special metrics and
     even a midpoint division won't disturb the basic properties) */

  long ret;
  double *n=alloca(sizeof(double)*v->elements);
  double c;
  long *entryA=calloc(entries,sizeof(long));
  long *entryB=calloc(entries,sizeof(long));
  long entriesA=0;
  long entriesB=0;
  long entriesC=0;
  long pointsA=0;
  long i,j,k;

  long *membership=malloc(sizeof(long)*points);

  long   besti=-1;
  long   bestj=-1;

  char spinbuf[80];
  sprintf(spinbuf,"splitting [%ld left]... ",v->points-*pointsofar);

  if(depth==22 && points==9 && entries==2 && *pointsofar==252935){
    fprintf(stderr,"HERE\n");

  }

  
  /* which cells do points belong to?  Do this before n^2 best pair chooser. */

  for(i=0;i<points;i++){
    double *ppt=_point(v,pointindex[i]);
    long   firstentry=0;
    double firstmetric=_dist(v,_now(v,entryindex[0]),ppt);
    
    if(points*entries>64*1024)spinnit(spinbuf,entries);

    for(j=1;j<entries;j++){
      double thismetric=_dist(v,_now(v,entryindex[j]),ppt);
      if(thismetric<=firstmetric){ /* Not <; on the line goes to higher number */
	firstmetric=thismetric;
	firstentry=j;
      }
    }
    
    membership[i]=firstentry;
  }

  /* We need to find the dividing hyperplane. find the median of each
     axis as the centerpoint and the normal facing farthest point */

  /* more than one way to do this part.  For small sets, we can brute
     force it. */

  if(entries<8 || (double)points*entries*entries<16.*1024*1024){
    /* try every pair possibility */
    double best=0;
    double this;
    for(i=0;i<entries-1;i++){
      for(j=i+1;j<entries;j++){
	spinnit(spinbuf,entries-i);
	pq_in_out(v,n,&c,_now(v,entryindex[i]),_now(v,entryindex[j]));
	vqsp_count(v,membership,
		   entryindex,entries, 
		   pointindex,points,
		   entryA,entryB,
		   n, c, 
		   &entriesA,&entriesB,&entriesC);
	this=(entriesA-entriesC)*(entriesB-entriesC);

	/* when choosing best, we also want some form of stability to
           make sure more branches are pared later; secondary
           weighting isn;t needed as the entry lists are in ascending
           order, and we always try p/q in the same sequence */
	
	if( (besti==-1) ||
	    (this>best) ){
	  
	  best=this;
	  besti=entryindex[i];
	  bestj=entryindex[j];

	}
      }
    }
  }else{
    double *p=alloca(v->elements*sizeof(double));
    double *q=alloca(v->elements*sizeof(double));
    double best=0.;
    
    /* try COG/normal and furthest pairs */
    /* meanpoint */
    /* eventually, we want to select the closest entry and figure n/c
       from p/q (because storing n/c is too large */
    for(k=0;k<v->elements;k++){
      spinnit(spinbuf,entries);
      
      p[k]=0.;
      for(j=0;j<entries;j++)
	p[k]+=v->entrylist[entryindex[j]*v->elements+k];
      p[k]/=entries;

    }

    /* we go through the entries one by one, looking for the entry on
       the other side closest to the point of reflection through the
       center */

    for(i=0;i<entries;i++){
      double *ppi=_now(v,entryindex[i]);
      double ref_best=0.;
      double ref_j=-1;
      double this;
      spinnit(spinbuf,entries-i);
      
      for(k=0;k<v->elements;k++)
	q[k]=2*p[k]-ppi[k];

      for(j=0;j<entries;j++){
	if(j!=i){
	  double this=_dist(v,q,_now(v,entryindex[j]));
	  if(ref_j==-1 || this<=ref_best){ /* <=, not <; very important */
	    ref_best=this;
	    ref_j=entryindex[j];
	  }
	}
      }

      pq_in_out(v,n,&c,ppi,_now(v,ref_j));
      vqsp_count(v,membership,
		 entryindex,entries, 
		 pointindex,points,
		 entryA,entryB,
		 n, c, 
		 &entriesA,&entriesB,&entriesC);
      this=(entriesA-entriesC)*(entriesB-entriesC);

	/* when choosing best, we also want some form of stability to
           make sure more branches are pared later; secondary
           weighting isn;t needed as the entry lists are in ascending
           order, and we always try p/q in the same sequence */
	
      if( (besti==-1) ||
	  (this>best) ){
	
	best=this;
	besti=entryindex[i];
	bestj=ref_j;

      }
    }
    if(besti>bestj){
      long temp=besti;
      besti=bestj;
      bestj=temp;
    }

  }
  
  /* find cells enclosing points */
  /* count A/B points */

  pq_in_out(v,n,&c,_now(v,besti),_now(v,bestj));
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

  /*  fprintf(stderr,"split: total=%ld depth=%ld set A=%ld:%ld:%ld=B\n",
      entries,depth,entriesA-entriesC,entriesC,entriesB-entriesC);*/
  {
    long thisaux=t->aux++;
    if(t->aux>=t->alloc){
      t->alloc*=2;
      t->ptr0=realloc(t->ptr0,sizeof(long)*t->alloc);
      t->ptr1=realloc(t->ptr1,sizeof(long)*t->alloc);
      t->p=realloc(t->p,sizeof(long)*t->alloc);
      t->q=realloc(t->q,sizeof(long)*t->alloc);
    }
    
    t->p[thisaux]=besti;
    t->q[thisaux]=bestj;
    
    if(entriesA==1){
      ret=1;
      t->ptr0[thisaux]=entryA[0];
      *pointsofar+=pointsA;
    }else{
      t->ptr0[thisaux]= -t->aux;
      ret=lp_split(v,b,entryA,entriesA,pointindex,pointsA,depth+1,pointsofar); 
    }
    if(entriesB==1){
      ret++;
      t->ptr1[thisaux]=entryB[0];
      *pointsofar+=points-pointsA;
    }else{
      t->ptr1[thisaux]= -t->aux;
      ret+=lp_split(v,b,entryB,entriesB,pointindex+pointsA,points-pointsA,
		    depth+1,pointsofar); 
    }
  }
  free(entryA);
  free(entryB);
  return(ret);
}

static int _node_eq(encode_aux *v, long a, long b){
  long    Aptr0=v->ptr0[a];
  long    Aptr1=v->ptr1[a];
  long    Bptr0=v->ptr0[b];
  long    Bptr1=v->ptr1[b];

  /* the possibility of choosing the same p and q, but switched, can;t
     happen because we always look for the best p/q in the same search
     order and the search is stable */

  if(Aptr0==Bptr0 && Aptr1==Bptr1)
    return(1);

  return(0);
}

void vqsp_book(vqgen *v, codebook *b, long *quantlist){
  long i,j;
  static_codebook *c=(static_codebook *)b->c;
  encode_aux *t;

  memset(b,0,sizeof(codebook));
  memset(c,0,sizeof(static_codebook));
  b->c=c;
  t=c->encode_tree=calloc(1,sizeof(encode_aux));

  /* make sure there are no duplicate entries and that every 
     entry has points */

  for(i=0;i<v->entries;){
    /* duplicate? if so, eliminate */
    for(j=0;j<i;j++){
      if(_dist(v,_now(v,i),_now(v,j))==0.){
	fprintf(stderr,"found a duplicate entry!  removing...\n");
	v->entries--;
	memcpy(_now(v,i),_now(v,v->entries),sizeof(double)*v->elements);
	memcpy(quantlist+i*v->elements,quantlist+v->entries*v->elements,
	       sizeof(long)*v->elements);
	break;
      }
    }
    if(j==i)i++;
  }

  {
    v->assigned=calloc(v->entries,sizeof(long));
    for(i=0;i<v->points;i++){
      double *ppt=_point(v,i);
      double firstmetric=_dist(v,_now(v,0),ppt);
      long   firstentry=0;

      if(!(i&0xff))spinnit("checking... ",v->points-i);

      for(j=0;j<v->entries;j++){
	double thismetric=_dist(v,_now(v,j),ppt);
	if(thismetric<firstmetric){
	  firstmetric=thismetric;
	  firstentry=j;
	}
      }
      
      v->assigned[firstentry]++;
    }

    for(j=0;j<v->entries;){
      if(v->assigned[j]==0){
	fprintf(stderr,"found an unused entry!  removing...\n");
	v->entries--;
	memcpy(_now(v,j),_now(v,v->entries),sizeof(double)*v->elements);
	v->assigned[j]=v->assigned[v->elements];
	memcpy(quantlist+j*v->elements,quantlist+v->entries*v->elements,
	       sizeof(long)*v->elements);
	continue;
      }
      j++;
    }
  }

  fprintf(stderr,"Building a book with %ld unique entries...\n",v->entries);

  {
    long *entryindex=malloc(v->entries*sizeof(long *));
    long *pointindex=malloc(v->points*sizeof(long));
    long pointssofar=0;
      
    for(i=0;i<v->entries;i++)entryindex[i]=i;
    for(i=0;i<v->points;i++)pointindex[i]=i;

    t->alloc=4096;
    t->ptr0=malloc(sizeof(long)*t->alloc);
    t->ptr1=malloc(sizeof(long)*t->alloc);
    t->p=malloc(sizeof(long)*t->alloc);
    t->q=malloc(sizeof(long)*t->alloc);
    t->aux=0;
    c->dim=v->elements;
    c->entries=v->entries;
    c->lengthlist=calloc(c->entries,sizeof(long));
    
    fprintf(stderr,"Leaves added: %d              \n",
	    lp_split(v,b,entryindex,v->entries,
		     pointindex,v->points,0,&pointssofar));
      
    free(pointindex);
    
    fprintf(stderr,"Paring/rerouting redundant branches... ");
    
    /* The tree is likely big and redundant.  Pare and reroute branches */
    {
      int changedflag=1;
      
      while(changedflag){
	changedflag=0;
	
	/* span the tree node by node; list unique decision nodes and
	   short circuit redundant branches */
	
	for(i=0;i<t->aux;){
	  int k;
	  
	  /* check list of unique decisions */
	  for(j=0;j<i;j++)
	    if(_node_eq(t,i,j))break;
	  
	  if(j<i){
	    /* a redundant entry; find all higher nodes referencing it and
	       short circuit them to the previously noted unique entry */
	    changedflag=1;
	    for(k=0;k<t->aux;k++){
	      if(t->ptr0[k]==-i)t->ptr0[k]=-j;
	      if(t->ptr1[k]==-i)t->ptr1[k]=-j;
	    }
	    
	    /* Now, we need to fill in the hole from this redundant
	       entry in the listing.  Insert the last entry in the list.
	       Fix the forward pointers to that last entry */
	    t->aux--;
	    t->ptr0[i]=t->ptr0[t->aux];
	    t->ptr1[i]=t->ptr1[t->aux];
	    t->p[i]=t->p[t->aux];
	    t->q[i]=t->q[t->aux];
	    for(k=0;k<t->aux;k++){
	      if(t->ptr0[k]==-t->aux)t->ptr0[k]=-i;
	      if(t->ptr1[k]==-t->aux)t->ptr1[k]=-i;
	    }
	    /* hole plugged */
	    
	  }else
	    i++;
	}
	
	fprintf(stderr,"\rParing/rerouting redundant branches... "
		"%ld remaining   ",t->aux);
      }
      fprintf(stderr,"\n");
    }
  }
  
  /* run all training points through the decision tree to get a final
     probability count */
  {
    long *probability=malloc(c->entries*sizeof(long));
    for(i=0;i<c->entries;i++)probability[i]=1; /* trivial guard */
    b->valuelist=v->entrylist; /* temporary for vqenc_entry; replaced later */

    /* sigh.  A necessary hack */
    for(i=0;i<t->aux;i++)t->p[i]*=c->dim;
    for(i=0;i<t->aux;i++)t->q[i]*=c->dim;
    
    for(i=0;i<v->points;i++){
      int ret=codebook_entry(b,v->pointlist+i*v->elements);
      probability[ret]++;
      if(!(i&0xff))spinnit("counting hits... ",v->points-i);
    }
    for(i=0;i<t->aux;i++)t->p[i]/=c->dim;
    for(i=0;i<t->aux;i++)t->q[i]/=c->dim;

    build_tree_from_lengths(c->entries,probability,c->lengthlist);
    
    free(probability);
  }

  /* Sort the entries by codeword length, short to long (eases
     assignment and packing to do it now) */
  {
    long *wordlen=c->lengthlist;
    long *index=malloc(c->entries*sizeof(long));
    long *revindex=malloc(c->entries*sizeof(long));
    int k;
    for(i=0;i<c->entries;i++)index[i]=i;
    isortvals=c->lengthlist;
    qsort(index,c->entries,sizeof(long),iascsort);

    /* rearrange storage; ptr0/1 first as it needs a reverse index */
    /* n and c stay unchanged */
    for(i=0;i<c->entries;i++)revindex[index[i]]=i;
    for(i=0;i<t->aux;i++){
      if(!(i&0x3f))spinnit("sorting... ",t->aux-i);

      if(t->ptr0[i]>=0)t->ptr0[i]=revindex[t->ptr0[i]];
      if(t->ptr1[i]>=0)t->ptr1[i]=revindex[t->ptr1[i]];
      t->p[i]=revindex[t->p[i]];
      t->q[i]=revindex[t->q[i]];
    }
    free(revindex);

    /* map lengthlist and vallist with index */
    c->lengthlist=calloc(c->entries,sizeof(long));
    b->valuelist=malloc(sizeof(double)*c->entries*c->dim);
    c->quantlist=malloc(sizeof(long)*c->entries*c->dim);
    for(i=0;i<c->entries;i++){
      long e=index[i];
      for(k=0;k<c->dim;k++){
	b->valuelist[i*c->dim+k]=v->entrylist[e*c->dim+k];
	c->quantlist[i*c->dim+k]=quantlist[e*c->dim+k];
      }
      c->lengthlist[i]=wordlen[e];
    }

    free(wordlen);
  }

  fprintf(stderr,"Done.                            \n\n");
}

