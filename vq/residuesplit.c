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
 last mod: $Id: residuesplit.c,v 1.1 2000/02/12 08:33:01 xiphmont Exp $

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
   will need to change in the future when we het real multichannel
   mappings */

static double _maxval(double *v,int n){
  int i;
  double acc=0.;
  for(i=0;i<n;i++){
    double val=fabs(v[i]);
    if(acc<val)acc=val;
  }
  return(acc);
}

/* mean dB actually */
static double _meanval(double *v,int n){
  int i;
  double acc=0.;
  for(i=0;i<n;i++)
    acc+=todB(fabs(v[i]));
  return(fromdB(acc/n));
}

static FILE *of;
static FILE **or;

int quantaux(double *mask, double *floor,int n,
	     double *maskmbound,double *maskabound,double *floorbound,
	     int parts, int subn){
  long i,j;

  for(i=0;i<=n-subn;){
    double maxmask=_maxval(mask+i,subn);
    double meanmask=_meanval(mask+i,subn);
    double meanfloor=_meanval(floor+i,subn);
    int aux;

    for(j=0;j<parts-1;j++)
      if(maxmask<maskmbound[j] && 
	 meanmask<maskabound[j] &&
	 meanfloor<floorbound[j])
	break;
    aux=j;

    fprintf(of,"%d, ",aux);      

    for(j=0;j<subn;j++,i++)
      fprintf(or[aux],"%g, ",floor[i]);
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
	  "residuesplit <mask> <floor> <begin,n,group> <baseout> <m,a,f> [<m,a,f>]...\n"
	  "   where begin,n,group is first scalar, \n"
	  "                          number of scalars of each in line,\n"
	  "                          number of scalars in a group\n"
	  "         m,a,f are the boundary conditions for each group\n"
	  "eg: residuesplit mask.vqd floor.vqd 0,1024,32 res .5 2.5,1.5 ,,.25\n"
	  "produces resaux.vqd and res_0...n.vqd\n\n");
  exit(1);
}

int main(int argc, char *argv[]){
  char *buffer;
  char *base;
  int i,parts,begin,n,subn;
  FILE *mask;
  FILE *floor;
  double *maskmbound,*maskabound,*maskvec;
  double *floorbound,*floorvec;

  if(argc<6)usage();

  base=strdup(argv[4]);
  buffer=alloca(strlen(base)+20);
  {
    char *pos=strchr(argv[3],',');
    begin=atoi(argv[3]);
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

  /* how many parts?  Need to scan m,f... */
  parts=argc-4; /* yes, one past */
  maskmbound=malloc(sizeof(double)*parts);
  maskabound=malloc(sizeof(double)*parts);
  floorbound=malloc(sizeof(double)*parts);

  for(i=0;i<parts-1;i++){
    char *pos=strchr(argv[5+i],',');
    maskmbound[i]=atof(argv[5+i]);
    if(*argv[5+i]==',')maskmbound[i]=1e50;
    if(!pos){
      maskabound[i]=1e50;
      floorbound[i]=1e50;
    }else{
      maskabound[i]=atof(pos+1);
      if(pos[1]==',')maskabound[i]=1e50;
      pos=strchr(pos+1,',');
      if(!pos)
	floorbound[i]=1e50;
      else{
	floorbound[i]=atof(pos+1);
      }
    }
  }
  maskmbound[i]=1e50;
  maskabound[i]=1e50;
  floorbound[i]=1e50;

  mask=fopen(argv[1],"r");
  if(!mask){
    fprintf(stderr,"Could not open file %s\n",argv[1]);
    exit(1);
  }
  floor=fopen(argv[2],"r");
  if(!mask){
    fprintf(stderr,"Could not open file %s\n",argv[2]);
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
  
  maskvec=malloc(sizeof(double)*n);
  floorvec=malloc(sizeof(double)*n);
  /* get the input line by line and process it */
  while(!feof(mask) && !feof(floor)){
    if(getline(mask,maskvec,begin,n) &&
       getline(floor,floorvec,begin,n)) 
      quantaux(maskvec,floorvec,n,
	       maskmbound,maskabound,floorbound,parts,subn);
    
  }
  fclose(mask);
  fclose(floor);
  fclose(of);
  for(i=0;i<parts;i++)
    fclose(or[i]);
  return(0);
}




