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
 last mod: $Id: metrics.c,v 1.1 2000/01/06 13:57:13 xiphmont Exp $

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
double *histogram_errorsq=NULL;
double *histogram_distance=NULL;
double *histogram_hi=NULL;
double *histogram_lo=NULL;

double count=0.;

int histerrsort(const void *a, const void *b){
  double av=histogram_distance[*((long *)a)];
  double bv=histogram_distance[*((long *)b)];
  if(av<bv)return(-1);
  return(1);
}

void process_preprocess(codebook *b,char *basename){
  histogram=calloc(b->entries,sizeof(double));
  histogram_distance=calloc(b->entries,sizeof(double));
  histogram_errorsq=calloc(b->entries*b->dim,sizeof(double));
  histogram_hi=calloc(b->entries*b->dim,sizeof(double));
  histogram_lo=calloc(b->entries*b->dim,sizeof(double));
}

void process_postprocess(codebook *b,char *basename){
  int i,j,k;
  char *buffer=alloca(strlen(basename)+80);

  fprintf(stderr,"Done.  Processed %ld data points for %ld entries:\n",
	  (long)count,b->entries);
  fprintf(stderr,"\tglobal mean amplitude: %g\n",
	  meanamplitude_acc/(count*b->dim));
  fprintf(stderr,"\tglobal mean squared amplitude: %g\n",
	  sqrt(meanamplitudesq_acc/(count*b->dim)));

  fprintf(stderr,"\tglobal mean error: %g\n",
	  meanerror_acc/(count*b->dim));
  fprintf(stderr,"\tglobal mean squared error: %g\n",
	  sqrt(meanerrorsq_acc/(count*b->dim)));
  fprintf(stderr,"\tglobal mean deviation: %g\n",
	  meandev_acc/(count*b->dim));
  {
    FILE *out;
    sprintf(buffer,"%s-fit.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }

    for(i=0;i<b->entries;i++){
      for(k=0;k<b->dim;k++){
	fprintf(out,"%d, %g, %g\n",
		i*b->dim+k,(b->valuelist+i*b->dim)[k],
		sqrt((histogram_errorsq+i*b->dim)[k]/histogram[i]));
      }
    }
    fclose(out);

    sprintf(buffer,"%s-worst.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }

    for(i=0;i<b->entries;i++){
      for(k=0;k<b->dim;k++){
	fprintf(out,"%d, %g, %g, %g\n",
		i*b->dim+k,(b->valuelist+i*b->dim)[k],
		(b->valuelist+i*b->dim)[k]+(histogram_lo+i*b->dim)[k],
		(b->valuelist+i*b->dim)[k]+(histogram_hi+i*b->dim)[k]);
      }
    }
    fclose(out);
  }

  {
    FILE *out;
    long *index=alloca(sizeof(long)*b->entries);
    sprintf(buffer,"%s-distance.m",basename);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    for(j=0;j<b->entries;j++){
      if(histogram[j])histogram_distance[j]/=histogram[j];
      index[j]=j;
    }

    qsort(index,b->entries,sizeof(long),histerrsort);

    for(j=0;j<b->entries;j++)
      for(k=0;k<histogram[index[j]];k++)
	fprintf(out,"%g,\n",histogram_distance[index[j]]);
    fclose(out);
		
  }

}

void process_vector(codebook *b,double *a){
  int entry=codebook_entry(b,a);
  double *e=b->valuelist+b->dim*entry;
  int i;
  double amplitude=0.;
  double distance=0.;
  double base=0.;
  for(i=0;i<b->dim;i++){
    double error=a[i]-e[i];
    if(b->q_sequencep){
      amplitude=a[i]-base;
      base=a[i];
    }else
      amplitude=a[i];
    
    meanamplitude_acc+=fabs(amplitude);
    meanamplitudesq_acc+=amplitude*amplitude;
    meanerror_acc+=fabs(error);
    meanerrorsq_acc+=error*error;
    
    if(amplitude)
      meandev_acc+=fabs((a[i]-e[i])/amplitude);
    else
      meandev_acc+=fabs(a[i]-e[i]); /* yeah, yeah */
    
    histogram_errorsq[entry*b->dim+i]+=error*error;
    if(histogram[entry]==0 || histogram_hi[entry*b->dim+i]<error)
      histogram_hi[entry*b->dim+i]=error;
    if(histogram[entry]==0 || histogram_lo[entry*b->dim+i]>error)
      histogram_lo[entry*b->dim+i]=error;
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
	  "       basename-fit.m:      gnuplot: mean square error by entry value\n"
	  "       basename-worst.m:    gnuplot: worst error by entry value\n"
	  "       basename-distance.m: gnuplot file showing distance probability\n"
	  "\n");

}
