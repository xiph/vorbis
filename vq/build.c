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

 function: utility main for building codebooks from training sets
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Dec 15 1999

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vqgen.h"

static char *linebuffer=NULL;
static int  lbufsize=0;
static char *rline(FILE *in,FILE *out,int pass){
  long sofar=0;
  if(feof(in))return NULL;

  while(1){
    int gotline=0;

    while(!gotline){
      if(sofar>=lbufsize){
	if(!lbufsize){	
	  lbufsize=1024;
	  linebuffer=malloc(lbufsize);
	}else{
	  lbufsize*=2;
	  linebuffer=realloc(linebuffer,lbufsize);
	}
      }
      {
	long c=fgetc(in);
	switch(c){
	case '\n':
	case EOF:
	  gotline=1;
	  break;
	default:
	  linebuffer[sofar++]=c;
	  linebuffer[sofar]='\0';
	  break;
	}
      }
    }
    
    if(linebuffer[0]=='#'){
      if(pass)fprintf(out,"%s\n",linebuffer);
      sofar=0;
    }else{
      return(linebuffer);
    }
  }
}

/* command line:
   buildvq file
*/

int main(int argc,char *argv[]){
  vqgen v;
  vqbook b;
  quant_meta q;
  int *quantlist=NULL;

  int entries=-1,dim=-1;
  FILE *out=NULL;
  FILE *in=NULL;
  char *line,*name;
  long i,j,k;

  if(argv[1]==NULL){
    fprintf(stderr,"Need a trained data set on the command line.\n");
    exit(1);
  }

  {
    char *ptr;
    char *filename=strdup(argv[1]);

    in=fopen(filename,"r");
    if(!in){
      fprintf(stderr,"Could not open input file %s\n",filename);
      exit(1);
    }
    
    ptr=strrchr(filename,'.');
    if(ptr){
      *ptr='\0';
      name=strdup(ptr);
      sprintf(ptr,".h");
    }else{
      name=strdup(filename);
      strcat(filename,".h");
    }

    out=fopen(filename,"w");
    if(out==NULL){
      fprintf(stderr,"Unable to open %s for writing\n",filename);
      exit(1);
    }
  }

  /* suck in the trained book */

  /* read book type, but it doesn't matter */
  line=rline(in,out,1);
  
  line=rline(in,out,1);
  if(sscanf(line,"%d %d",&entries,&dim)!=2){
    fprintf(stderr,"Syntax error reading book file\n");
    exit(1);
  }
  
  /* just use it to allocate mem */
  vqgen_init(&v,dim,entries,NULL);
  
  /* quant */
  line=rline(in,out,1);
  if(sscanf(line,"%ld %ld %d %d",&q.min,&q.delta,
	    &q.quant,&q.sequencep)!=4){
    fprintf(stderr,"Syntax error reading book file\n");
    exit(1);
  }
  
  /* quantized entries */
  /* save quant data; we don't want to requantize later as our method
     is currently imperfect wrt repeated application */
  i=0;
  quantlist=malloc(sizeof(int)*v.elements*v.entries);
  for(j=0;j<entries;j++){
    double a;
    for(k=0;k<dim;k++){
      line=rline(in,out,0);
      sscanf(line,"%lf",&a);
      v.entrylist[i]=a;
      quantlist[i++]=rint(a);
    }
  }    
  
  /* ignore bias */
  for(j=0;j<entries;j++)line=rline(in,out,0);
  free(v.bias);
  v.bias=NULL;
  
  /* training points */
  {
    double b[80];
    i=0;
    v.entries=0; /* hack to avoid reseeding */
    while(1){
      for(k=0;k<dim && k<80;k++){
	line=rline(in,out,0);
	if(!line)break;
	sscanf(line,"%lf",b+k);
      }
      if(feof(in))break;
      vqgen_addpoint(&v,b);
    }
    v.entries=entries;
  }
  
  fclose(in);
  vqgen_unquantize(&v,&q);

  /* build the book */
  vqsp_book(&v,&b);

  /* save the book in C header form */




  exit(0);
}
