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
 last modification date: Oct 08 1999

 ********************************************************************/

/* This code is *not* part of libvorbis.  It is used to generate
   trained codebooks offline and then spit the results into a
   pregenerated codebook that is compiled into libvorbis.  It is an
   expensive (but good) algorithm.  Run it on big iron. */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "minit.h"

/************************************************************************
 * The basic idea here is that a VQ codebook is like an m-dimensional
 * foam with n bubbles.  The bubbles compete for space/volume and are
 * 'pressurized' [biased] according to some metric.  The basic alg
 * iterates through allowing the bubbles to compete for space until
 * they converge (if the damping is dome properly) on a steady-state
 * solution. Individual input points, collected from libvorbis, are
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
	val=actualdist/testdist;
      else
	val=testdist/actualdist;
      acc+=val*val-1.;
    }else{
      fprintf(stderr,"\nA zero (shouldn't happen in our data) \n");
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

static long it=0;
void vqgen_recenter(vqgen *v){
  long   i,j,k;
  double fdesired=(double)v->points/v->entries;
  double asserror=0.;
  double meterror=0.;

#ifdef NOISY
  char buff[80];
  FILE *cells;
  FILE *assig;
  FILE *bias;
  sprintf(buff,"cells%d.m",it);
  cells=fopen(buff,"w");
  sprintf(buff,"assig%d.m",it);
  assig=fopen(buff,"w");
  sprintf(buff,"bias%d.m",it);
  bias=fopen(buff,"w");
#endif

  if(v->entries<2)exit(1);

  /* first round: new midpoints */
  {
    double *newlo=malloc(sizeof(double)*v->entries*v->elements);
    double *newhi=malloc(sizeof(double)*v->entries*v->elements);

    memset(v->assigned,0,sizeof(long)*v->entries);

    for(i=0;i<v->points;i++){
      double firstmetric=v->metric_func(v,_now(v,0),_point(v,i))+v->bias[0];
      long   firstentry=0;
    
      for(j=1;j<v->entries;j++){
	double thismetric=v->metric_func(v,_now(v,j),_point(v,i))+v->bias[j];
	if(thismetric<firstmetric){
	  firstmetric=thismetric;
	  firstentry=j;
	}
      }

      j=firstentry;
#ifdef NOISY
      fprintf(cells,"%g %g\n%g %g\n\n",_now(v,j)[0],_now(v,j)[1],
	      _point(v,i)[0],_point(v,i)[1]);
#endif
      meterror+=firstmetric-v->bias[firstentry];
      if(v->assigned[j]++){
	for(k=0;k<v->elements;k++){
	  vN(newlo,j)[k]+=_point(v,i)[k];
	  /*if(_point(v,i)[k]<vN(newlo,j)[k])vN(newlo,j)[k]=_point(v,i)[k];
	    if(_point(v,i)[k]>vN(newhi,j)[k])vN(newhi,j)[k]=_point(v,i)[k];*/
	}
      }else{
	for(k=0;k<v->elements;k++){
	  vN(newlo,j)[k]=_point(v,i)[k];
	  /*vN(newlo,j)[k]=_point(v,i)[k];
	    vN(newhi,j)[k]=_point(v,i)[k];*/
	}	
      }
    }

    for(j=0;j<v->entries;j++){
      if(v->assigned[j]){
	for(k=0;k<v->elements;k++){
	  _now(v,j)[k]=vN(newlo,j)[k]/v->assigned[j];
	  /*_now(v,j)[k]=(vN(newlo,j)[k]+vN(newhi,j)[k])/2.;*/
	}
      }
    }
    free(newlo);
    free(newhi);
  }

  for(i=0;i<v->entries;i++){
    asserror+=fabs(fdesired-v->assigned[i]);
#ifdef NOISY
    fprintf(assig,"%ld\n",v->assigned[i]);
    fprintf(bias,"%g\n",v->bias[i]);
#endif
  }

  fprintf(stderr,"recenter: dist error=%g(%g) metric error=%g\n",
	  asserror/v->entries,fdesired,meterror/v->points);
  
  it++;

#ifdef NOISY
  fclose(cells);
  fclose(assig);
  fclose(bias);
#endif
}

void vqgen_rebias(vqgen *v){
  long   i,j,k;
  double fdesired=(float)v->points/v->entries;
  long   desired=fdesired;

  double asserror=0.;
  double meterror=0.;
  long   *nearcount=calloc(v->entries,sizeof(long));
  double *nearbias=malloc(v->entries*desired*sizeof(double));

  if(v->entries<2)exit(1);

  /* second round: fill in nearest points for entries */

  memset(v->assigned,0,sizeof(long)*v->entries);
  for(i=0;i<v->points;i++){
    /* not clear this should be biased */
    double firstmetric=v->metric_func(v,_now(v,0),_point(v,i))+v->bias[0];
    double secondmetric=v->metric_func(v,_now(v,1),_point(v,i))+v->bias[1];
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
    v->assigned[firstentry]++;
    meterror+=firstmetric-v->bias[firstentry];
    
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
  
  /* third round: inflate/deflate */
  {
    for(i=0;i<v->entries;i++){
      v->bias[i]=nearbias[(i+1)*desired-1];
    }
  }

  for(i=0;i<v->entries;i++){
    asserror+=fabs(fdesired-v->assigned[i]);
  }

  fprintf(stderr,"rebias: dist error=%g(%g) metric error=%g\n",
	  asserror/v->entries,fdesired,meterror/v->points);
  
  free(nearcount);
  free(nearbias);
}

/* the additional fields are the hyperplanes we've already used to
   split the set.  The additional hyperplanes are necessary to bound
   later splits.  One per depth */

int lp_split(vqgen *v,long *index,long points,
	      double *additional_a, double *additional_b, long depth){
  static int frameno=0;
  long A[points];
  long B[points];
  long ap=0;
  long bp=0;
  long cp=0;
  long i,j,k;
  long leaves=0;

  double best=0.;
  long besti=-1;
  long bestj=-1;

  if(depth>7){
    printf("leaf: points %ld, depth %ld\n",points,depth);
    return(1);
  }

  if(points==1){
    printf("leaf: point %ld, depth %ld\n",index[0],depth);
    return(1);
  }

  if(points==2){
    /* The result must be an even split */
    B[bp++]=index[0];
    A[ap++]=index[1];
    printf("even split: depth %ld\n",depth);
    return(2);
  }else{
    /* We need to find the best split */
    for(i=0;i<points-1;i++){
      for(j=i+1;j<points;j++){
	if(_dist_sq(v,_now(v,index[i]),_now(v,index[j]))>best){
	  besti=i;
	  bestj=j;
	  best=_dist_sq(v,_now(v,index[i]),_now(v,index[j]));
	}
      }
    }

    /* We have our endpoints. initial divvy */
    {
      double *cA=malloc(v->elements*sizeof(double));
      double *cB=malloc(v->elements*sizeof(double));
      double **a=malloc((points+depth)*sizeof(double *));
      double *b=malloc((points+depth)*sizeof(double));
      double *aa=malloc((points+depth)*v->elements*sizeof(double));
      double dA=0.,dB=0.;
      minit l;
      
      minit_init(&l,v->elements,points+depth-1,0);
      
      /* divisor hyperplane */
      for(i=0;i<v->elements;i++){
	double m=(_now(v,index[besti])[i]+_now(v,index[bestj])[i])/2;
	cA[i]=_now(v,index[besti])[i]-_now(v,index[bestj])[i];
	cB[i]=-cA[i];
	dA+=cA[i]*m;
	dB-=cA[i]*m;
      }
      
      for(i=0;i<points+depth;i++)
	a[i]=aa+i*v->elements;
      
      /* check each bubble to see if it intersects the divisor hyperplane */
      for(j=0;j<points;j++){
	long count=0,m=0;
	double d1=0.,d2=0.;
	int ret;
	
	if(j==besti){
	  B[bp++]=index[j];
	}else if (j==bestj){
	  A[ap++]=index[j];
	}else{
	 
	  for(i=0;i<points;i++){
	    if(i!=j){
	      b[count]=0.;
	      for(k=0;k<v->elements;k++){
		double m=(_now(v,index[i])[k]+_now(v,index[j])[k])/2;
		a[count][k]=_now(v,index[i])[k]-_now(v,index[j])[k];
		b[count]+=a[count][k]*m;
	      }
	      count++;
	    }
	  }
	  
	  /* additional bounding hyperplanes from previous splits */
	  for(i=0;i<depth;i++){
	    b[count]=0.;
	    for(k=0;k<v->elements;k++)
	      a[count][k]=additional_a[m++];
	    b[count++]=additional_b[i];
	  }
	  
	  /* on what side of the dividing hyperplane is the current test
	     point? */
	  for(i=0;i<v->elements;i++)
	    d1+=_now(v,index[j])[i]*cA[i];
	  
	  
	  if(d1<dA){
	    fprintf(stderr,"+");
	    ret=minit_solve(&l,a,b,cA,1e-6,NULL,NULL,&d2);
	  }else{
	    fprintf(stderr,"-");
	    ret=minit_solve(&l,a,b,cB,1e-6,NULL,NULL,&d2);
	    d2= -d2;
	  }
	  
	  switch(ret){
	  case 0:
	    /* bounded solution */
	    if(d1<dA){
	      A[ap++]=index[j];
	      if(d2>dA){cp++;B[bp++]=index[j];}
	    }else{
	      B[bp++]=index[j];
	      if(d2<dA){cp++;A[ap++]=index[j];}
	    }
	    break;
	  default:
	    /* unbounded solution or no solution; we're in both sets */
	    B[bp++]=index[j];
	    A[ap++]=index[j];
	    cp++;
	    break;
	  }
	}
      }

      /*{
	char buffer[80];
	FILE *o;
	int i;

	sprintf(buffer,"set%d.m",frameno);
	o=fopen(buffer,"w");
	for(i=0;i<points;i++)
	  fprintf(o,"%g %g\n\n",_now(v,index[i])[0],_now(v,index[i])[1]);
	fclose(o);

	sprintf(buffer,"setA%d.m",frameno);
	o=fopen(buffer,"w");
	for(i=0;i<ap;i++)
	  fprintf(o,"%g %g\n\n",_now(v,A[i])[0],_now(v,A[i])[1]);
	fclose(o);

	sprintf(buffer,"setB%d.m",frameno);
	o=fopen(buffer,"w");
	for(i=0;i<bp;i++)
	  fprintf(o,"%g %g\n\n",_now(v,B[i])[0],_now(v,B[i])[1]);
	fclose(o);

	sprintf(buffer,"div%d.m",frameno);
	o=fopen(buffer,"w");
	fprintf(o,"%g %g\n%g %g\n\n",
		_now(v,index[besti])[0],
		(dA-cA[0]*_now(v,index[besti])[0])/cA[1],
		_now(v,index[bestj])[0],
		(dA-cA[0]*_now(v,index[bestj])[0])/cA[1]);
	fclose(o);

	sprintf(buffer,"bound%d.m",frameno);
	o=fopen(buffer,"w");
	for(i=0;i<depth;i++)
	  fprintf(o,"%g %g\n%g %g\n\n",
		  _now(v,index[besti])[0],
		  (additional_b[i]-
		   additional_a[i*2]*_now(v,index[besti])[0])/
		  additional_a[i*2+1],
		  _now(v,index[bestj])[0],
		  (additional_b[i]-
		   additional_a[i*2]*_now(v,index[bestj])[0])/
		  additional_a[i*2+1]);
	fclose(o);
	frameno++;
	}*/


      minit_clear(&l);
      free(a);
      free(b);
      free(aa);

      fprintf(stderr,"split: total=%ld depth=%ld set A=%ld:%ld:%ld=B\n",
	      points,depth,ap-cp,cp,bp-cp);

      /* handle adding this divisor hyperplane to the additional
         constraints.  The 'inside' side is different for A and B */

      {
	/* marginally wasteful */
	double *newadd_a=alloca(sizeof(double)*v->elements*(depth+1));
	double *newadd_b=alloca(sizeof(double)*(depth+1));
	if(additional_a){
	  memcpy(newadd_a,additional_a,sizeof(double)*v->elements*depth);
	  memcpy(newadd_b,additional_b,sizeof(double)*depth);
	}

	memcpy(newadd_a+v->elements*depth,cA,sizeof(double)*v->elements);
	newadd_b[depth]=dA;	
	leaves=lp_split(v,A,ap,newadd_a,newadd_b,depth+1);

	memcpy(newadd_a+v->elements*depth,cB,sizeof(double)*v->elements);
	newadd_b[depth]=dB;	
	leaves+=lp_split(v,B,bp,newadd_a,newadd_b,depth+1);
      
      }
      free(cA);
      free(cB);    
      return(leaves);
    }
  }
}

void vqgen_book(vqgen *v){




}

static double testset24[48]={
1.047758,1.245406,
1.007972,1.150078,
1.064390,1.146266,
0.761842,0.826055,
0.862073,1.243707,
0.746351,1.055368,
0.956844,1.048223,
0.877782,1.111871,
0.964564,1.309219,
0.920062,1.168658,
0.787654,0.895163,
0.974003,1.200115,
0.788360,1.142671,
0.978414,1.122249,
0.869938,0.954436,
1.131220,1.348747,
0.934630,1.108564,
0.896666,1.045772,
0.707360,0.920954,
0.788914,0.970626,
0.828682,1.043673,
1.042016,1.190406,
1.031643,1.068194,
1.120824,1.220326,
};

static double testset256[1024]={
1.047758,1.245406,1.007972,1.150078,
1.064390,1.146266,0.761842,0.826055,
0.862073,1.243707,0.746351,1.055368,
0.956844,1.048223,0.877782,1.111871,
0.964564,1.309219,0.920062,1.168658,
0.787654,0.895163,0.974003,1.200115,
0.788360,1.142671,0.978414,1.122249,
0.869938,0.954436,1.131220,1.348747,
0.934630,1.108564,0.896666,1.045772,
0.707360,0.920954,0.788914,0.970626,
0.828682,1.043673,1.042016,1.190406,
1.031643,1.068194,1.120824,1.220326,
0.747084,0.888176,1.051535,1.221335,
0.980283,1.087708,1.311154,1.445530,
0.927548,1.107237,1.296902,1.480610,
0.804415,0.921948,1.029952,1.231078,
0.923413,1.064919,1.143109,1.242755,
0.826403,1.010624,1.353139,1.455870,
0.837930,1.073739,1.251770,1.405370,
0.704437,0.890366,1.016676,1.136689,
0.840924,1.151858,1.369346,1.558572,
0.835229,1.095713,1.165585,1.311155,
0.763157,1.000718,1.168780,1.345318,
0.912852,1.040279,1.260220,1.468606,
0.680839,0.920854,1.056199,1.214924,
0.993572,1.124795,1.220880,1.416346,
0.766878,0.994942,1.170035,1.432164,
1.006875,1.069566,1.283594,1.531742,
1.029033,1.188357,1.330626,1.519446,
0.915581,1.121514,1.193260,1.431981,
0.950939,1.117674,1.280092,1.487413,
0.783569,0.863166,1.198348,1.309199,
0.845209,0.949087,1.133299,1.253263,
0.836299,1.050413,1.263712,1.482634,
1.049458,1.194466,1.332964,1.485894,
0.921516,1.086016,1.244810,1.349821,
0.888983,1.111494,1.313032,1.351122,
1.015785,1.142849,1.292746,1.463028,
0.860002,1.249262,1.354167,1.534761,
0.942136,1.053546,1.120840,1.373279,
0.846661,1.059871,1.294814,1.504904,
0.796592,1.061316,1.349680,1.494821,
0.880504,1.033570,1.314379,1.484966,
0.770438,1.039993,1.238670,1.412317,
1.036503,1.068649,1.202522,1.487553,
0.942615,1.056630,1.344355,1.562161,
0.963759,1.096427,1.324085,1.497266,
0.899956,1.206083,1.349655,1.572202,
0.977023,1.132722,1.338437,1.488086,
1.096643,1.242143,1.357079,1.513244,
0.795193,1.011915,1.312761,1.419554,
1.021869,1.075269,1.265230,1.475986,
1.019546,1.186088,1.347798,1.567607,
1.051191,1.172904,1.381952,1.511843,
0.924332,1.192953,1.385378,1.420727,
0.869539,0.962051,1.400803,1.539414,
0.949875,1.053939,1.305695,1.422764,
0.962223,1.085902,1.247702,1.462327,
0.991777,1.182062,1.287421,1.508453,
0.918274,1.107269,1.325467,1.454377,
0.993589,1.184665,1.352294,1.475413,
0.804960,1.011490,1.259031,1.462335,
0.845949,1.173858,1.389681,1.441148,
0.858168,1.019930,1.232463,1.440218,
0.884411,1.206226,1.318968,1.457990,
0.893469,1.074271,1.274410,1.433475,
1.039654,1.061160,1.122426,1.189133,
1.062243,1.177357,1.344765,1.542071,
0.900964,1.116803,1.380397,1.419108,
0.969535,1.216066,1.383582,1.435924,
1.017913,1.330986,1.403452,1.450709,
0.771534,0.973324,1.171604,1.236873,
0.955683,1.029609,1.309284,1.498449,
0.892165,1.192347,1.322444,1.356025,
0.890209,1.023145,1.326649,1.431669,
1.038788,1.188891,1.330844,1.436180,
1.050355,1.177363,1.283447,1.479040,
0.825367,0.960892,1.179047,1.381923,
0.978931,1.122163,1.304581,1.532161,
0.970750,1.149650,1.390225,1.547422,
0.805416,0.945116,1.118815,1.343870,
1.004106,1.185996,1.304086,1.464108,
0.771591,1.055865,1.150908,1.228683,
0.950175,1.104237,1.203437,1.387264,
0.923504,1.122814,1.173989,1.318650,
0.960635,1.042342,1.196995,1.395394,
1.035111,1.064804,1.134387,1.341424,
0.815182,1.083612,1.159518,1.255777,
0.784079,0.830594,1.056005,1.289902,
0.737774,0.994134,1.115470,1.264879,
0.820162,1.105657,1.182947,1.423629,
1.067552,1.142869,1.191626,1.387014,
0.752416,0.918026,1.124637,1.414500,
0.767963,0.840069,0.997625,1.325653,
0.787595,0.865440,1.090071,1.227348,
0.798877,1.105239,1.331379,1.440643,
0.829079,1.133632,1.280774,1.470690,
1.009195,1.132959,1.284044,1.500589,
0.698569,0.824611,1.236682,1.462088,
0.817460,0.985767,1.081910,1.257751,
0.784033,0.882552,1.149678,1.326920,
1.039403,1.085310,1.211033,1.558635,
0.966795,1.168857,1.309960,1.497788,
0.906244,1.027050,1.198846,1.323671,
0.776495,1.185285,1.309167,1.380683,
0.799628,0.927976,1.048238,1.299045,
0.779808,0.881990,1.167898,1.220314,
0.770446,0.918281,1.049189,1.179393,
0.768416,1.037558,1.165760,1.302606,
0.743121,0.814671,0.990501,1.224236,
1.037050,1.068240,1.159690,1.426280,
0.978810,1.214329,1.253336,1.324395,
0.984003,1.121443,1.376382,1.510519,
1.146510,1.229726,1.417616,1.781032,
0.897163,1.147910,1.221186,1.371815,
1.068525,1.211553,1.343551,1.506743,
0.762313,1.091082,1.253251,1.472381,
0.960562,1.041965,1.247053,1.399214,
0.864482,1.123473,1.163412,1.238620,
0.963484,1.132803,1.164992,1.250389,
1.009456,1.139510,1.251339,1.449078,
0.851837,1.113642,1.170290,1.362806,
0.857073,0.962039,1.127381,1.471682,
1.047754,1.213381,1.388899,1.492383,
0.938921,1.267308,1.337076,1.478427,
0.790388,0.912816,1.159450,1.273259,
0.832690,0.997776,1.156639,1.302621,
0.783009,0.975374,1.080630,1.311112,
0.819784,1.145093,1.326949,1.525480,
0.806394,1.089564,1.329564,1.550362,
0.958608,1.036364,1.379118,1.443043,
0.753680,0.941781,1.147749,1.297211,
0.883974,1.091394,1.315093,1.480307,
1.239591,1.417601,1.495843,1.641941,
0.881068,0.973780,1.278918,1.429384,
0.869052,0.977661,1.280744,1.551295,
1.022685,1.052986,1.105046,1.168670,
0.981698,1.131448,1.197781,1.467704,
0.945034,1.163410,1.250872,1.428793,
1.092055,1.139380,1.187113,1.264586,
0.788897,0.953764,1.232844,1.506461,
0.885206,0.978419,1.209467,1.387569,
0.762835,0.964279,1.064844,1.243153,
0.974906,1.134763,1.283544,1.460386,
1.081114,1.169260,1.290987,1.535452,
0.880531,1.075660,1.378980,1.522978,
1.039431,1.083792,1.350481,1.588522,
0.922449,1.060622,1.152309,1.467930,
0.918935,1.073732,1.283243,1.479567,
0.992722,1.145398,1.239500,1.509004,
0.903405,1.243062,1.421386,1.572148,
0.912531,1.100239,1.310219,1.521424,
0.875168,0.912787,1.123618,1.363635,
0.732241,1.108317,1.323119,1.515675,
0.951518,1.141874,1.341623,1.533409,
0.992099,1.215801,1.477413,1.734691,
1.005267,1.122655,1.356975,1.436445,
0.740659,0.807749,1.167728,1.380618,
1.078727,1.214706,1.328173,1.556699,
1.026027,1.168906,1.313249,1.486078,
0.914877,1.147150,1.389489,1.523984,
1.062339,1.120684,1.160024,1.234794,
1.129141,1.197655,1.374217,1.547755,
1.070011,1.255271,1.360225,1.467869,
0.779980,0.871696,1.098031,1.284490,
1.062409,1.177542,1.314581,1.696662,
0.702935,1.229873,1.370813,1.479500,
1.029357,1.225167,1.341607,1.478163,
1.025666,1.141749,1.185959,1.332892,
0.799462,0.951470,1.214070,1.305787,
0.740521,0.805457,1.107504,1.317258,
0.784194,0.838683,1.055934,1.242692,
0.839416,1.048060,1.391801,1.623786,
0.692627,0.907677,1.060843,1.341002,
0.823625,1.244497,1.396901,1.586243,
0.942859,1.232978,1.348170,1.536735,
0.894882,1.131376,1.292892,1.462724,
0.776974,0.904449,1.325557,1.451968,
1.066188,1.218328,1.376282,1.545381,
0.990053,1.124279,1.340534,1.559666,
0.776321,0.935169,1.191081,1.372326,
0.949935,1.115929,1.397704,1.442549,
0.936357,1.126885,1.356247,1.579931,
1.045433,1.274605,1.366947,1.590215,
1.063890,1.138062,1.220645,1.460005,
0.751448,0.811338,1.078027,1.403146,
0.935678,1.102858,1.356557,1.515948,
1.026417,1.143843,1.299309,1.413976,
0.821475,1.000237,1.105073,1.379882,
0.960249,1.031602,1.250804,1.462620,
0.661745,1.106452,1.291188,1.439529,
0.691661,0.947241,1.261183,1.457391,
0.839024,0.914750,1.040695,1.375853,
1.029401,1.065349,1.121370,1.270670,
0.888316,1.003349,1.103749,1.341290,
0.766328,0.879083,1.217779,1.419868,
0.811672,1.049673,1.186460,1.375742,
0.969585,1.170126,1.259338,1.595070,
1.016617,1.145706,1.335413,1.503064,
0.980227,1.295316,1.417964,1.478684,
0.885701,1.248099,1.416821,1.465338,
0.953583,1.266596,1.325829,1.372230,
1.038619,1.225860,1.351664,1.527048,
1.104724,1.215839,1.250491,1.335237,
1.124449,1.269485,1.420756,1.677260,
0.881337,1.352259,1.433218,1.492756,
1.023097,1.059205,1.110249,1.212976,
1.135632,1.282243,1.415540,1.566039,
1.063524,1.252342,1.399082,1.518433,
0.790658,0.856337,1.154909,1.274963,
1.119059,1.382625,1.423651,1.492741,
1.030526,1.296610,1.330511,1.396550,
0.947952,1.065235,1.225274,1.554455,
0.960669,1.282313,1.370362,1.572736,
0.996042,1.208193,1.385036,1.534877,
1.003206,1.377432,1.431110,1.497189,
1.088166,1.227944,1.508129,1.740407,
0.996566,1.162407,1.347665,1.524235,
0.944606,1.287026,1.400822,1.437156,
1.066144,1.238314,1.454451,1.616016,
1.121065,1.200875,1.316542,1.459583,
1.158944,1.271448,1.356823,1.450510,
0.811670,1.278011,1.361550,1.440647,
0.875620,1.103051,1.230854,1.483429,
0.959882,1.180685,1.381224,1.572807,
1.049222,1.186513,1.387883,1.567423,
1.049920,1.293154,1.507194,1.678371,
0.872138,1.193727,1.455817,1.665837,
0.738018,0.946583,1.335543,1.552061,
1.072856,1.151838,1.321877,1.486028,
1.026909,1.153575,1.306700,1.550408,
0.940629,1.121943,1.303228,1.409285,
1.023544,1.269480,1.316695,1.508732,
0.924872,1.119626,1.479602,1.698360,
0.793358,1.160361,1.381276,1.636124,
0.892504,1.248046,1.320256,1.367114,
1.165222,1.230265,1.507234,1.820748,
0.711084,0.876165,1.145850,1.493303,
0.776482,0.869670,1.124462,1.370831,
0.785085,0.912534,1.099328,1.281523,
0.947788,1.168239,1.407409,1.473592,
0.913992,1.316132,1.619366,1.865030,
1.024592,1.151907,1.383557,1.545112,
0.987748,1.168194,1.415323,1.639563,
0.725008,1.037088,1.306659,1.474607,
0.965243,1.263523,1.454521,1.638929,
1.035169,1.219404,1.427315,1.578414,
0.787215,0.960639,1.245517,1.423549,
0.902895,1.153039,1.412772,1.613360,
0.983205,1.338886,1.493960,1.619623,
0.822576,1.010524,1.232397,1.359007,
0.773450,1.090005,1.170248,1.369534,
1.117781,1.354485,1.662228,1.829565,
1.104316,1.324945,1.525085,1.741040,
1.135275,1.371208,1.537082,1.673949,
0.889899,1.195451,1.242648,1.291881,

};

int main(int argc,char *argv[]){
  FILE *in=fopen(argv[1],"r");
  vqgen v;
  char buffer[160];
  int i,j;

  vqgen_init(&v,4,256,_dist_sq,0.);
  
  while(fgets(buffer,160,in)){
    double a[8];
    if(sscanf(buffer,"%lf %lf %lf %lf",
	      a,a+1,a+2,a+3)==4)
      vqgen_addpoint(&v,a);
  }
  fclose(in);
  
  for(i=0;i<10;i++)
    vqgen_recenter(&v);
  
  for(i=0;i<200;i++){
    vqgen_rebias(&v);
    vqgen_rebias(&v);
    vqgen_rebias(&v);
    vqgen_rebias(&v);
    vqgen_rebias(&v);
    vqgen_recenter(&v);
    fprintf(stderr,"%d ",i);
  }

  vqgen_recenter(&v);

  exit(0);

  memcpy(v.entrylist,testset256,sizeof(testset256));

  for(i=0;i<v.entries;i++){
    printf("\n");
    for(j=0;j<v.elements;j++)
      printf("%f,",_now(&v,i)[j]);
  }
  printf("\n");
    

  /* try recursively splitting the space using LP 
  {
    long index[v.entries];
    for(i=0;i<v.entries;i++)index[i]=i;
    fprintf(stderr,"\n\nleaves=%d\n",
	    lp_split(&v,index,v.entries,NULL,NULL,0));
	    }*/
  
  return(0);
}














