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

 function: utility functions for loading .vqh and .vqd files
 last mod: $Id: bookutil.c,v 1.13 2000/05/08 20:49:50 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vorbis/codebook.h"
#include "../lib/sharedbook.h"
#include "bookutil.h"

/* A few little utils for reading files */
/* read a line.  Use global, persistent buffering */
static char *linebuffer=NULL;
static int  lbufsize=0;
char *get_line(FILE *in){
  long sofar=0;
  if(feof(in))return NULL;

  while(1){
    int gotline=0;

    while(!gotline){
      if(sofar+1>=lbufsize){
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
        case EOF:
	  if(sofar==0)return(NULL);
	  /* fallthrough correct */
        case '\n':
          linebuffer[sofar]='\0';
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

/* read the next numerical value from the given file */
static char *value_line_buff=NULL;

int get_line_value(FILE *in,double *value){
  char *next;

  if(!value_line_buff)return(-1);

  *value=strtod(value_line_buff, &next);
  if(next==value_line_buff){
    value_line_buff=NULL;
    return(-1);
  }else{
    value_line_buff=next;
    while(*value_line_buff>44)value_line_buff++;
    if(*value_line_buff==44)value_line_buff++;
    return(0);
  }
}

int get_next_value(FILE *in,double *value){
  while(1){
    if(get_line_value(in,value)){
      value_line_buff=get_line(in);
      if(!value_line_buff)return(-1);
    }else{
      return(0);
    }
  }
}

int get_next_ivalue(FILE *in,long *ivalue){
  double value;
  int ret=get_next_value(in,&value);
  *ivalue=value;
  return(ret);
}

static double sequence_base=0.;
static int v_sofar=0;
void reset_next_value(void){
  value_line_buff=NULL;
  sequence_base=0.;
  v_sofar=0;
}

char *setup_line(FILE *in){
  reset_next_value();
  value_line_buff=get_line(in);
  return(value_line_buff);
}


int get_vector(codebook *b,FILE *in,int start, int n,double *a){
  int i;
  const static_codebook *c=b->c;

  while(1){

    if(v_sofar==n || get_line_value(in,a)){
      reset_next_value();
      if(get_next_value(in,a))
	break;
      for(i=0;i<start;i++){
	sequence_base=*a;
	get_line_value(in,a);
      }
    }

    for(i=1;i<c->dim;i++)
      if(get_line_value(in,a+i))
	break;
    
    if(i==c->dim){
      double temp=a[c->dim-1];
      for(i=0;i<c->dim;i++)a[i]-=sequence_base;
      if(c->q_sequencep)sequence_base=temp;
      v_sofar++;
      return(0);
    }
    sequence_base=0.;
  }

  return(-1);
}

/* read lines fromt he beginning until we find one containing the
   specified string */
char *find_seek_to(FILE *in,char *s){
  rewind(in);
  while(1){
    char *line=get_line(in);
    if(line){
      if(strstr(line,s))
	return(line);
    }else
      return(NULL);
  }
}


/* this reads the format as written by vqbuild; innocent (legal)
   tweaking of the file that would not affect its valid header-ness
   will break this routine */

codebook *codebook_load(char *filename){
  codebook *b=calloc(1,sizeof(codebook));
  static_codebook *c=(static_codebook *)(b->c=calloc(1,sizeof(static_codebook)));
  encode_aux_nearestmatch *a=NULL;
  encode_aux_threshmatch *t=NULL;
  int quant_to_read=0;
  FILE *in=fopen(filename,"r");
  char *line;
  long i;

  if(in==NULL){
    fprintf(stderr,"Couldn't open codebook %s\n",filename);
    exit(1);
  }

  /* find the codebook struct */
  find_seek_to(in,"static static_codebook _vq_book_");

  /* get the major important values */
  line=get_line(in);
  if(sscanf(line,"%ld, %ld,",
	    &(c->dim),&(c->entries))!=2){
    fprintf(stderr,"1: syntax in %s in line:\t %s",filename,line);
    exit(1);
  }
  line=get_line(in);
  line=get_line(in);
  if(sscanf(line,"%d, %ld, %ld, %d, %d,",
	    &(c->maptype),&(c->q_min),&(c->q_delta),&(c->q_quant),
	    &(c->q_sequencep))!=5){
    fprintf(stderr,"1: syntax in %s in line:\t %s",filename,line);
    exit(1);
  }
  
  /* find the auxiliary encode struct[s] (if any) */
  if(find_seek_to(in,"static encode_aux_nearestmatch _vq_aux")){
    /* how big? */
    c->nearest_tree=a=calloc(1,sizeof(encode_aux_nearestmatch));
    line=get_line(in);
    line=get_line(in);
    line=get_line(in);
    line=get_line(in);
    line=get_line(in);
    if(sscanf(line,"%ld, %ld",&(a->aux),&(a->alloc))!=2){
      fprintf(stderr,"2: syntax in %s in line:\t %s",filename,line);
      exit(1);
    }

    /* load ptr0 */
    find_seek_to(in,"static long _vq_ptr0");
    reset_next_value();
    a->ptr0=malloc(sizeof(long)*a->aux);
    for(i=0;i<a->aux;i++)
      if(get_next_ivalue(in,a->ptr0+i)){
	fprintf(stderr,"out of data while reading codebook %s\n",filename);
	exit(1);
      }
    
    /* load ptr1 */
    find_seek_to(in,"static long _vq_ptr1");
    reset_next_value();
    a->ptr1=malloc(sizeof(long)*a->aux);
    for(i=0;i<a->aux;i++)
      if(get_next_ivalue(in,a->ptr1+i)){
	fprintf(stderr,"out of data while reading codebook %s\n",filename);
	exit(1);
    }
    
    
    /* load p */
    find_seek_to(in,"static long _vq_p_");
    reset_next_value();
    a->p=malloc(sizeof(long)*a->aux);
    for(i=0;i<a->aux;i++)
      if(get_next_ivalue(in,a->p+i)){
	fprintf(stderr,"out of data while reading codebook %s\n",filename);
	exit(1);
      }
    
    /* load q */
    find_seek_to(in,"static long _vq_q_");
    reset_next_value();
    a->q=malloc(sizeof(long)*a->aux);
    for(i=0;i<a->aux;i++)
      if(get_next_ivalue(in,a->q+i)){
	fprintf(stderr,"out of data while reading codebook %s\n",filename);
	exit(1);
      }    
  }
  
  if(find_seek_to(in,"static encode_aux_threshmatch _vq_aux")){
    /* how big? */
    c->thresh_tree=t=calloc(1,sizeof(encode_aux_threshmatch));
    line=get_line(in);
    line=get_line(in);
    line=get_line(in);
    if(sscanf(line,"%d",&(t->quantvals))!=1){
      fprintf(stderr,"3: syntax in %s in line:\t %s",filename,line);
      exit(1);
    }
    line=get_line(in);
    if(sscanf(line,"%d",&(t->threshvals))!=1){
      fprintf(stderr,"4: syntax in %s in line:\t %s",filename,line);
      exit(1);
    }
    /* load quantthresh */
    find_seek_to(in,"static double _vq_quantthresh_");
    reset_next_value();
    t->quantthresh=malloc(sizeof(double)*t->threshvals);
    for(i=0;i<t->threshvals-1;i++)
      if(get_next_value(in,t->quantthresh+i)){
	fprintf(stderr,"out of data 1 while reading codebook %s\n",filename);
	exit(1);
      }    
    /* load quantmap */
    find_seek_to(in,"static long _vq_quantmap_");
    reset_next_value();
    t->quantmap=malloc(sizeof(long)*t->threshvals);
    for(i=0;i<t->threshvals;i++)
      if(get_next_ivalue(in,t->quantmap+i)){
	fprintf(stderr,"out of data 2 while reading codebook %s\n",filename);
	exit(1);
      }    
  }
    
  switch(c->maptype){
  case 0:
    quant_to_read=0;
    break;
  case 1:
    quant_to_read=_book_maptype1_quantvals(c);
    break;
  case 2:
    quant_to_read=c->entries*c->dim;
    break;
  }
    
  /* load the quantized entries */
  find_seek_to(in,"static long _vq_quantlist_");
  reset_next_value();
  c->quantlist=malloc(sizeof(long)*quant_to_read);
  for(i=0;i<quant_to_read;i++)
    if(get_next_ivalue(in,c->quantlist+i)){
      fprintf(stderr,"out of data while reading codebook %s\n",filename);
      exit(1);
    }
  
  /* load the lengthlist */
  find_seek_to(in,"static long _vq_lengthlist");
  reset_next_value();
  c->lengthlist=malloc(sizeof(long)*c->entries);
  for(i=0;i<c->entries;i++)
    if(get_next_ivalue(in,c->lengthlist+i)){
      fprintf(stderr,"out of data while reading codebook %s\n",filename);
      exit(1);
    }

  /* got it all */
  fclose(in);
  
  vorbis_book_init_encode(b,c);

  return(b);
}

void spinnit(char *s,int n){
  static int p=0;
  static long lasttime=0;
  long test;
  struct timeval thistime;

  gettimeofday(&thistime,NULL);
  test=thistime.tv_sec*10+thistime.tv_usec/100000;
  if(lasttime!=test){
    lasttime=test;

    fprintf(stderr,"%s%d ",s,n);

    p++;if(p>3)p=0;
    switch(p){
    case 0:
      fprintf(stderr,"|    \r");
      break;
    case 1:
      fprintf(stderr,"/    \r");
      break;
    case 2:
      fprintf(stderr,"-    \r");
      break;
    case 3:
      fprintf(stderr,"\\    \r");
      break;
    }
    fflush(stderr);
  }
}

void build_tree_from_lengths(int vals, long *hist, long *lengths){
  int i,j;
  long *membership=malloc(vals*sizeof(long));
  long *histsave=alloca(vals*sizeof(long));
  memcpy(histsave,hist,vals*sizeof(long));

  for(i=0;i<vals;i++)membership[i]=i;

  /* find codeword lengths */
  /* much more elegant means exist.  Brute force n^2, minimum thought */
  for(i=vals;i>1;i--){
    int first=-1,second=-1;
    long least=-1;
	
    spinnit("building... ",i);
    
    /* find the two nodes to join */
    for(j=0;j<vals;j++)
      if(least==-1 || hist[j]<least){
	least=hist[j];
	first=membership[j];
      }
    least=-1;
    for(j=0;j<vals;j++)
      if((least==-1 || hist[j]<least) && membership[j]!=first){
	least=hist[j];
	second=membership[j];
      }
    if(first==-1 || second==-1){
      fprintf(stderr,"huffman fault; no free branch\n");
      exit(1);
    }
    
    /* join them */
    least=hist[first]+hist[second];
    for(j=0;j<vals;j++)
      if(membership[j]==first || membership[j]==second){
	membership[j]=first;
	hist[j]=least;
	lengths[j]++;
      }
  }
  for(i=0;i<vals-1;i++)
    if(membership[i]!=membership[i+1]){
      fprintf(stderr,"huffman fault; failed to build single tree\n");
      exit(1);
    }

  /* for sanity check purposes: how many bits would it have taken to
     encode the training set? */
  {
    long bitsum=0;
    long samples=0;
    for(i=0;i<vals;i++){
      bitsum+=(histsave[i]-1)*lengths[i];
      samples+=histsave[i]-1;
    }
    fprintf(stderr,"\rTotal samples in training set: %ld      \n",samples);
    fprintf(stderr,"\rTotal bits used to represent training set: %ld\n",
	    bitsum);
  }

  free(membership);
}

void write_codebook(FILE *out,char *name,const static_codebook *c){
  encode_aux_threshmatch *t=c->thresh_tree;
  encode_aux_nearestmatch *n=c->nearest_tree;
  int j,k;

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
 " function: static codebook autogenerated by vq/somethingorother\n"
 "\n"
 " ********************************************************************/\n\n");

  fprintf(out,"#ifndef _V_%s_VQH_\n#define _V_%s_VQH_\n",name,name);
  fprintf(out,"#include \"vorbis/codebook.h\"\n\n");

  /* first, the static vectors, then the book structure to tie it together. */
  /* quantlist */
  if(c->quantlist){
    long vals=(c->maptype==1?_book_maptype1_quantvals(c):c->entries*c->dim);
    fprintf(out,"static long _vq_quantlist_%s[] = {\n",name);
    for(j=0;j<vals;j++){
      fprintf(out,"\t%ld,\n",c->quantlist[j]);
    }
    fprintf(out,"};\n\n");
  }

  /* lengthlist */
  fprintf(out,"static long _vq_lengthlist_%s[] = {\n",name);
  for(j=0;j<c->entries;){
    fprintf(out,"\t");
    for(k=0;k<16 && j<c->entries;k++,j++)
      fprintf(out,"%2ld,",c->lengthlist[j]);
    fprintf(out,"\n");
  }
  fprintf(out,"};\n\n");

  if(t){
    /* quantthresh */
    fprintf(out,"static double _vq_quantthresh_%s[] = {\n",name);
    for(j=0;j<t->threshvals-1;){
      fprintf(out,"\t");
      for(k=0;k<8 && j<t->threshvals-1;k++,j++)
	fprintf(out,"%.5g, ",t->quantthresh[j]);
      fprintf(out,"\n");
    }
    fprintf(out,"};\n\n");

    /* quantmap */
    fprintf(out,"static long _vq_quantmap_%s[] = {\n",name);
    for(j=0;j<t->threshvals;){
      fprintf(out,"\t");
      for(k=0;k<8 && j<t->threshvals;k++,j++)
	fprintf(out,"%5ld,",t->quantmap[j]);
      fprintf(out,"\n");
    }
    fprintf(out,"};\n\n");  

    fprintf(out,"static encode_aux_threshmatch _vq_auxt_%s = {\n",name);
    fprintf(out,"\t_vq_quantthresh_%s,\n",name);
    fprintf(out,"\t_vq_quantmap_%s,\n",name);
    fprintf(out,"\t%d,\n",t->quantvals);
    fprintf(out,"\t%d\n};\n\n",t->threshvals);
  }

  if(n){
    
    /* ptr0 */
    fprintf(out,"static long _vq_ptr0_%s[] = {\n",name);
    for(j=0;j<n->aux;){
      fprintf(out,"\t");
      for(k=0;k<8 && j<n->aux;k++,j++)
	fprintf(out,"%6ld,",n->ptr0[j]);
      fprintf(out,"\n");
    }
    fprintf(out,"};\n\n");
    
    /* ptr1 */
    fprintf(out,"static long _vq_ptr1_%s[] = {\n",name);
    for(j=0;j<n->aux;){
      fprintf(out,"\t");
      for(k=0;k<8 && j<n->aux;k++,j++)
	fprintf(out,"%6ld,",n->ptr1[j]);
      fprintf(out,"\n");
    }
    fprintf(out,"};\n\n");
    
    /* p */
    fprintf(out,"static long _vq_p_%s[] = {\n",name);
    for(j=0;j<n->aux;){
      fprintf(out,"\t");
      for(k=0;k<8 && j<n->aux;k++,j++)
	fprintf(out,"%6ld,",n->p[j]*c->dim);
      fprintf(out,"\n");
    }
    fprintf(out,"};\n\n");
    
    /* q */
    fprintf(out,"static long _vq_q_%s[] = {\n",name);
    for(j=0;j<n->aux;){
      fprintf(out,"\t");
      for(k=0;k<8 && j<n->aux;k++,j++)
	fprintf(out,"%6ld,",n->q[j]*c->dim);
      fprintf(out,"\n");
    }
    fprintf(out,"};\n\n");
  
    fprintf(out,"static encode_aux_nearestmatch _vq_auxn_%s = {\n",name);
    fprintf(out,"\t_vq_ptr0_%s,\n",name);
    fprintf(out,"\t_vq_ptr1_%s,\n",name);
    fprintf(out,"\t_vq_p_%s,\n",name);
    fprintf(out,"\t_vq_q_%s,\n",name);
    fprintf(out,"\t%ld, %ld\n};\n\n",n->aux,n->aux);
  }

  /* tie it all together */
  
  fprintf(out,"static static_codebook _vq_book_%s = {\n",name);
  
  fprintf(out,"\t%ld, %ld,\n",c->dim,c->entries);
  fprintf(out,"\t_vq_lengthlist_%s,\n",name);
  fprintf(out,"\t%d, %ld, %ld, %d, %d,\n",
          c->maptype,c->q_min,c->q_delta,c->q_quant,c->q_sequencep);
  if(c->quantlist)
    fprintf(out,"\t_vq_quantlist_%s,\n",name);
  else
    fprintf(out,"\tNULL,\n");

  if(n)
    fprintf(out,"\t&_vq_auxn_%s,\n",name);
  else
    fprintf(out,"\tNULL,\n");
  if(t)
    fprintf(out,"\t&_vq_auxt_%s,\n",name);
  else
    fprintf(out,"\tNULL,\n");

  fprintf(out,"};\n\n");

  fprintf(out,"\n#endif\n");
}
