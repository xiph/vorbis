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

 function: function calls to collect codebook metrics
 last mod: $Id: metrics.c,v 1.6 2000/02/16 16:18:37 xiphmont Exp $

 ********************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "bookutil.h"

/* set up metrics */

double meanamplitude_acc=0.;
double meanamplitudesq_acc=0.;
double meanerror_acc=0.;
double meanerrorsq_acc=0.;
double meandev_acc=0.;

double *histogram=NULL;
double *histogram_error=NULL;
double *histogram_errorsq=NULL;
double *histogram_distance=NULL;
double *histogram_hi=NULL;
double *histogram_lo=NULL;

double count=0.;

int books=0;
int dim=-1;
double *work;

static double *_now(codebook *c, int i){
  return c->valuelist+i*c->c->dim;
}

int histerrsort(const void *a, const void *b){
  double av=histogram_distance[*((long *)a)];
  double bv=histogram_distance[*((long *)b)];
  if(av<bv)return(-1);
  return(1);
}

void process_preprocess(codebook **bs,char *basename){
  while(bs[books]){
    codebook *b=bs[books];
    if(dim==-1){
      dim=b->c->dim;
      work=malloc(sizeof(double)*dim);
    }else{
      if(dim!=b->c->dim){
	fprintf(stderr,"Each codebook in a cascade must have the same dimensional order\n");
	exit(1);
      }
    }
    books++;
  }

  if(books){
    const static_codebook *b=bs[books-1]->c;
    histogram=calloc(b->entries,sizeof(double));
    histogram_distance=calloc(b->entries,sizeof(double));
    histogram_errorsq=calloc(b->entries*dim,sizeof(double));
    histogram_error=calloc(b->entries*dim,sizeof(double));
    histogram_hi=calloc(b->entries*dim,sizeof(double));
    histogram_lo=calloc(b->entries*dim,sizeof(double));
  }else{
    fprintf(stderr,"Specify at least one codebook\n");
    exit(1);
  }
}

static double _dist(int el,double *a, double *b){
  int i;
  double acc=0.;
  for(i=0;i<el;i++){
    double val=(a[i]-b[i]);
    acc+=val*val;
  }
  return acc;
}

void cell_spacing(codebook **b){
  int i,j,k;
  for(i=0;i<books;i++){
    double min,max,mean=0.,meansq=0.;
    codebook *c=b[i];

    fprintf(stderr,"\nCell spacing for book %d:\n",i);
    
    /* minimum, maximum, mean, ms cell spacing */
    for(j=0;j<c->c->entries;j++){
      double localmin=-1.;
      for(k=0;k<c->c->entries;k++){
	double this=_dist(c->c->dim,_now(c,j),_now(c,k));
	if(j!=k &&
	   (localmin==-1 || this<localmin))
	  localmin=this;
      }
      
      if(j==0 || localmin<min)min=localmin;
      if(j==0 || localmin>max)max=localmin;
      mean+=sqrt(localmin);
      meansq+=localmin;
    }

    fprintf(stderr,"\tminimum cell spacing (closest side): %g\n",sqrt(min));
    fprintf(stderr,"\tmaximum cell spacing (closest side): %g\n",sqrt(max));
    fprintf(stderr,"\tmean closest side spacing: %g\n",mean/c->c->entries);
    fprintf(stderr,"\tmean sq closest side spacing: %g\n",sqrt(meansq/c->c->entries));
  }
}

void process_postprocess(codebook **b,char *basename){
  int i,j,k;
  char *buffer=alloca(strlen(basename)+80);
  codebook *bb=b[books-1];

  fprintf(stderr,"Done.  Processed %ld data points for %ld entries:\n",
	  (long)count,bb->c->entries);
  fprintf(stderr,"\tglobal mean amplitude: %g\n",
	  meanamplitude_acc/(count*dim));
  fprintf(stderr,"\tglobal mean squared amplitude: %g\n",
	  sqrt(meanamplitudesq_acc/(count*dim)));

  fprintf(stderr,"\tglobal mean error: %g\n",
	  meanerror_acc/(count*dim));
  fprintf(stderr,"\tglobal mean squared error: %g\n",
	  sqrt(meanerrorsq_acc/(count*dim)));
  fprintf(stderr,"\tglobal mean deviation: %g\n",
	  meandev_acc/(count*dim));

  cell_spacing(b);

  {
    FILE *out;

    sprintf(buffer,"%s-mse.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }

    for(i=0;i<bb->c->entries;i++){
      for(k=0;k<bb->c->dim;k++){
	fprintf(out,"%ld, %g, %g\n",
		i*bb->c->dim+k,(bb->valuelist+i*bb->c->dim)[k],
		sqrt((histogram_errorsq+i*bb->c->dim)[k]/histogram[i]));
      }
    }
    fclose(out);

    sprintf(buffer,"%s-me.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }

    for(i=0;i<bb->c->entries;i++){
      for(k=0;k<bb->c->dim;k++){
	fprintf(out,"%ld, %g, %g\n",
		i*bb->c->dim+k,(bb->valuelist+i*bb->c->dim)[k],
		(histogram_error+i*bb->c->dim)[k]/histogram[i]);
      }
    }
    fclose(out);

    sprintf(buffer,"%s-worst.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }

    for(i=0;i<bb->c->entries;i++){
      for(k=0;k<bb->c->dim;k++){
	fprintf(out,"%ld, %g, %g, %g\n",
		i*bb->c->dim+k,(bb->valuelist+i*bb->c->dim)[k],
		(bb->valuelist+i*bb->c->dim)[k]+(histogram_lo+i*bb->c->dim)[k],
		(bb->valuelist+i*bb->c->dim)[k]+(histogram_hi+i*bb->c->dim)[k]);
      }
    }
    fclose(out);
  }

  {
    FILE *out;
    long *index=alloca(sizeof(long)*bb->c->entries);
    sprintf(buffer,"%s-distance.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    for(j=0;j<bb->c->entries;j++){
      if(histogram[j])histogram_distance[j]/=histogram[j];
      index[j]=j;
    }

    qsort(index,bb->c->entries,sizeof(long),histerrsort);

    for(j=0;j<bb->c->entries;j++)
      for(k=0;k<histogram[index[j]];k++)
	fprintf(out,"%g,\n",histogram_distance[index[j]]);
    fclose(out);
		
  }
}

void process_vector(codebook **bs,double *a){
  int bi;
  int i;
  double amplitude=0.;
  double distance=0.;
  double base=0.;
  int entry;
  double *e;
  memcpy(work,a,sizeof(double)*dim);

  for(bi=0;bi<books;bi++){
    codebook *b=bs[bi];
    entry=codebook_entry(b,work);
    e=b->valuelist+b->c->dim*entry;
    for(i=0;i<b->c->dim;i++)work[i]-=e[i];
  }

  for(i=0;i<dim;i++){
    double error=work[i];
    if(bs[0]->c->q_sequencep){
      amplitude=a[i]-base;
      base=a[i];
    }else
      amplitude=a[i];
    
    meanamplitude_acc+=fabs(amplitude);
    meanamplitudesq_acc+=amplitude*amplitude;
    meanerror_acc+=fabs(error);
    meanerrorsq_acc+=error*error;
    
    if(amplitude)
      meandev_acc+=fabs(error/amplitude);
    else
      meandev_acc+=fabs(error); /* yeah, yeah */
    
    histogram_errorsq[entry*dim+i]+=error*error;
    histogram_error[entry*dim+i]+=fabs(error);
    if(histogram[entry]==0 || histogram_hi[entry*dim+i]<error)
      histogram_hi[entry*dim+i]=error;
    if(histogram[entry]==0 || histogram_lo[entry*dim+i]>error)
      histogram_lo[entry*dim+i]=error;
    distance+=error*error;
  }

  histogram[entry]++;  
  histogram_distance[entry]+=sqrt(distance);

  if((long)(count++)%100)spinnit("working.... lines: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqmetrics <codebook>.vqh datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  Output goes to output files:\n"
	  "       basename-me.m:       gnuplot: mean error by entry value\n"
	  "       basename-mse.m:      gnuplot: mean square error by entry value\n"
	  "       basename-worst.m:    gnuplot: worst error by entry value\n"
	  "       basename-distance.m: gnuplot file showing distance probability\n"
	  "\n");

}
