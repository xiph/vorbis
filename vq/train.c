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

 function: utility main for training codebooks
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Dec 14 1999

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include "vqgen.h"

/* A metric for LSP codes */
                            /* candidate,actual */
static double _dist_and_pos(vqgen *v,double *b, double *a){
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

static void *set_metric(int m){
  switch(m){
  case 0:
    return(NULL);
  case 1:
    return(_dist_and_pos);
  default:
    fprintf(stderr,"Invalid metric number\n");
    exit(0);
  }
}

static int rline(FILE *in,FILE *out,char *line,int max,int pass){
  while(fgets(line,160,in)){
    if(line[0]=='#'){
      if(pass)fprintf(out,"%s",line);
    }else{
      return(1);
    }
  }
  return(0);
}

/* command line:
   trainvq [vq=file | [entries=n] [dim=n] [quant=n]] met=n in=file,firstcol 
           [in=file,firstcol]
*/

int exiting=0;
void setexit(int dummy){
  fprintf(stderr,"\nexiting... please wait to finish this iteration\n");
  exiting=1;
}

int main(int argc,char *argv[]){
  vqgen v;
  int entries=-1,dim=-1,quant=-1;
  FILE *out=NULL;
  int met=0;
  double (*metric)(vqgen *,double *, double *)=NULL;
  char line[1024];
  long i,j,k;

  double desired=.05;
  int iter=1000;

  int init=0;
  while(*argv){

    /* continue training an existing book */
    if(!strncmp(*argv,"vq=",3)){
      FILE *in=NULL;
      char filename[80],*ptr;
      if(sscanf(*argv,"vq=%70s",filename)!=1){
	fprintf(stderr,"Syntax error in argument '%s'\n",*argv);
	exit(1);
      }

      in=fopen(filename,"r");
      ptr=strrchr(filename,'-');
      if(ptr){
	int num;
	ptr++;
	num=atoi(ptr);
	sprintf(ptr,"%d.vqi",num+1);
      }else
	strcat(filename,"-0.vqi");
      
      out=fopen(filename,"w");
      if(out==NULL){
	fprintf(stderr,"Unable to open %s for writing\n",filename);
	exit(1);
      }
      fprintf(out,"# OggVorbis VQ codebook trainer, intermediate file\n");

      if(in){
	/* we wish to suck in a preexisting book and continue to train it */
	double a;
	    
	rline(in,out,line,160,1);
	if(sscanf(line,"%d %d %d %d",&entries,&dim,&met,&quant)!=3){
	  fprintf(stderr,"Syntax error reading book file\n");
	  exit(1);
	}

	metric=set_metric(met);
	vqgen_init(&v,dim,entries,metric,quant);
	init=1;

	/* entries, bias, points */
	i=0;
	for(j=0;j<entries;j++){
	  for(k=0;k<dim;k++){
	    rline(in,out,line,160,0);
	    sscanf(line,"%lf",&a);
	    v.entrylist[i++]=a;
	  }
	}

	i=0;
	for(j=0;j<entries;j++){
	  rline(in,out,line,160,0);
	  sscanf(line,"%lf",&a);
	  v.bias[i++]=a;
	}

	{
	  double b[80];
	  i=0;
	  v.entries=0; /* hack to avoid reseeding */
	  while(1){
	    for(k=0;k<dim && k<80;k++){
	      rline(in,out,line,160,0);
	      sscanf(line,"%lf",b+k);
	    }
	    if(feof(in))break;
	    vqgen_addpoint(&v,b);
	  }
	  v.entries=entries;
	}

	fclose(in);
      }
    }

    /* set parameters if we're not loading a pre book */
    if(!strncmp(*argv,"quant=",6)){
      sscanf(*argv,"quant=%d",&quant);
    }
    if(!strncmp(*argv,"entries=",8)){
      sscanf(*argv,"entries=%d",&entries);
    }
    if(!strncmp(*argv,"desired=",8)){
      sscanf(*argv,"desired=%lf",&desired);
    }
    if(!strncmp(*argv,"dim=",4)){
      sscanf(*argv,"dim=%d",&dim);
    }

    /* which error metric (0==euclidian distance default) */
    if(!strncmp(*argv,"met=",4)){
      sscanf(*argv,"met=%d",&met);
      metric=set_metric(met);
    }

    if(!strncmp(*argv,"in=",3)){
      int start;
      char file[80];
      FILE *in;

      if(sscanf(*argv,"in=%79[^,],%d",file,&start)!=2)goto syner;
      if(!out){
	fprintf(stderr,"vq= must preceed in= arguments\n");
	exit(1);
      }
      if(!init){
	if(dim==-1 || entries==-1 || quant==-1){
	  fprintf(stderr,"Must specify dimensionality,entries,quant before"
		  " first input file\n");
	  exit(1);
	}
	vqgen_init(&v,dim,entries,metric,quant);
	init=1;
      }

      in=fopen(file,"r");
      if(in==NULL){
	fprintf(stderr,"Could not open input file %s\n",file);
	exit(1);
      }
      fprintf(out,"# training file entry: %s\n",file);

      while(rline(in,out,line,1024,1)){
	double b[16];
	int n=sscanf(line,"%lf %lf %lf %lf %lf %lf %lf %lf "
		     "%lf %lf %lf %lf %lf %lf %lf %lf",
		     b,b+1,b+2,b+3,b+4,b+5,b+6,b+7,b+8,b+9,b+10,b+11,b+12,b+13,
		     b+14,b+15);
	if(start+dim>n){
	  fprintf(stderr,"ran out of columns reading %s\n",file);
	  exit(1);
	}
	vqgen_addpoint(&v,b+start);
      }

      fclose(in);
    }
    argv++;
  }

  /* train the book */
  signal(SIGTERM,setexit);
  signal(SIGINT,setexit);

  for(i=0;i<iter && !exiting;i++){
    if(vqgen_iterate(&v)<desired)break;
  }

  /* save the book */

  fprintf(out,"%d %d %d %d\n",entries,dim,met,quant);

  i=0;
  for(j=0;j<entries;j++)
    for(k=0;k<dim;k++)
      fprintf(out,"%f\n",v.entrylist[i++]);
  
  fprintf(out,"# biases---\n");
  i=0;
  for(j=0;j<entries;j++)
    fprintf(out,"%f\n",v.bias[i++]);

  fprintf(out,"# points---\n");
  i=0;
  for(j=0;j<v.points;j++)
    for(k=0;k<dim && k<80;k++)
      fprintf(out,"%f\n",v.pointlist[i++]);

  fclose(out);
  exit(0);

  syner:
    fprintf(stderr,"Syntax error in argument '%s'\n",*argv);
    exit(1);
}
