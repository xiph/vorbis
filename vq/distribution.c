/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: utility for finding the distribution in a data set
 last mod: $Id: distribution.c,v 1.1.2.1 2000/12/30 03:00:49 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "bookutil.h"

/* command line:
   distribution file.vqd
*/

int main(int argc,char *argv[]){
  FILE *in;
  long lines=0;
  float min;
  float max;
  long bins=-1;
  int flag=0;
  long *countarray;
  long total=0;
  char *line;

  if(argv[1]==NULL){
    fprintf(stderr,"Usage: distribution file.vqd [bins]\n\n");
    exit(1);
  }
  if(argv[2]!=NULL)
    bins=atoi(argv[2])-1;

  in=fopen(argv[1],"r");
  if(!in){
    fprintf(stderr,"Could not open input file %s\n",argv[1]);
    exit(1);
  }

  /* do it the simple way; two pass. */
  line=setup_line(in);
  while(line){      
    float code;
    lines++;
    if(!(lines&0xff))spinnit("getting min/max. lines so far...",lines);
    
    while(!flag && sscanf(line,"%f",&code)==1){
      line=strchr(line,',');
      min=max=code;
      flag=1;
    }

    while(sscanf(line,"%f",&code)==1){
      line=strchr(line,',');
      if(code<min)min=code;
      if(code>max)max=code;
    }
    
    line=setup_line(in);
  }

  if(bins<1){
    if((int)(max-min)==min-max){
      bins=max-min;
    }else{
      bins=25;
    }
  }

  printf("\r                                                     \r");
  printf("Minimum scalar value: %f\n",min);
  printf("Maximum scalar value: %f\n",max);

  printf("\n counting hits into %d bins...\n",bins+1);
  countarray=calloc(bins+1,sizeof(long));

  rewind(in);
  line=setup_line(in);
  while(line){      
    float code;
    lines--;
    if(!(lines&0xff))spinnit("counting distribution. lines so far...",lines);
    
    while(sscanf(line,"%f",&code)==1){
      line=strchr(line,',');

      code-=min;
      code/=(max-min);
      code*=bins;
      countarray[(int)rint(code)]++;
      total++;
    }
    
    line=setup_line(in);
  }


  fclose(in);

  /* make a pretty graph */
  {
    long maxcount=0,i,j;
    for(i=0;i<bins+1;i++)
      if(countarray[i]>maxcount)maxcount=countarray[i];

    printf("\r                                                     \r");
    for(i=0;i<bins+1;i++){
      int stars=rint(50./maxcount*countarray[i]);
      printf("%08f (%8ld) |",(max-min)/bins*i+min,countarray[i]);
      for(j=0;j<stars;j++)printf("*");
      printf("\n");
    }
  }
  printf("\nDone.\n");
  exit(0);
}
