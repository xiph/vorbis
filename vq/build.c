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

 function: utility main for building codebooks from training sets
 last mod: $Id: build.c,v 1.7 1999/12/30 07:26:59 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vqgen.h"

static char *linebuffer=NULL;
static int  lbufsize=0;
static char *rline(FILE *in,FILE *out){
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
  long *quantlist=NULL;

  int entries=-1,dim=-1,aux=-1;
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
    
    ptr=strrchr(filename,'-');
    if(ptr){
      *ptr='\0';
      name=strdup(filename);
      sprintf(ptr,".vqh");
    }else{
      name=strdup(filename);
      strcat(filename,".vqh");
    }

    out=fopen(filename,"w");
    if(out==NULL){
      fprintf(stderr,"Unable to open %s for writing\n",filename);
      exit(1);
    }
  }

  /* suck in the trained book */

  /* read book type, but it doesn't matter */
  line=rline(in,out);
  
  line=rline(in,out);
  if(sscanf(line,"%d %d %d",&entries,&dim,&aux)!=3){
    fprintf(stderr,"Syntax error reading book file\n");
    exit(1);
  }
  
  /* just use it to allocate mem */
  vqgen_init(&v,dim,0,entries,NULL,NULL);
  
  /* quant */
  line=rline(in,out);
  if(sscanf(line,"%ld %ld %d %d",&q.min,&q.delta,
	    &q.quant,&q.sequencep)!=4){
    fprintf(stderr,"Syntax error reading book file\n");
    exit(1);
  }
  
  /* quantized entries */
  /* save quant data; we don't want to requantize later as our method
     is currently imperfect wrt repeated application */
  i=0;
  quantlist=malloc(sizeof(long)*v.elements*v.entries);
  for(j=0;j<entries;j++){
    double a;
    for(k=0;k<dim;k++){
      line=rline(in,out);
      sscanf(line,"%lf",&a);
      v.entrylist[i]=a;
      quantlist[i++]=rint(a);
    }
  }    
  
  /* ignore bias */
  for(j=0;j<entries;j++)line=rline(in,out);
  free(v.bias);
  v.bias=NULL;
  
  /* training points */
  {
    double *b=alloca(sizeof(double)*(dim+aux));
    i=0;
    v.entries=0; /* hack to avoid reseeding */
    while(1){
      for(k=0;k<dim+aux;k++){
	line=rline(in,out);
	if(!line)break;
	sscanf(line,"%lf",b+k);
      }
      if(feof(in))break;
      vqgen_addpoint(&v,b,NULL);
    }
    v.entries=entries;
  }
  
  fclose(in);
  vqgen_unquantize(&v,&q);

  /* build the book */
  vqsp_book(&v,&b,quantlist);

  /* save the book in C header form */
  fprintf(out,
 "/********************************************************************\n"
 " *                                                                  *\n"
 " * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *\n"
 " * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *\n"
 " * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *\n"
 " * PLEASE READ THESE TERMS DISTRIBUTING.                            *\n"
 " *                                                                  *\n"
 " * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *\n"
 " * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *\n"
 " * http://www.xiph.org/                                             *\n"
 " *                                                                  *\n"
 " ********************************************************************\n"
 "\n"
 " function: static codebook autogenerated by vq/vqbuild\n"
 "\n"
 " ********************************************************************/\n\n");

  fprintf(out,"#ifndef _V_%s_VQH_\n#define _V_%s_VQH_\n",name,name);
  fprintf(out,"#include \"vqgen.h\"\n\n");

  /* first, the static vectors, then the book structure to tie it together. */
  /* quantlist */
  fprintf(out,"static long _vq_quantlist_%s[] = {\n",name);
  i=0;
  for(j=0;j<entries;j++){
    fprintf(out,"\t");
    for(k=0;k<dim;k++)
      fprintf(out,"%5ld, ",b.quantlist[i++]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* codelist */
  fprintf(out,"static long _vq_codelist_%s[] = {\n",name);
  for(j=0;j<entries;){
    fprintf(out,"\t");
    for(k=0;k<8 && j<entries;k++,j++)
      fprintf(out,"%ld,",b.codelist[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* lengthlist */
  fprintf(out,"static long _vq_lengthlist_%s[] = {\n",name);
  for(j=0;j<entries;){
    fprintf(out,"\t");
    for(k=0;k<16 && j<entries;k++,j++)
      fprintf(out,"%2ld,",b.lengthlist[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* ptr0 */
  fprintf(out,"static long _vq_ptr0_%s[] = {\n",name);
  for(j=0;j<b.aux;){
    fprintf(out,"\t");
    for(k=0;k<8 && j<b.aux;k++,j++)
      fprintf(out,"%5ld,",b.ptr0[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* ptr1 */
  fprintf(out,"static long _vq_ptr1_%s[] = {\n",name);
  for(j=0;j<b.aux;){
    fprintf(out,"\t");
    for(k=0;k<8 && j<b.aux;k++,j++)
      fprintf(out,"%6ld,",b.ptr1[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* p */
  fprintf(out,"static long _vq_p_%s[] = {\n",name);
  for(j=0;j<b.aux;){
    fprintf(out,"\t");
    for(k=0;k<8 && j<b.aux;k++,j++)
      fprintf(out,"%6ld,",b.p[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* q */
  fprintf(out,"static long _vq_q_%s[] = {\n",name);
  for(j=0;j<b.aux;){
    fprintf(out,"\t");
    for(k=0;k<8 && j<b.aux;k++,j++)
      fprintf(out,"%6ld,",b.q[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  /* tie it all together */
  fprintf(out,"static vqbook _vq_book_%s = {\n",name);
  fprintf(out,"\t%ld, %ld, %ld, %ld, %d, %d,\n",
	  b.dim,b.entries,q.min,q.delta,q.quant,q.sequencep);
  fprintf(out,"\t0,\n"); /* valuelist */
  fprintf(out,"\t_vq_quantlist_%s,\n",name);
  fprintf(out,"\t_vq_codelist_%s,\n",name);
  fprintf(out,"\t_vq_lengthlist_%s,\n",name);
  fprintf(out,"\t_vq_ptr0_%s,\n",name);
  fprintf(out,"\t_vq_ptr1_%s,\n",name);
  fprintf(out,"\t0,\n",name);
  fprintf(out,"\t0,\n",name);
  fprintf(out,"\t_vq_p_%s,\n",name);
  fprintf(out,"\t_vq_q_%s,\n",name);
  fprintf(out,"\t%ld, %ld };\n\n",b.aux,b.aux);

  fprintf(out,"\n#endif\n");
  fclose(out);
  exit(0);
}
