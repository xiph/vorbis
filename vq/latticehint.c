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

 function: utility main for building thresh/pigeonhole encode hints
 last mod: $Id: latticehint.c,v 1.1.2.2 2000/08/04 05:15:09 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vorbis/codebook.h"
#include "../lib/sharedbook.h"
#include "bookutil.h"
#include "vqgen.h"
#include "vqsplit.h"

/* The purpose of this util is to build encode hints for lattice
   codebooks so that brute forcing each codebook entry isn't needed.
   Threshhold hints are for books in which each scalar in the vector
   is independant (eg, residue) and pigeonhole lookups provide a
   minimum error fit for words where the scalars are interdependant
   (each affecting the fit of the next in sequence) as in an LSP
   sequential book (or can be used along with a sparse threshhold map,
   like a splitting tree that need not be trained) 

   If the input book is non-sequential, a threshhold hint is built.
   If the input book is sequential, a pigeonholing hist is built.
   If the book is sparse, a pigeonholing hint is built, possibly in addition
     to the threshhold hint 

   command line:
   latticehint book.vqh

   latticehint produces book.vqh on stdout */

static int longsort(const void *a, const void *b){
  return(**((long **)a)-**((long **)b));
}

static int addtosearch(int entry,long **tempstack,long *tempcount,int add){
  long *ptr=tempstack[entry];
  long i=tempcount[entry];

  if(ptr){
    while(i--)
      if(*ptr++==add)return(0);
    tempstack[entry]=realloc(tempstack[entry],
			     (tempcount[entry]+1)*sizeof(long));
  }else{
    tempstack[entry]=malloc(sizeof(long));
  }

  tempstack[entry][tempcount[entry]++]=add;
  return(1);
}

static void setvals(int dim,encode_aux_pigeonhole *p,
		    long *temptrack,double *tempmin,double *tempmax,
		    int seqp){
  int i;
  double last=0.;
  for(i=0;i<dim;i++){
    tempmin[i]=(temptrack[i])*p->del+p->min+last;
    tempmax[i]=tempmin[i]+p->del;
    if(seqp)last=tempmin[i];
  }
}

/* note that things are currently set up such that input fits that
   quantize outside the pigeonmap are dropped and brute-forced.  So we
   can ignore the <0 and >=n boundary cases in min/max error */

static double minerror(int dim,double *a,encode_aux_pigeonhole *p,
		       long *temptrack,double *tempmin,double *tempmax){
  int i;
  double err=0.;
  for(i=0;i<dim;i++){
    double eval=0.;
    if(a[i]<tempmin[i]){
      eval=tempmin[i]-a[i];
    }else if(a[i]>tempmax[i]){
      eval=a[i]-tempmax[i];
    }
    err+=eval*eval;
  }
  return(err);
}

static double maxerror(int dim,double *a,encode_aux_pigeonhole *p,
		       long *temptrack,double *tempmin,double *tempmax){
  int i;
  double err=0.,eval;
  for(i=0;i<dim;i++){
    if(a[i]<tempmin[i]){
      eval=tempmax[i]-a[i];
    }else if(a[i]>tempmax[i]){
      eval=a[i]-tempmin[i];
    }else{
      double t1=a[i]-tempmin[i];
      eval=tempmax[i]-a[i];
      if(t1>eval)eval=t1;
    }
    err+=eval*eval;
  }
  return(err);
}

static double _dist(double *ref, double *b,int el){
  int i;
  double acc=0.;
  for(i=0;i<el;i++){
    double val=(ref[i]-b[i]);
    acc+=val*val;
  }
  return(acc);
}

int main(int argc,char *argv[]){
  codebook *b;
  static_codebook *c;
  int entries=-1,dim=-1;
  double min,del,cutoff=1.;
  char *name;
  long i,j;

  if(argv[1]==NULL){
    fprintf(stderr,"Need a lattice book on the command line.\n");
    exit(1);
  }

  if(argv[2])cutoff=atof(argv[2]);

  {
    char *ptr;
    char *filename=strdup(argv[1]);

    b=codebook_load(filename);
    c=(static_codebook *)(b->c);
    
    ptr=strrchr(filename,'.');
    if(ptr){
      *ptr='\0';
      name=strdup(filename);
    }else{
      name=strdup(filename);
    }
  }

  if(c->maptype!=1){
    fprintf(stderr,"Provided book is not a latticebook.\n");
    exit(1);
  }

  entries=b->entries;
  dim=b->dim;
  min=_float32_unpack(c->q_min);
  del=_float32_unpack(c->q_delta);

  /* Do we want to gen a threshold hint? */
  if(c->q_sequencep==0){
    /* yes. Discard any preexisting threshhold hint */
    long quantvals=_book_maptype1_quantvals(c);
    long **quantsort=alloca(quantvals*sizeof(long *));
    encode_aux_threshmatch *t=calloc(1,sizeof(encode_aux_threshmatch));
    c->thresh_tree=t;

    fprintf(stderr,"Adding threshold hint to %s...\n",name);

    /* simplest possible threshold hint only */
    t->quantthresh=calloc(quantvals-1,sizeof(double));
    t->quantmap=calloc(quantvals,sizeof(int));
    t->threshvals=quantvals;
    t->quantvals=quantvals;

    /* the quantvals may not be in order; sort em first */
    for(i=0;i<quantvals;i++)quantsort[i]=c->quantlist+i;
    qsort(quantsort,quantvals,sizeof(long *),longsort);

    /* ok, gen the map and thresholds */
    for(i=0;i<quantvals;i++)t->quantmap[i]=quantsort[i]-c->quantlist;
    for(i=0;i<quantvals-1;i++)
      t->quantthresh[i]=(*(quantsort[i])+*(quantsort[i+1]))*.5*del+min;
  }

  /* do we want a split-tree hint? */
  if(1){
    long quantvals=_book_maptype1_quantvals(c);
    int fac=4;
    int over=4;
    double ldel=del/fac;
    double lmin=min-ldel*fac*over-ldel*.5;
    long max=quantvals*fac+over*2;
    long points=1;
    long *entryindex=malloc(c->entries*sizeof(long *));
    long *pointindex;
    long *membership;
    long *reventry;
    double *pointlist;
    long pointssofar=0;
    encode_aux_nearestmatch *t;

    for(i=0;i<dim;i++)points*=max;

    t=c->nearest_tree=calloc(1,sizeof(encode_aux_nearestmatch));
  
    pointindex=malloc(points*sizeof(long));
    pointlist=malloc(points*dim*sizeof(double));
    membership=malloc(points*sizeof(long));
    reventry=malloc(c->entries*sizeof(long));
      
    for(i=0;i<c->entries;i++)entryindex[i]=i;

    /* set points on a fine mesh */
    {
      long k=0;
      long *temptrack=calloc(dim,sizeof(long));
      for(i=0;i<points;i++){
	double last=0.;
	pointindex[i]=i;

	for(j=0;j<dim;j++){
	  pointlist[k]=temptrack[j]*ldel+lmin+last;
	  if(c->q_sequencep)last=pointlist[k];
	  k++;
	}
	for(j=0;j<dim;j++){
	  temptrack[j]++;
	  if(temptrack[j]<max)break;
	  temptrack[j]=0;
	}
      }
    }

    t->alloc=4096;
    t->ptr0=malloc(sizeof(long)*t->alloc);
    t->ptr1=malloc(sizeof(long)*t->alloc);
    t->p=malloc(sizeof(long)*t->alloc);
    t->q=malloc(sizeof(long)*t->alloc);
    t->aux=0;

    for(i=0;i<points;i++)membership[i]=-1;
    for(i=0;i<points;i++){
      double *ppt=pointlist+i*dim;
      long   firstentry=0;
      double firstmetric=_dist(b->valuelist,ppt,dim);
    
      if(!(i&0xff))spinnit("assigning... ",points-i);

      for(j=1;j<b->entries;j++){
	if(c->lengthlist[j]>0){
	  double thismetric=_dist(b->valuelist+j*dim,ppt,dim);
	  if(thismetric<=firstmetric){
	    firstmetric=thismetric;
	    firstentry=j;
	  }
	}
      }
      
      membership[i]=firstentry;
    }

    fprintf(stderr,"Leaves added: %d              \n",
	    lp_split(pointlist,points,
		     b,entryindex,b->entries,
		     pointindex,points,
		     membership,reventry,
		     0,&pointssofar));
      
    free(pointlist);
    free(pointindex);
    free(entryindex);
    free(membership);
    free(reventry);
  }

  /* Do we want to gen a pigeonhole hint? */
  for(i=0;i<entries;i++)if(c->lengthlist[i]==0)break;
  if(0){/*if(c->q_sequencep || i<entries){*/
    long **tempstack;
    long *tempcount;
    long *temptrack;
    double *tempmin;
    double *tempmax;
    long totalstack=0;
    long pigeons;
    long subpigeons;
    long quantvals=_book_maptype1_quantvals(c);
    int changep=1;

    encode_aux_pigeonhole *p=calloc(1,sizeof(encode_aux_pigeonhole));
    c->pigeon_tree=p;

    fprintf(stderr,"Adding pigeonhole hint to %s...\n",name);
    
    /* the idea is that we quantize uniformly, even in a nonuniform
       lattice, so that quantization of one scalar has a predictable
       result on the next sequential scalar in a greedy matching
       algorithm.  We generate a lookup based on the quantization of
       the vector (pigeonmap groups quantized entries together) and
       list the entries that could possible be the best fit for any
       given member of that pigeonhole.  The encode process then has a
       much smaller list to brute force */

    /* find our pigeonhole-specific quantization values, fill in the
       quant value->pigeonhole map */
    p->del=del;
    p->min=min-del*.5;
    p->quantvals=(quantvals+1)/2;
    {
      int max=0;
      for(i=0;i<quantvals;i++)if(max<c->quantlist[i])max=c->quantlist[i];
      p->mapentries=max;
    }
    p->pigeonmap=malloc(p->mapentries*sizeof(long));

    /* pigeonhole roughly on the boundaries of the quantvals; the
       exact pigeonhole grouping is an optimization issue, not a
       correctness issue */
    for(i=0;i<p->mapentries;i++){
      double thisval=del*(i+.5)+min; /* middle of the quant zone */
      int quant=0;
      double err=fabs(c->quantlist[0]*del+min-thisval);
      for(j=1;j<quantvals;j++){
	double thiserr=fabs(c->quantlist[j]*del+min-thisval);
	if(thiserr<err){
	  quant=j/2;
	  err=thiserr;
	}
      }
      p->pigeonmap[i]=quant;
    }
    
    /* pigeonmap complete.  Now do the grungy business of finding the
    entries that could possibly be the best fit for a value appearing
    in the pigeonhole. The trick that allows the below to work is the
    uniform quantization; even though the scalars may be 'sequential'
    (each a delta from the last), the uniform quantization means that
    the error variance is *not* dependant.  Given a pigeonhole and an
    entry, we can find the minimum and maximum possible errors
    (relative to the entry) for any point that could appear in the
    pigeonhole */
    
    /* must iterate over both pigeonholes and entries */
    /* temporarily (in order to avoid thinking hard), we grow each
       pigeonhole seperately, the build a stack of 'em later */
    pigeons=1;
    subpigeons=1;
    for(i=0;i<dim;i++)subpigeons*=p->mapentries;
    for(i=0;i<dim;i++)pigeons*=p->quantvals;
    temptrack=calloc(dim,sizeof(long));
    tempmin=calloc(dim,sizeof(double));
    tempmax=calloc(dim,sizeof(double));
    tempstack=calloc(pigeons,sizeof(long *));
    tempcount=calloc(pigeons,sizeof(long));

    while(1){
      double errorpost=-1;
      char buffer[80];

      /* map our current pigeonhole to a 'big pigeonhole' so we know
         what list we're after */
      int entry=0;
      for(i=dim-1;i>=0;i--)entry=entry*p->quantvals+p->pigeonmap[temptrack[i]];
      setvals(dim,p,temptrack,tempmin,tempmax,c->q_sequencep);
      sprintf(buffer,"Building pigeonhole search list [%ld]...",totalstack);


      /* Search all entries to find the one with the minimum possible
         maximum error.  Record that error */
      for(i=0;i<entries;i++){
	if(c->lengthlist[i]>0){
	  double this=maxerror(dim,b->valuelist+i*dim,p,
			       temptrack,tempmin,tempmax);
	  if(errorpost==-1 || this<errorpost)errorpost=this;
	  spinnit(buffer,subpigeons);
	}
      }

      /* Our search list will contain all entries with a minimum
         possible error <= our errorpost */
      for(i=0;i<entries;i++)
	if(c->lengthlist[i]>0){
	  spinnit(buffer,subpigeons);
	  if(minerror(dim,b->valuelist+i*dim,p,
		      temptrack,tempmin,tempmax)<errorpost)
	    totalstack+=addtosearch(entry,tempstack,tempcount,i);
	}

      for(i=0;i<dim;i++){
	temptrack[i]++;
	if(temptrack[i]<p->mapentries)break;
	temptrack[i]=0;
      }
      if(i==dim)break;
      subpigeons--;
    }

    fprintf(stderr,"\r                                                     "
	    "\rTotal search list size (all entries): %ld\n",totalstack);

    /* pare the index of lists for improbable quantizations (where
       improbable is determined by c->lengthlist; we assume that
       pigeonholing is in sync with the codeword cells, which it is */
    /*for(i=0;i<entries;i++){
      double probability= 1./(1<<c->lengthlist[i]);
      if(c->lengthlist[i]==0 || probability*entries<cutoff){
	totalstack-=tempcount[i];
	tempcount[i]=0;
      }
      }*/

    /* pare the list of shortlists; merge contained and similar lists
       together */
    p->fitmap=malloc(pigeons*sizeof(long));
    for(i=0;i<pigeons;i++)p->fitmap[i]=-1;
    while(changep){
      char buffer[80];
      changep=0;

      for(i=0;i<pigeons;i++){
	if(p->fitmap[i]<0 && tempcount[i]){
	  for(j=i+1;j<pigeons;j++){
	    if(p->fitmap[j]<0 && tempcount[j]){
	      /* is one list a superset, or are they sufficiently similar? */
	      int amiss=0,bmiss=0,ii,jj;
	      for(ii=0;ii<tempcount[i];ii++){
		for(jj=0;jj<tempcount[j];jj++)
		  if(tempstack[i][ii]==tempstack[j][jj])break;
		if(jj==tempcount[j])amiss++;
	      }
	      for(jj=0;jj<tempcount[j];jj++){
		for(ii=0;ii<tempcount[i];ii++)
		  if(tempstack[i][ii]==tempstack[j][jj])break;
		if(ii==tempcount[i])bmiss++;
	      }
	      if(amiss==0 ||
		 bmiss==0 ||
		 (amiss*2<tempcount[i] && bmiss*2<tempcount[j] &&
		  tempcount[i]+bmiss<entries/30)){

		/*superset/similar  Add all of one to the other. */
		for(jj=0;jj<tempcount[j];jj++)
		  totalstack+=addtosearch(i,tempstack,tempcount,
					  tempstack[j][jj]);
		totalstack-=tempcount[j];
		p->fitmap[j]=i;
		changep=1;
	      }
	    }
	  }
	  sprintf(buffer,"Consolidating [%ld total, %s]... ",totalstack,
		  changep?"reit":"nochange");
	  spinnit(buffer,pigeons-i);
	}
      }
    }

    /* repack the temp stack in final form */

    p->fittotal=totalstack;
    p->fitlist=malloc((totalstack+1)*sizeof(long));
    p->fitlength=malloc(pigeons*sizeof(long));
    {
      long usage=0;
      for(i=0;i<pigeons;i++){
	if(p->fitmap[i]==-1){
	  if(tempcount[i])
	    memcpy(p->fitlist+usage,tempstack[i],tempcount[i]*sizeof(long));
	  p->fitmap[i]=usage;
	  p->fitlength[i]=tempcount[i];
	  usage+=tempcount[i];
	  if(usage>totalstack){
	    fprintf(stderr,"Internal error; usage>totalstack\n");
	    exit(1);
	  }
	}else{
	  p->fitlength[i]=p->fitlength[p->fitmap[i]];
	  p->fitmap[i]=p->fitmap[p->fitmap[i]];
	}
      }
    }
  }

  write_codebook(stdout,name,c); 
  fprintf(stderr,"\r                                                     "
	  "\nDone.\n");
  exit(0);
}
