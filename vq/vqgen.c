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
 last modification date: Dec 08 1999

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
#include "minit.h"

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

typedef struct vqgen{
  int it;

  int    elements;
  double errspread;

  /* point cache */
  double *pointlist; 
  long   *first;
  long   *second;
  long   points;
  long   allocated;

  /* entries */
  double *entrylist;
  long   *assigned;
  double *bias;
  long   entries;

  double (*metric_func)   (struct vqgen *v,double *a,double *b);
} vqgen;

typedef struct vqbook{



} vqbook;

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

/* A metric for LSP codes */
                            /* candidate,actual */
double _dist_and_pos(vqgen *v,double *b, double *a){
  int i;
  int el=v->elements;
  double acc=0.;
  double lastb=0.;
  for(i=0;i<el;i++){
    double actualdist=(a[i]-lastb);
    double testdist=(b[i]-lastb);
    if(actualdist>0 && testdist>0){
      double val;
      if(actualdist>testdist)
	val=actualdist/testdist-1.;
      else
	val=testdist/actualdist-1.;
      acc+=val;
    }else{
      acc+=999999.;
    }
    lastb=b[i];
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
		double spread){
  memset(v,0,sizeof(vqgen));

  v->elements=elements;
  v->errspread=spread;
  v->allocated=32768;
  v->pointlist=malloc(v->allocated*v->elements*sizeof(double));
  v->first=malloc(v->allocated*sizeof(long));
  v->second=malloc(v->allocated*sizeof(long));

  v->entries=entries;
  v->entrylist=malloc(v->entries*v->elements*sizeof(double));
  v->assigned=malloc(v->entries*sizeof(long));
  v->bias=calloc(v->entries,sizeof(double));
  if(metric)
    v->metric_func=metric;
  else
    v->metric_func=_dist_and_pos;
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

  /* last, assign midpoints */
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

  fprintf(stderr,": dist %g(%g) metric error=%g \n",
	  asserror/v->entries,fdesired,meterror/v->points);
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

int lp_split(vqgen *v,long *entryindex,long entries, 
	     long *pointindex,long points,long depth,
	     double slack){

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

  /* depth limit */
  if(depth>20){
    printf("leaf: entries %ld, depth %ld\n",entries,depth);
    return(entries);
  }

  /* nothing to do */
  if(entries==1){
    printf("leaf: entry %ld, depth %ld\n",entryindex[0],depth);
    return(1);
  }

  /* The result must be an even split */
  if(entries==2){
    printf("even split: depth %ld\n",depth);
    return(2);
  }

  /* We need to find the dividing hyperplane. find the median of each
     axis as the centerpoint and the normal facing farthest point */

  /* more than one way to do this part.  For small sets, we can brute
     force it. */

  if(entries<64){
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
    while(Aptr<Bptr){
      while(Aptr<Bptr){
	double position=-c;
	for(k=0;k<v->elements;k++)
	  position+=_point(v,pointindex[Aptr])[k]*n[k];
	if(position<0.)break; /* not in A */
	Aptr++;
      }
      while(Aptr<Bptr){
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
      Aptr++;
      Bptr--;
    }
  }

  fprintf(stderr,"split: total=%ld depth=%ld set A=%ld:%ld:%ld=B\n",
	  entries,depth,entriesA-entriesC,entriesC,entriesB-entriesC);

  ret=lp_split(v,entryA,entriesA,pointindex,pointsA,depth+1,slack); 
  ret+=lp_split(v,entryB,entriesB,pointindex+pointsA,points-pointsA,
		depth+1,slack); 
  return(ret);
}

void vqgen_book(vqgen *v){




}

static double testset24[48]={

};

static double testset256[1024]={
0.334427,0.567149,0.749324,0.838460,
0.212558,0.435792,0.661945,0.781195,
0.316791,0.551190,0.700236,0.813437,
0.240661,0.398754,0.570649,0.666909,
0.233216,0.549751,0.715668,0.844393,
0.275589,0.460791,0.687872,0.786855,
0.145195,0.478688,0.678451,0.788536,
0.332491,0.577111,0.770913,0.875295,
0.162683,0.250422,0.399770,0.688120,
0.301578,0.424074,0.595725,0.829533,
0.118704,0.276233,0.492329,0.605532,
0.240679,0.468862,0.670704,0.809156,
0.109799,0.227123,0.533265,0.686982,
0.119657,0.423823,0.597676,0.710405,
0.207444,0.437121,0.671331,0.809054,
0.222787,0.324523,0.466905,0.652758,
0.117279,0.268017,0.410036,0.593785,
0.316531,0.493867,0.642588,0.815791,
0.218069,0.315630,0.554662,0.749348,
0.152218,0.412864,0.664135,0.814275,
0.144242,0.221503,0.457281,0.631403,
0.290454,0.458806,0.603994,0.751802,
0.236719,0.341259,0.586677,0.707653,
0.257617,0.406941,0.602074,0.696609,
0.141833,0.254061,0.523788,0.701270,
0.136685,0.333920,0.515585,0.642137,
0.191560,0.485163,0.665121,0.755073,
0.086062,0.375207,0.623168,0.763659,
0.184017,0.404999,0.609708,0.746118,
0.208200,0.414804,0.618734,0.732746,
0.184643,0.390701,0.666137,0.791445,
0.216594,0.519656,0.713291,0.810097,
0.118095,0.361088,0.577184,0.667553,
0.243858,0.394640,0.550682,0.774866,
0.266410,0.430656,0.629812,0.729035,
0.140187,0.323258,0.534031,0.762617,
0.097902,0.230404,0.382920,0.707036,
0.197244,0.424752,0.578097,0.741857,
0.216830,0.460883,0.622823,0.747350,
0.096586,0.252731,0.471721,0.766199,
0.385920,0.650477,0.846797,0.972222,
0.249895,0.442701,0.706660,0.824760,
0.207781,0.355197,0.538083,0.686972,
0.166064,0.261252,0.493243,0.642141,
0.146441,0.385197,0.595104,0.739789,
0.207555,0.311523,0.533963,0.666395,
0.172735,0.455702,0.624525,0.713503,
0.132351,0.420175,0.528877,0.716175,
0.113027,0.270164,0.462866,0.653668,
0.306970,0.575262,0.785717,0.942889,
0.122649,0.451363,0.652393,0.749624,
0.337459,0.561671,0.713605,0.878091,
0.154890,0.324078,0.469466,0.677888,
0.242478,0.420602,0.651274,0.762209,
0.162266,0.401114,0.639885,0.769255,
0.275493,0.500799,0.753756,0.884563,
0.219840,0.410217,0.634590,0.784979,
0.197822,0.418074,0.514919,0.784821,
0.176855,0.327821,0.624861,0.768299,
0.187133,0.373303,0.614062,0.764326,
0.148866,0.339151,0.571398,0.693017,
0.126516,0.364169,0.643363,0.784478,
0.176790,0.354609,0.553963,0.728466,
0.169151,0.294951,0.559738,0.718315,
0.189233,0.369137,0.615112,0.736337,
0.211442,0.459206,0.587994,0.715496,
0.203544,0.332755,0.496152,0.707495,
0.264950,0.377661,0.601633,0.738910,
0.144156,0.332387,0.533986,0.858065,
0.236705,0.493254,0.646422,0.780372,
0.247066,0.480770,0.725958,0.847099,
0.121273,0.415837,0.575449,0.785059,
0.109050,0.326623,0.585313,0.725416,
0.079853,0.281243,0.579608,0.721251,
0.164131,0.452931,0.608512,0.772505,
0.183958,0.382443,0.595812,0.708979,
0.232813,0.460979,0.612592,0.785098,
0.372632,0.625661,0.794886,0.915078,
0.197199,0.422499,0.630676,0.764479,
0.112259,0.228495,0.423574,0.529402,
0.313775,0.495449,0.684949,0.906438,
0.121923,0.272895,0.590304,0.762871,
0.114215,0.298370,0.523344,0.690963,
0.195814,0.446435,0.567172,0.888808,
0.292057,0.491911,0.707296,0.836253,
0.198244,0.410661,0.555814,0.699832,
0.152291,0.332174,0.609291,0.732500,
0.051002,0.303083,0.609333,0.769208,
0.086086,0.233029,0.475899,0.590504,
0.096449,0.366927,0.502638,0.761407,
0.256632,0.423960,0.650693,0.805566,
0.235739,0.542293,0.776812,0.939851,
0.210050,0.310752,0.439640,0.760752,
0.168588,0.352547,0.620034,0.811813,
0.217251,0.438798,0.704758,0.854804,
0.112327,0.304063,0.583490,0.813809,
0.183413,0.263207,0.503397,0.753610,
0.241265,0.367568,0.648774,0.795899,
0.262796,0.558878,0.784660,0.908486,
0.221425,0.479136,0.683794,0.830981,
0.325039,0.573201,0.837740,0.994924,
0.263655,0.636322,0.878818,1.052555,
0.228884,0.510746,0.742781,0.903443,
0.169799,0.354550,0.667261,0.880659,
0.429209,0.689908,0.942083,1.075538,
0.178843,0.393247,0.698620,0.846058,
0.410735,0.684575,0.900212,1.026610,
0.212082,0.407634,0.677794,0.824167,
0.183638,0.456984,0.701680,0.828091,
0.198668,0.290621,0.586525,0.796566,
0.212584,0.394786,0.624992,0.756258,
0.218545,0.327989,0.671635,0.851695,
0.118076,0.487280,0.758482,0.920421,
0.184608,0.426118,0.642211,0.797123,
0.248725,0.534657,0.737666,0.869414,
0.075312,0.218113,0.574791,0.767607,
0.184571,0.487955,0.742725,0.865892,
0.148818,0.284078,0.481156,0.816008,
0.205061,0.336518,0.506794,0.610053,
0.054274,0.238425,0.414471,0.577301,
0.082819,0.289140,0.632090,0.789039,
0.095066,0.249569,0.666867,0.857491,
0.129159,0.215969,0.524985,0.808056,
0.120949,0.473613,0.723959,0.844274,
0.225445,0.469240,0.781258,0.937501,
0.190737,0.487420,0.664112,0.798043,
0.249655,0.533041,0.808386,0.984218,
0.236544,0.473767,0.723955,0.872327,
0.093351,0.432384,0.672316,0.803956,
0.171295,0.491945,0.662536,0.844976,
0.165080,0.389674,0.533508,0.664579,
0.224994,0.356513,0.535555,0.640525,
0.158366,0.244279,0.437682,0.579242,
0.267249,0.423934,0.542629,0.635151,
0.105840,0.183727,0.395015,0.546705,
0.128701,0.214397,0.323019,0.569708,
0.205691,0.367797,0.491331,0.579727,
0.136376,0.295543,0.461283,0.567165,
0.090912,0.326863,0.449785,0.549052,
0.280052,0.463123,0.590556,0.676314,
0.053220,0.327610,0.496921,0.611028,
0.074906,0.374797,0.563476,0.691693,
0.064942,0.286131,0.414479,0.492806,
0.303210,0.504309,0.652008,0.744313,
0.322904,0.532050,0.682097,0.777012,
0.247437,0.397965,0.504882,0.680784,
0.076782,0.185773,0.355825,0.478750,
0.170565,0.428925,0.587157,0.683893,
0.186692,0.282933,0.468043,0.583405,
0.286847,0.489852,0.626934,0.714018,
0.196398,0.333200,0.447551,0.551070,
0.061757,0.242351,0.503337,0.676114,
0.119199,0.359308,0.514024,0.595332,
0.282369,0.428107,0.549797,0.721344,
0.120772,0.328758,0.465806,0.797635,
0.251676,0.385411,0.692374,0.930173,
0.265590,0.605988,0.828534,0.964088,
0.118910,0.347784,0.690580,0.839342,
0.148664,0.429126,0.564376,0.640949,
0.273239,0.395545,0.492952,0.803872,
0.085855,0.175468,0.449037,0.649337,
0.101341,0.351796,0.455442,0.645061,
0.181516,0.301506,0.381633,0.628656,
0.165340,0.385706,0.549717,0.787060,
0.102530,0.247439,0.362454,0.570843,
0.115626,0.291845,0.411639,0.730918,
0.130168,0.402586,0.531264,0.842151,
0.312057,0.497535,0.608141,0.933018,
0.061317,0.329143,0.457434,0.717368,
0.260419,0.364385,0.522931,0.733634,
0.287346,0.448695,0.703337,0.864549,
0.158190,0.329088,0.444748,0.609073,
0.057217,0.172765,0.500069,0.763990,
0.185582,0.346521,0.426838,0.724792,
0.152203,0.253061,0.580445,0.835384,
0.223681,0.376501,0.463553,0.758668,
0.153621,0.385013,0.482490,0.723052,
0.165862,0.300622,0.515314,0.628741,
0.087543,0.300967,0.529657,0.641654,
0.257693,0.367954,0.453859,0.654989,
0.077763,0.391224,0.586210,0.846828,
0.140653,0.296311,0.431419,0.524173,
0.276106,0.566792,0.723587,0.928366,
0.345768,0.530982,0.711197,1.011075,
0.328643,0.578965,0.775015,1.036448,
0.295489,0.451392,0.541007,0.831967,
0.249971,0.517649,0.662078,0.872210,
0.171802,0.382312,0.492482,0.629808,
0.192277,0.478000,0.611564,0.846436,
0.217769,0.335272,0.508986,0.813008,
0.265875,0.429911,0.625892,0.958544,
0.183329,0.299421,0.565539,0.885282,
0.311307,0.432098,0.756371,0.937483,
0.153908,0.264165,0.391052,0.502304,
0.267528,0.476930,0.862271,1.099421,
0.221527,0.513611,0.684819,0.993058,
0.220330,0.352263,0.575808,0.823017,
0.084526,0.372939,0.705268,0.879314,
0.179752,0.287765,0.507176,0.683204,
0.234005,0.381653,0.537092,0.887657,
0.068805,0.135538,0.363455,0.732055,
0.266267,0.440971,0.719076,1.025917,
0.169728,0.426510,0.670186,0.982939,
0.248443,0.463944,0.736167,0.898651,
0.074789,0.155652,0.601987,0.864470,
0.394203,0.665959,0.950879,1.183016,
0.056927,0.247163,0.620166,0.886961,
0.081110,0.260015,0.725344,0.969987,
0.187566,0.605658,0.907862,1.113592,
0.188174,0.417467,0.746757,0.912718,
0.214944,0.444902,0.813762,1.006999,
0.177305,0.322288,0.623538,0.972940,
0.108933,0.560083,0.892489,1.105198,
0.326010,0.486890,0.808596,1.012135,
0.507702,0.815903,1.085892,1.267201,
0.121027,0.368451,0.715029,1.000456,
0.424229,0.713942,0.971720,1.123569,
0.158148,0.411905,0.812352,1.057000,
0.330896,0.639677,0.814721,1.083593,
0.116143,0.314050,0.643242,0.928001,
0.228088,0.364437,0.747642,1.045211,
0.457076,0.783579,1.021362,1.174221,
0.069613,0.430727,0.845847,1.061835,
0.343281,0.553869,0.865545,1.077270,
0.152973,0.304560,0.642854,0.830441,
0.157602,0.568564,0.815800,1.036183,
0.104003,0.505075,0.796932,0.999392,
0.377220,0.622243,0.901761,1.072988,
0.060039,0.378222,0.710910,0.939836,
0.220206,0.539490,0.849893,1.056103,
0.116884,0.454544,0.627779,0.924877,
0.293087,0.633251,0.927781,1.114120,
0.234659,0.458666,0.693916,0.925308,
0.271802,0.385786,0.661508,0.862002,
0.185212,0.282842,0.431237,0.663853,
0.152441,0.280173,0.711942,0.961418,
0.264191,0.647776,0.976337,1.171963,
0.227340,0.567363,0.753334,1.049249,
0.173306,0.508941,0.761828,0.970236,
0.271942,0.496903,0.759621,0.963074,
0.187050,0.355811,0.752185,0.952287,
0.172003,0.363447,0.486011,0.829142,
0.364755,0.598200,0.868222,1.025048,
0.226504,0.432744,0.649002,0.852690,
0.200857,0.436929,0.651778,0.905448,
0.137860,0.385245,0.609566,0.867573,
0.217881,0.368030,0.601419,0.954770,
0.199237,0.398640,0.593487,0.830535,
0.324623,0.518989,0.782275,0.926822,
0.245185,0.419081,0.717432,0.883462,
0.277741,0.494080,0.765001,0.915227,
0.200418,0.463680,0.705028,0.875441,
0.171638,0.526057,0.697163,0.915452,
0.142351,0.416539,0.707280,0.876499,
0.178309,0.553184,0.777786,0.903243,
0.335361,0.540299,0.812589,0.965039,


};

int main(int argc,char *argv[]){
  FILE *in=fopen(argv[1],"r");
  vqgen v;
  char buffer[160];
  int i,j;

  vqgen_init(&v,4,256,_dist_and_pos,0.);
  
  while(fgets(buffer,160,in)){
    double a[8];
    if(sscanf(buffer,"%lf %lf %lf %lf",
	      a,a+1,a+2,a+3)==4)
      vqgen_addpoint(&v,a);
  }
  fclose(in);

  /*for(i=0;i<200;i++){
    vqgen_iterate(&v);
  }

  for(i=0;i<v.entries;i++){
    printf("\n");
    for(j=0;j<v.elements;j++)
      printf("%f,",_now(&v,i)[j]);
  }
  printf("\n");
    
  exit(0);*/


  memcpy(v.entrylist,testset256,sizeof(testset256));

  {
    long entryindex[v.entries];
    long pointindex[v.points];
    for(i=0;i<v.entries;i++)entryindex[i]=i;
    for(i=0;i<v.points;i++)pointindex[i]=i;

    fprintf(stderr,"\n\nleaves=%d\n",
	    lp_split(&v,entryindex,v.entries, pointindex,v.points,0,0));
  }
  
  return(0);
}














