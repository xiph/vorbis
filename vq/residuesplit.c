/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: residue backend 0 partitioner/classifier
 last mod: $Id: residuesplit.c,v 1.11 2001/08/13 01:37:17 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "bookutil.h"

/* does not guard against invalid settings; eg, a subn of 16 and a
   subgroup request of 32.  Max subn of 128 */
static float _testhack(float *vec,int n){
  int i,j=0;
  float max=0.f;
  float temp[128];
  float entropy=0.;

  /* setup */
  for(i=0;i<n;i++)temp[i]=fabs(vec[i]);

  /* handle case subgrp==1 outside */
  for(i=0;i<n;i++)
    if(temp[i]>max)max=temp[i];

  for(i=0;i<n;i++)temp[i]=rint(temp[i]);

  for(i=0;i<n;i++)
    entropy+=temp[i];
  return entropy;

  /*while(1){
    entropy[j]=max;
    n>>=1;
    j++;

    if(n<=0)break;
    for(i=0;i<n;i++){
      temp[i]+=temp[i+n];
    }
    max=0.f;
    for(i=0;i<n;i++)
      if(temp[i]>max)max=temp[i];
      }*/
}

static FILE *of;
static FILE **or;

/* we evaluate the the entropy measure for each interleaved subgroup */
/* This is currently a bit specific to/hardwired for mapping 0; things
   will need to change in the future when we get real multichannel
   mappings */
int quantaux(float *res,int n,float *ebound,float *mbound,int *subgrp,int parts, int subn){
  long i,j,part=0;
  int aux;

  for(i=0;i<=n-subn;i+=subn,part++){
    float max=0.f;
    float lentropy=0.f;

    lentropy=_testhack(res+i,subn);

    for(j=0;j<subn;j++)
      if(fabs(res[i+j])>max)max=fabs(res[i+j]);

    for(j=0;j<parts-1;j++)
      if(lentropy<=ebound[j] &&
	 max<=mbound[j] &&
	 part<subgrp[j])
	break;
    aux=j;
    
    fprintf(of,"%d, ",aux);
    
    for(j=0;j<subn;j++)
      fprintf(or[aux],"%g, ",res[j+i]);
    
    fprintf(or[aux],"\n");
  }

  fprintf(of,"\n");

  return(0);
}

static int getline(FILE *in,float *vec,int begin,int n){
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
	  "residuesplit <res> <begin,n,group> <baseout> <ent,peak,sub> [<ent,peak,sub>]...\n"
	  "   where begin,n,group is first scalar, \n"
	  "                          number of scalars of each in line,\n"
	  "                          number of scalars in a group\n"
	  "         ent is the maximum entropy value allowed for membership in a group\n"
	  "         peak is the maximum amplitude value allowed for membership in a group\n"
	  "         subn is the maximum subpartiton number allowed in the group\n"
	           
	  "eg: residuesplit mask.vqd floor.vqd 0,1024,16 res 0,.5,2 3,1.5,4 \n"
	  "produces resaux.vqd and res_0...n.vqd\n\n");
  exit(1);
}

int main(int argc, char *argv[]){
  char *buffer;
  char *base;
  int i,parts,begin,n,subn,*subgrp;
  FILE *res;
  float *ebound,*mbound,*vec;
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
  
  ebound=_ogg_malloc(sizeof(float)*parts);
  mbound=_ogg_malloc(sizeof(float)*parts);
  subgrp=_ogg_malloc(sizeof(int)*parts);
  
  for(i=0;i<parts-1;i++){
    char *pos=strchr(argv[4+i],',');
    subgrp[i]=0;
    if(*argv[4+i]==',')
      ebound[i]=1e50f;
    else
      ebound[i]=atof(argv[4+i]);

    if(!pos){
      mbound[i]=1e50f;
    }else{
      if(*(pos+1)==',')
	mbound[i]=1e50f;
      else
	mbound[i]=atof(pos+1);
      pos=strchr(pos+1,',');
      
       if(pos)
	 subgrp[i]=atoi(pos+1);
       
    }
    if(subgrp[i]<=0)subgrp[i]=99999;
  }

  ebound[i]=1e50f;
  mbound[i]=1e50f;
  subgrp[i]=9999999;

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
  
  vec=_ogg_malloc(sizeof(float)*n);
  /* get the input line by line and process it */
  while(!feof(res)){
    if(getline(res,vec,begin,n))
      quantaux(vec,n,ebound,mbound,subgrp,parts,subn);
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




