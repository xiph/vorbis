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

 function: residue backend 0 partitioner
 last mod: $Id: residuesplit.c,v 1.1.4.1 2000/04/13 04:53:03 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "../lib/scales.h"
#include "../vq/bookutil.h"

/* take a masking curve and raw residue; eliminate the inaduble and
   quantize to the final form handed to the VQ.  All and any tricks to
   squeeze out bits given knowledge of the encoding mode should go
   here too */

/* modifies the pcm vector, returns book membership in aux */

/* This is currently a bit specific to/hardwired for mapping 0; things
   will need to change in the future when we get real multichannel
   mappings */

/* does not guard against invalid settings; eg, a subn of 16 and a
   subgroup request of 32.  Max subn of 128 */
static void _testhack(double *vec,int n,double *entropy){
  int i,j=0;
  double max=0.;
  double temp[128];

  /* setup */
  for(i=0;i<n;i++){
    if(vec[i])
      temp[i]=(todB(vec[i])+6.);
    else
      temp[i]=0.;
  }

  /* handle case subgrp==1 outside */
  for(i=0;i<n;i++)
    if(temp[i]>max)max=temp[i];

  while(1){
    entropy[j]=max;
    n>>=1;
    j++;

    if(n<=0)break;
    for(i=0;i<n;i++){
      temp[i]+=temp[i+n];
    }
    max=0.;
    for(i=0;i<n;i++)
      if(temp[i]>max)max=temp[i];
  }
}

static FILE *of;
static FILE **or;


/* we evaluate the the entropy measure for each interleaved subgroup */
int quantaux(double *res,int n,double *bound,int *subgrp,int parts, int subn){
  long i,j,p;
  double entropy[8];

  for(i=0;i<=n-subn;i+=subn){
    int aux;
    int step;
    _testhack(res+i,subn,entropy);

    for(j=0;j<parts-1;j++)
      if(entropy[subgrp[j]]>bound[j])
	break;
    aux=j;

    fprintf(of,"%d, ",aux);      
    step=subn/(1<<subgrp[j]);

    for(j=0;j<step;j++){
      for(p=j;p<subn;p+=step)
	fprintf(or[aux],"%g, ",res[p+i]);
      fprintf(or[aux],"\n");
    }
  }

  fprintf(of,"\n");

  return(0);
}

static int getline(FILE *in,double *vec,int begin,int n){
  int i,next=0;

  reset_next_value();
  if(get_next_value(in,vec))return(0);
  if(begin){
    for(i=1;i<begin;i++)
      get_line_value(in,vec);
    next=0;
  }else{
    next=1;
  }

  for(i=next;i<n;i++)
    if(get_line_value(in,vec+i)){
      fprintf(stderr,"ran out of columns in input data\n");
      exit(1);
    }
  
  return(1);
}

static void usage(){
  fprintf(stderr,
	  "usage:\n" 
	  "residuesplit <res> <begin,n,group> <baseout> <max,subgr> [<max,subgr>]...\n"
	  "   where begin,n,group is first scalar, \n"
	  "                          number of scalars of each in line,\n"
	  "                          number of scalars in a group\n"
	  "         maxent is the maximum allowable entropy heuristic for a group\n"
	  "eg: residuesplit mask.vqd floor.vqd 0,1024,16 res 10,2 10,4 5,4 ,8\n"
	  "produces resaux.vqd and res_0...n.vqd\n\n");
  exit(1);
}

int main(int argc, char *argv[]){
  char *buffer;
  char *base;
  int i,parts,begin,n,subn,*subgrp;
  FILE *res;
  double *bound,*vec;
  long c=0;
  if(argc<5)usage();

  base=strdup(argv[3]);
  buffer=alloca(strlen(base)+20);
  {
    char *pos=strchr(argv[2],',');
    begin=atoi(argv[2]);
    if(!pos)
      usage();
    else
      n=atoi(pos+1);
    pos=strchr(pos+1,',');
    if(!pos)
      usage();
    else
      subn=atoi(pos+1);
    if(n/subn*subn != n){
      fprintf(stderr,"n must be divisible by group\n");
      exit(1);
    }
  }

  /* how many parts?... */
  parts=argc-4;
  bound=malloc(sizeof(double)*parts);
  subgrp=malloc(sizeof(int)*parts);

  for(i=0;i<parts;i++){
    char *pos=strchr(argv[4+i],',');
    if(*argv[4+i]==',')
      bound[i]=1e50;
    else
      bound[i]=atof(argv[4+i]);
    if(!pos){
      subgrp[i]=_ilog(subn)-1;
    }else{
      subgrp[i]=_ilog(atoi(pos+1))-1;
    }

  }

  res=fopen(argv[1],"r");
  if(!res){
    fprintf(stderr,"Could not open file %s\n",argv[1]);
    exit(1);
  }

  or=alloca(parts*sizeof(FILE*));
  sprintf(buffer,"%saux.vqd",base);
  of=fopen(buffer,"w");
  if(!of){
    fprintf(stderr,"Could not open file %s for writing\n",buffer);
    exit(1);
  }
  for(i=0;i<parts;i++){
    sprintf(buffer,"%s_%d.vqd",base,i);
    or[i]=fopen(buffer,"w");
    if(!or[i]){
      fprintf(stderr,"Could not open file %s for writing\n",buffer);
      exit(1);
    }
  }
  
  vec=malloc(sizeof(double)*n);
  /* get the input line by line and process it */
  while(!feof(res)){
    if(getline(res,vec,begin,n))
      quantaux(vec,n,bound,subgrp,parts,subn);
    c++;
    if(c&0xff==0xff){
      c=0;
      spinnit("kB so far...",(int)(ftell(res)/1024));
    }
  }
  fclose(res);
  fclose(of);
  for(i=0;i<parts;i++)
    fclose(or[i]);
  fprintf(stderr,"\rDone                         \n");
  return(0);
}




