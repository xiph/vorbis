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

 function: residue backend 0 partitioner/classifier
 last mod: $Id: residuesplit.c,v 1.2 2000/05/08 20:49:42 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "../vq/bookutil.h"

static FILE *of;
static FILE **or;

/* currently classify only by maximum amplitude */

/* This is currently a bit specific to/hardwired for mapping 0; things
   will need to change in the future when we get real multichannel
   mappings */

int quantaux(double *res,int n,double *bound,int parts, int subn){
  long i,j,aux;
  
  for(i=0;i<=n-subn;i+=subn){
    double max=0.;
    for(j=0;j<subn;j++)
      if(fabs(res[i+j])>max)max=fabs(res[i+j]);
    
    for(j=0;j<parts-1;j++)
      if(max>=bound[j])
	break;
    aux=j;
    
    fprintf(of,"%ld, ",aux);
    
    for(j=0;j<subn;j++)
      fprintf(or[aux],"%g, ",res[j+i]);
    
    fprintf(or[aux],"\n");
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
	  "residuesplit <res> <begin,n,group> <baseout> <min> [<min>]...\n"
	  "   where begin,n,group is first scalar, \n"
	  "                          number of scalars of each in line,\n"
	  "                          number of scalars in a group\n"
	  "         min is the minimum peak value required for membership in a group\n"
	  "eg: residuesplit mask.vqd floor.vqd 0,1024,16 res 25.5 13.5 7.5 1.5 0\n"
	  "produces resaux.vqd and res_0...n.vqd\n\n");
  exit(1);
}

int main(int argc, char *argv[]){
  char *buffer;
  char *base;
  int i,parts,begin,n,subn;
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
  parts=argc-3;
  bound=malloc(sizeof(double)*parts);

  for(i=0;i<parts-1;i++){
    bound[i]=atof(argv[4+i]);
  }
  bound[i]=0;

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
      quantaux(vec,n,bound,parts,subn);
    c++;
    if(!(c&0xf)){
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




