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

 function: basic shared codebook operations
 last mod: $Id: sharedbook.c,v 1.1.2.2 2000/04/04 07:08:44 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/codebook.h"
#include "bitwise.h"
#include "scales.h"
#include "sharedbook.h"

/**** pack/unpack helpers ******************************************/
int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* 24 bit float (not IEEE; nonnormalized mantissa +
   biased exponent ): neeeeemm mmmmmmmm mmmmmmmm 
   Why not IEEE?  It's just not that important here. */

long _float24_pack(double val){
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

double _float24_unpack(long val){
  double mant=val&0x3ffff;
  double sign=val&0x800000;
  double exp =(val&0x7c0000)>>18;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-17-VQ_FEXP_BIAS));
}

/* given a list of word lengths, generate a list of codewords.  Works
   for length ordered or unordered, always assigns the lowest valued
   codewords first */
long *_make_words(long *l,long n){
  long i,j;
  long marker[33];
  long *r=malloc(n*sizeof(long));
  memset(marker,0,sizeof(marker));

  for(i=0;i<n;i++){
    long length=l[i];
    long entry=marker[length];

    /* when we claim a node for an entry, we also claim the nodes
       below it (pruning off the imagined tree that may have dangled
       from it) as well as blocking the use of any nodes directly
       above for leaves */

    /* update ourself */
    if(length<32 && (entry>>length)){
      /* error condition; the lengths must specify an overpopulated tree */
      free(r);
      return(NULL);
    }
    r[i]=entry;
    
    /* Look to see if the next shorter marker points to the node
       above. if so, update it and repeat.  */
    {
      for(j=length;j>0;j--){

	if(marker[j]&1){
	  /* have to jump branches */
	  if(j==1)
	    marker[1]++;
	  else
	    marker[j]=marker[j-1]<<1;
	  break; /* invariant says next upper marker would already
		    have been moved if it was on the same path */
	}
	marker[j]++;
      }
    }

    /* prune the tree; the implicit invariant says all the longer
       markers were dangling from our just-taken node.  Dangle them
       from our *new* node. */
    for(j=length+1;j<33;j++)
      if((marker[j]>>1) == entry){
	entry=marker[j];
	marker[j]=marker[j-1]<<1;
      }else
	break;
  }

  /* bitreverse the words because our bitwise packer/unpacker is LSb
     endian */
  for(i=0;i<n;i++){
    long temp=0;
    for(j=0;j<l[i];j++){
      temp<<=1;
      temp|=(r[i]>>j)&1;
    }
    r[i]=temp;
  }

  return(r);
}

/* build the decode helper tree from the codewords */
decode_aux *_make_decode_tree(codebook *c){
  const static_codebook *s=c->c;
  long top=0,i,j;
  decode_aux *t=malloc(sizeof(decode_aux));
  long *ptr0=t->ptr0=calloc(c->entries*2,sizeof(long));
  long *ptr1=t->ptr1=calloc(c->entries*2,sizeof(long));
  long *codelist=_make_words(s->lengthlist,s->entries);

  if(codelist==NULL)return(NULL);
  t->aux=c->entries*2;

  for(i=0;i<c->entries;i++){
    long ptr=0;
    for(j=0;j<s->lengthlist[i]-1;j++){
      int bit=(codelist[i]>>j)&1;
      if(!bit){
	if(!ptr0[ptr])
	  ptr0[ptr]= ++top;
	ptr=ptr0[ptr];
      }else{
	if(!ptr1[ptr])
	  ptr1[ptr]= ++top;
	ptr=ptr1[ptr];
      }
    }
    if(!((codelist[i]>>j)&1))
      ptr0[ptr]=-i;
    else
      ptr1[ptr]=-i;
  }
  free(codelist);
  return(t);
}

/* unpack the quantized list of values for encode/decode ***********/
double *_book_unquantize(const static_codebook *b){
  long j,k;
  if(b->quantlist){
    double mindel=_float24_unpack(b->q_min);
    double delta=_float24_unpack(b->q_delta);
    double *r=malloc(sizeof(double)*b->entries*b->dim);
    
    for(j=0;j<b->entries;j++){
      double last=0.;
      for(k=0;k<b->dim;k++){
	double val=b->quantlist[j*b->dim+k];
	if(val!=0){
	  val=(fabs(val)-1.)*delta+mindel+last;
	  if(b->q_sequencep)last=val;	  
	  if(b->q_log)val=fromdB(val);
	  if(b->quantlist[j*b->dim+k]<0)val= -val;
	}
	r[j*b->dim+k]=val;
      }
    }
    return(r);
  }else
    return(NULL);
}

/* on the encode side of a log scale, we need both the linear
   representation (done by unquantize above) but also a log version to
   speed error metric computation */
double *_book_logdist(const static_codebook *b,double *vals){
  long j;
  if(b->quantlist && b->q_log){
    double *r=malloc(sizeof(double)*b->entries*b->dim);
    for(j=0;j<b->entries;j++){
      if(vals[j]==0){
	r[j]=0.;
      }else{
	r[j]=todB(vals[j])+b->q_encodebias;
	if(vals[j]<0)r[j]= -r[j];
      }
    }
    return(r);
  }else
    return(NULL);
}

void vorbis_staticbook_clear(static_codebook *b){
  if(b->quantlist)free(b->quantlist);
  if(b->lengthlist)free(b->lengthlist);
  if(b->encode_tree){
    free(b->encode_tree->ptr0);
    free(b->encode_tree->ptr1);
    free(b->encode_tree->p);
    free(b->encode_tree->q);
    memset(b->encode_tree,0,sizeof(encode_aux));
    free(b->encode_tree);
  }
  memset(b,0,sizeof(static_codebook));
}

void vorbis_book_clear(codebook *b){
  /* static book is not cleared; we're likely called on the lookup and
     the static codebook belongs to the info struct */
  if(b->decode_tree){
    free(b->decode_tree->ptr0);
    free(b->decode_tree->ptr1);
    memset(b->decode_tree,0,sizeof(decode_aux));
    free(b->decode_tree);
  }
  if(b->valuelist)free(b->valuelist);
  if(b->logdist)free(b->logdist);
  if(b->codelist)free(b->codelist);
  memset(b,0,sizeof(codebook));
}

int vorbis_book_init_encode(codebook *c,const static_codebook *s){
  memset(c,0,sizeof(codebook));
  c->c=s;
  c->entries=s->entries;
  c->dim=s->dim;
  c->codelist=_make_words(s->lengthlist,s->entries);
  c->valuelist=_book_unquantize(s);
  c->logdist=_book_logdist(s,c->valuelist);
  return(0);
}

int vorbis_book_init_decode(codebook *c,const static_codebook *s){
  memset(c,0,sizeof(codebook));
  c->c=s;
  c->entries=s->entries;
  c->dim=s->dim;
  c->valuelist=_book_unquantize(s);
  c->decode_tree=_make_decode_tree(c);
  if(c->decode_tree==NULL)goto err_out;
  return(0);
 err_out:
  vorbis_book_clear(c);
  return(-1);
}

int _best(codebook *book, double *a, int step){
  encode_aux *t=book->c->encode_tree;
  int dim=book->dim;
  int ptr=0,k,o;

  /* optimized using the decision tree */
  while(1){
    double c=0.;
    double *p=book->valuelist+t->p[ptr];
    double *q=book->valuelist+t->q[ptr];
    
    for(k=0,o=0;k<dim;k++,o+=step)
      c+=(p[k]-q[k])*(a[o]-(p[k]+q[k])*.5);
    
    if(c>0.) /* in A */
      ptr= -t->ptr0[ptr];
    else     /* in B */
      ptr= -t->ptr1[ptr];
    if(ptr<=0)break;
  }
  return(-ptr);
}

int _logbest(codebook *book, double *a, int step){
  encode_aux *t=book->c->encode_tree;
  int dim=book->dim;
  int ptr=0,k,o;
  double *loga=alloca(sizeof(double)*dim);
  double bias=book->c->q_encodebias;
  
  for(k=0,o=0;k<dim;k++,o+=step)
    if(a[o]==0.)
      loga[k]=0.;
    else{
      loga[k]=todB(a[o])+bias;
      if(a[o]<0)loga[k]= -loga[k];
    }

  /* optimized using the decision tree */
  while(1){
    double c=0.;
    double *p=book->logdist+t->p[ptr];
    double *q=book->logdist+t->q[ptr];
    
    for(k=0;k<dim;k++)
      c+=(p[k]-q[k])*(loga[k]-(p[k]+q[k])*.5);
    
    if(c>0.) /* in A */
      ptr= -t->ptr0[ptr];
    else     /* in B */
      ptr= -t->ptr1[ptr];
    if(ptr<=0)break;
  }
  return(-ptr);
}

