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
 last mod: $Id: bookutil.c,v 1.12 2000/02/23 09:10:10 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include "vorbis/codebook.h"
#include "bookutil.h"

void codebook_unquantize(codebook *b){
  long j,k;
  const static_codebook *c=b->c;
  double mindel=float24_unpack(c->q_min);
  double delta=float24_unpack(c->q_delta);
  if(!b->valuelist)b->valuelist=malloc(sizeof(double)*c->entries*c->dim);
  
  for(j=0;j<c->entries;j++){
    double last=0.;
    for(k=0;k<c->dim;k++){
      double val=c->quantlist[j*c->dim+k]*delta+last+mindel;
      b->valuelist[j*c->dim+k]=val;
      if(c->q_sequencep)last=val;

    }
  }
}

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
  encode_aux *a=calloc(1,sizeof(encode_aux));
  FILE *in=fopen(filename,"r");
  char *line;
  long i;

  c->encode_tree=a;

  if(in==NULL){
    fprintf(stderr,"Couldn't open codebook %s\n",filename);
    exit(1);
  }

  /* find the codebook struct */
  find_seek_to(in,"static static_codebook _vq_book_");

  /* get the major important values */
  line=get_line(in);
  if(sscanf(line,"%ld, %ld, %ld, %ld, %d, %d",
	    &(c->dim),&(c->entries),&(c->q_min),&(c->q_delta),&(c->q_quant),
	    &(c->q_sequencep))!=6){
    fprintf(stderr,"1: syntax in %s in line:\t %s",filename,line);
    exit(1);
  }

  /* find the auxiliary encode struct (if any) */
  find_seek_to(in,"static encode_aux _vq_aux_");
  /* how big? */
  line=get_line(in);
  line=get_line(in);
  line=get_line(in);
  line=get_line(in);
  line=get_line(in);
  if(sscanf(line,"%ld, %ld",&(a->aux),&(a->alloc))!=2){
    fprintf(stderr,"2: syntax in %s in line:\t %s",filename,line);
    exit(1);
  }
    
  /* load the quantized entries */
  find_seek_to(in,"static long _vq_quantlist_");
  reset_next_value();
  c->quantlist=malloc(sizeof(long)*c->entries*c->dim);
  for(i=0;i<c->entries*c->dim;i++)
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

  /* got it all */
  fclose(in);

  /* unquantize the entries while we're at it */
  codebook_unquantize(b);

  /* don't need n and c */
  return(b);
}

int codebook_entry(codebook *b,double *val){
  const static_codebook *c=b->c;
  encode_aux *t=c->encode_tree;
  double *n=alloca(c->dim*sizeof(double));
  int ptr=0,k;

  do{
    double C=0.;
    double *p=b->valuelist+t->p[ptr];
    double *q=b->valuelist+t->q[ptr];
    
    for(k=0;k<c->dim;k++){
      n[k]=p[k]-q[k];
      C-=(p[k]+q[k])*n[k];
    }
    C/=2.;
    
    for(k=0;k<c->dim;k++)
      C+=n[k]*val[k];
    
    if(C>0.) /* in A */
      ptr=-t->ptr0[ptr];
    else     /* in B */
      ptr=-t->ptr1[ptr];
  }while(ptr>0);

  return(-ptr);
}

/* 24 bit float (not IEEE; nonnormalized mantissa +
   biased exponent ): neeeeemm mmmmmmmm mmmmmmmm */

long float24_pack(double val){
  int sign=0;
  long exp;
  long mant;
  if(val<0){
    sign=0x800000;
    val= -val;
  }
  exp= floor(log(val)/log(2));
  mant=rint(ldexp(val,17-exp));
  exp=(exp+VQ_FEXP_BIAS)<<18;

  return(sign|exp|mant);
}

double float24_unpack(long val){
  double mant=val&0x3ffff;
  double sign=val&0x800000;
  double exp =(val&0x7c0000)>>18;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-17-VQ_FEXP_BIAS));
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

  free(membership);
}
