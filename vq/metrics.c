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
 last mod: $Id: metrics.c,v 1.6.4.3 2000/04/26 07:10:16 xiphmont Exp $

 ********************************************************************/


#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "vorbis/codebook.h"
#include "../lib/sharedbook.h"
#include "bookutil.h"

/* collect the following metrics:

   mean and mean squared amplitude
   mean and mean squared error 
   mean and mean squared error (per sample) by entry
   worst case fit by entry
   entry cell size
   hits by entry
   total bits
   total samples
   (average bits per sample)*/
   

/* set up metrics */

double meanamplitude_acc=0.;
double meanamplitudesq_acc=0.;
double meanerror_acc=0.;
double meanerrorsq_acc=0.;

double **histogram=NULL;
double **histogram_error=NULL;
double **histogram_errorsq=NULL;
double **histogram_hi=NULL;
double **histogram_lo=NULL;
double bits=0.;
double count=0.;

static double *_now(codebook *c, int i){
  return c->valuelist+i*c->c->dim;
}

int books=0;

void process_preprocess(codebook **bs,char *basename){
  int i;
  while(bs[books])books++;
  
  if(books){
    histogram=calloc(books,sizeof(double *));
    histogram_error=calloc(books,sizeof(double *));
    histogram_errorsq=calloc(books,sizeof(double *));
    histogram_hi=calloc(books,sizeof(double *));
    histogram_lo=calloc(books,sizeof(double *));
  }else{
    fprintf(stderr,"Specify at least one codebook\n");
    exit(1);
  }

  for(i=0;i<books;i++){
    codebook *b=bs[i];
    histogram[i]=calloc(b->entries,sizeof(double));
    histogram_error[i]=calloc(b->entries*b->dim,sizeof(double));
    histogram_errorsq[i]=calloc(b->entries*b->dim,sizeof(double));
    histogram_hi[i]=calloc(b->entries*b->dim,sizeof(double));
    histogram_lo[i]=calloc(b->entries*b->dim,sizeof(double));
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

void cell_spacing(codebook *c){
  int j,k;
  double min,max,mean=0.,meansq=0.;
  
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

void process_postprocess(codebook **bs,char *basename){
  int i,k,book;
  char *buffer=alloca(strlen(basename)+80);

  fprintf(stderr,"Done.  Processed %ld data points:\n\n",
	  (long)count);

  fprintf(stderr,"Global statistics:******************\n\n");

  fprintf(stderr,"\ttotal samples: %ld\n",(long)count);
  fprintf(stderr,"\ttotal bits required to code: %ld\n",(long)bits);
  fprintf(stderr,"\taverage bits per sample: %g\n\n",bits/count);

  fprintf(stderr,"\tmean sample amplitude: %g\n",
	  meanamplitude_acc/count);
  fprintf(stderr,"\tmean squared sample amplitude: %g\n\n",
	  sqrt(meanamplitudesq_acc/count));

  fprintf(stderr,"\tmean code error: %g\n",
	  meanerror_acc/count);
  fprintf(stderr,"\tmean squared code error: %g\n\n",
	  sqrt(meanerrorsq_acc/count));

  for(book=0;book<books;book++){
    FILE *out;
    codebook *b=bs[book];
    int n=b->c->entries;
    int dim=b->c->dim;

    fprintf(stderr,"Book %d statistics:------------------\n",book);

    cell_spacing(b);

    sprintf(buffer,"%s-%d-mse.m",basename,book);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    
    for(i=0;i<n;i++){
      for(k=0;k<dim;k++){
	fprintf(out,"%d, %g, %g\n",
		i*dim+k,(b->valuelist+i*dim)[k],
		sqrt((histogram_errorsq[book]+i*dim)[k]/histogram[book][i]));
      }
    }
    fclose(out);
      
    sprintf(buffer,"%s-%d-me.m",basename,book);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    
    for(i=0;i<n;i++){
      for(k=0;k<dim;k++){
	fprintf(out,"%d, %g, %g\n",
		i*dim+k,(b->valuelist+i*dim)[k],
		(histogram_error[book]+i*dim)[k]/histogram[book][i]);
      }
    }
    fclose(out);

    sprintf(buffer,"%s-%d-worst.m",basename,book);
    out=fopen(buffer,"w");
    if(!out){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
    
    for(i=0;i<n;i++){
      for(k=0;k<dim;k++){
	fprintf(out,"%d, %g, %g, %g\n",
		i*dim+k,(b->valuelist+i*dim)[k],
		(b->valuelist+i*dim)[k]+(histogram_lo[book]+i*dim)[k],
		(b->valuelist+i*dim)[k]+(histogram_hi[book]+i*dim)[k]);
      }
    }
    fclose(out);
  }
}

void process_one(codebook *b,int book,double *a,int dim,int step,int addmul){
  int j,entry;
  double base=0.;
  double amplitude=0.;

  if(book==0)
    for(j=0;j<dim;j++){
      if(b->c->q_sequencep){
	amplitude=a[j*step]-base;
	base=a[j*step];
      }else
	amplitude=a[j*step];
      meanamplitude_acc+=fabs(amplitude);
      meanamplitudesq_acc+=amplitude*amplitude;
      count++;
    }

  entry=vorbis_book_besterror(b,a,step,addmul);
  
  histogram[book][entry]++;  
  bits+=vorbis_book_codelen(b,entry);
	  
  for(j=0;j<dim;j++){
    double error=a[j*step];

    if(book==books-1){
      meanerror_acc+=fabs(error);
      meanerrorsq_acc+=error*error;
    }
    histogram_errorsq[book][entry*dim+j]+=error*error;
    histogram_error[book][entry*dim+j]+=fabs(error);
    if(histogram[book][entry]==0 || histogram_hi[book][entry*dim+j]<error)
      histogram_hi[book][entry*dim+j]=error;
    if(histogram[book][entry]==0 || histogram_lo[book][entry*dim+j]>error)
      histogram_lo[book][entry*dim+j]=error;
  }
}


void process_vector(codebook **bs,int *addmul,int inter,double *a,int n){
  int bi;
  int i;

  for(bi=0;bi<books;bi++){
    codebook *b=bs[bi];
    int dim=b->dim;
    
    if(inter){
      for(i=0;i<n/dim;i++)
	process_one(b,bi,a+i,dim,n/dim,addmul[bi]);
    }else{
      for(i=0;i<=n-dim;i+=dim)
	process_one(b,bi,a+i,dim,1,addmul[bi]);
    }
  }
  
  if((long)(count)%100)spinnit("working.... samples: ",count);
}

void process_usage(void){
  fprintf(stderr,
	  "usage: vqmetrics [-i] +|*<codebook>.vqh [ +|*<codebook.vqh> ]... \n"
	  "                 datafile.vqd [datafile.vqd]...\n\n"
	  "       data can be taken on stdin.  -i indicates interleaved coding.\n"
	  "       Output goes to output files:\n"
	  "       basename-me.m:       gnuplot: mean error by entry value\n"
	  "       basename-mse.m:      gnuplot: mean square error by entry value\n"
	  "       basename-worst.m:    gnuplot: worst error by entry value\n"
	  "       basename-distance.m: gnuplot file showing distance probability\n"
	  "\n");

}
