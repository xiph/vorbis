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

 function: basic codebook pack/unpack/code/decode operations
 last mod: $Id: codebook.c,v 1.17 2000/07/07 01:37:00 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/codebook.h"
#include "bitwise.h"
#include "scales.h"
#include "sharedbook.h"
#include "bookinternal.h"
#include "misc.h"
#include "os.h"

/* packs the given codebook into the bitstream **************************/

int vorbis_staticbook_pack(const static_codebook *c,oggpack_buffer *opb){
  long i,j;
  int ordered=0;

  /* first the basic parameters */
  _oggpack_write(opb,0x564342,24);
  _oggpack_write(opb,c->dim,16);
  _oggpack_write(opb,c->entries,24);

  /* pack the codewords.  There are two packings; length ordered and
     length random.  Decide between the two now. */
  
  for(i=1;i<c->entries;i++)
    if(c->lengthlist[i]<c->lengthlist[i-1])break;
  if(i==c->entries)ordered=1;
  
  if(ordered){
    /* length ordered.  We only need to say how many codewords of
       each length.  The actual codewords are generated
       deterministically */

    long count=0;
    _oggpack_write(opb,1,1);  /* ordered */
    _oggpack_write(opb,c->lengthlist[0]-1,5); /* 1 to 32 */

    for(i=1;i<c->entries;i++){
      long this=c->lengthlist[i];
      long last=c->lengthlist[i-1];
      if(this>last){
	for(j=last;j<this;j++){
	  _oggpack_write(opb,i-count,_ilog(c->entries-count));
	  count=i;
	}
      }
    }
    _oggpack_write(opb,i-count,_ilog(c->entries-count));
    
  }else{
    /* length random.  Again, we don't code the codeword itself, just
       the length.  This time, though, we have to encode each length */
    _oggpack_write(opb,0,1);   /* unordered */
    
    /* algortihmic mapping has use for 'unused entries', which we tag
       here.  The algorithmic mapping happens as usual, but the unused
       entry has no codeword. */
    for(i=0;i<c->entries;i++)
      if(c->lengthlist[i]==0)break;

    if(i==c->entries){
      _oggpack_write(opb,0,1); /* no unused entries */
      for(i=0;i<c->entries;i++)
	_oggpack_write(opb,c->lengthlist[i]-1,5);
    }else{
      _oggpack_write(opb,1,1); /* we have unused entries; thus we tag */
      for(i=0;i<c->entries;i++){
	if(c->lengthlist[i]==0){
	  _oggpack_write(opb,0,1);
	}else{
	  _oggpack_write(opb,1,1);
	  _oggpack_write(opb,c->lengthlist[i]-1,5);
	}
      }
    }
  }

  /* is the entry number the desired return value, or do we have a
     mapping? If we have a mapping, what type? */
  _oggpack_write(opb,c->maptype,4);
  switch(c->maptype){
  case 0:
    /* no mapping */
    break;
  case 1:case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */
    
    if(!c->quantlist){
      /* no quantlist?  error */
      return(-1);
    }
    
    /* values that define the dequantization */
    _oggpack_write(opb,c->q_min,32);
    _oggpack_write(opb,c->q_delta,32);
    _oggpack_write(opb,c->q_quant-1,4);
    _oggpack_write(opb,c->q_sequencep,1);
    
    {
      int quantvals;
      switch(c->maptype){
      case 1:
	/* a single column of (c->entries/c->dim) quantized values for
	   building a full value list algorithmically (square lattice) */
	quantvals=_book_maptype1_quantvals(c);
	break;
      case 2:
	/* every value (c->entries*c->dim total) specified explicitly */
	quantvals=c->entries*c->dim;
	break;
      }

      /* quantized values */
      for(i=0;i<quantvals;i++)
	_oggpack_write(opb,labs(c->quantlist[i]),c->q_quant);

    }
    break;
  default:
    /* error case; we don't have any other map types now */
    return(-1);
  }

  return(0);
}

/* unpacks a codebook from the packet buffer into the codebook struct,
   readies the codebook auxiliary structures for decode *************/
int vorbis_staticbook_unpack(oggpack_buffer *opb,static_codebook *s){
  long i,j;
  memset(s,0,sizeof(static_codebook));

  /* make sure alignment is correct */
  if(_oggpack_read(opb,24)!=0x564342)goto _eofout;

  /* first the basic parameters */
  s->dim=_oggpack_read(opb,16);
  s->entries=_oggpack_read(opb,24);
  if(s->entries==-1)goto _eofout;

  /* codeword ordering.... length ordered or unordered? */
  switch(_oggpack_read(opb,1)){
  case 0:
    /* unordered */
    s->lengthlist=malloc(sizeof(long)*s->entries);

    /* allocated but unused entries? */
    if(_oggpack_read(opb,1)){
      /* yes, unused entries */

      for(i=0;i<s->entries;i++){
	if(_oggpack_read(opb,1)){
	  long num=_oggpack_read(opb,5);
	  if(num==-1)goto _eofout;
	  s->lengthlist[i]=num+1;
	}else
	  s->lengthlist[i]=0;
      }
    }else{
      /* all entries used; no tagging */
      for(i=0;i<s->entries;i++){
	long num=_oggpack_read(opb,5);
	if(num==-1)goto _eofout;
	s->lengthlist[i]=num+1;
      }
    }
    
    break;
  case 1:
    /* ordered */
    {
      long length=_oggpack_read(opb,5)+1;
      s->lengthlist=malloc(sizeof(long)*s->entries);

      for(i=0;i<s->entries;){
	long num=_oggpack_read(opb,_ilog(s->entries-i));
	if(num==-1)goto _eofout;
	for(j=0;j<num;j++,i++)
	  s->lengthlist[i]=length;
	length++;
      }
    }
    break;
  default:
    /* EOF */
    return(-1);
  }
  
  /* Do we have a mapping to unpack? */
  switch((s->maptype=_oggpack_read(opb,4))){
  case 0:
    /* no mapping */
    break;
  case 1: case 2:
    /* implicitly populated value mapping */
    /* explicitly populated value mapping */

    s->q_min=_oggpack_read(opb,32);
    s->q_delta=_oggpack_read(opb,32);
    s->q_quant=_oggpack_read(opb,4)+1;
    s->q_sequencep=_oggpack_read(opb,1);

    {
      int quantvals;
      switch(s->maptype){
      case 1:
	quantvals=_book_maptype1_quantvals(s);
	break;
      case 2:
	quantvals=s->entries*s->dim;
	break;
      }
      
      /* quantized values */
      s->quantlist=malloc(sizeof(double)*quantvals);
      for(i=0;i<quantvals;i++)
	s->quantlist[i]=_oggpack_read(opb,s->q_quant);
      
      if(s->quantlist[quantvals-1]==-1)goto _eofout;
    }
    break;
  default:
    goto _errout;
  }

  /* all set */
  return(0);
  
 _errout:
 _eofout:
  vorbis_staticbook_clear(s);
  return(-1); 
}

/* returns the number of bits ************************************************/
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  _oggpack_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

/* One the encode side, our vector writers are each designed for a
specific purpose, and the encoder is not flexible without modification:

The LSP vector coder uses a single stage nearest-match with no
interleave, so no step and no error return.  This is specced by floor0
and doesn't change.

Residue0 encoding interleaves, uses multiple stages, and each stage
peels of a specific amount of resolution from a lattice (thus we want
to match by threshhold, not nearest match).  Residue doesn't *have* to
be encoded that way, but to change it, one will need to add more
infrastructure on the encode side (decode side is specced and simpler) */

/* floor0 LSP (single stage, non interleaved, nearest match) */
/* returns entry number and *modifies a* to the quantization value *****/
int vorbis_book_errorv(codebook *book,double *a){
  int dim=book->dim,k;
  int best=_best(book,a,1);
  for(k=0;k<dim;k++)
    a[k]=(book->valuelist+best*dim)[k];
  return(best);
}

/* returns the number of bits and *modifies a* to the quantization value *****/
int vorbis_book_encodev(codebook *book,int best,double *a,oggpack_buffer *b){
  int k,dim=book->dim;
  for(k=0;k<dim;k++)
    a[k]=(book->valuelist+best*dim)[k];
  return(vorbis_book_encode(book,best,b));
}

/* res0 (multistage, interleave, lattice) */
/* returns the number of bits and *modifies a* to the remainder value ********/
int vorbis_book_encodevs(codebook *book,double *a,oggpack_buffer *b,
			 int step,int addmul){

  int best=vorbis_book_besterror(book,a,step,addmul);
  return(vorbis_book_encode(book,best,b));
}

/* Decode side is specced and easier, because we don't need to find
   matches using different criteria; we simply read and map.  There are
   two things we need to do 'depending':
   
   We may need to support interleave.  We don't really, but it's
   convenient to do it here rather than rebuild the vector later.

   Cascades may be additive or multiplicitive; this is not inherent in
   the codebook, but set in the code using the codebook.  Like
   interleaving, it's easiest to do it here.  
   stage==0 -> declarative (set the value)
   stage==1 -> additive
   stage==2 -> multiplicitive */

/* returns the entry number or -1 on eof *************************************/
long vorbis_book_decode(codebook *book, oggpack_buffer *b){
  long ptr=0;
  decode_aux *t=book->decode_tree;
  do{
    switch(_oggpack_read1(b)){
    case 0:
      ptr=t->ptr0[ptr];
      break;
    case 1:
      ptr=t->ptr1[ptr];
      break;
    case -1:
      return(-1);
    }
  }while(ptr>0);
  return(-ptr);
}

/* returns the entry number or -1 on eof *************************************/
long vorbis_book_decodevs(codebook *book,double *a,oggpack_buffer *b,
			  int step,int addmul){
  long entry=vorbis_book_decode(book,b);
  int i,o;
  if(entry==-1)return(-1);
  switch(addmul){
  case -1:
    for(i=0,o=0;i<book->dim;i++,o+=step)
      a[o]=(book->valuelist+entry*book->dim)[i];
    break;
  case 0:
    for(i=0,o=0;i<book->dim;i++,o+=step)
      a[o]+=(book->valuelist+entry*book->dim)[i];
    break;
  case 1:
    for(i=0,o=0;i<book->dim;i++,o+=step)
      a[o]*=(book->valuelist+entry*book->dim)[i];
    break;
  }
  return(entry);
}

#ifdef _V_SELFTEST

/* Simple enough; pack a few candidate codebooks, unpack them.  Code a
   number of vectors through (keeping track of the quantized values),
   and decode using the unpacked book.  quantized version of in should
   exactly equal out */

#include <stdio.h>

#include "vorbis/book/lsp20_0.vqh"
#include "vorbis/book/res0a_13.vqh"
#define TESTSIZE 40

double test1[TESTSIZE]={
  0.105939,
  0.215373,
  0.429117,
  0.587974,

  0.181173,
  0.296583,
  0.515707,
  0.715261,

  0.162327,
  0.263834,
  0.342876,
  0.406025,

  0.103571,
  0.223561,
  0.368513,
  0.540313,

  0.136672,
  0.395882,
  0.587183,
  0.652476,

  0.114338,
  0.417300,
  0.525486,
  0.698679,

  0.147492,
  0.324481,
  0.643089,
  0.757582,

  0.139556,
  0.215795,
  0.324559,
  0.399387,

  0.120236,
  0.267420,
  0.446940,
  0.608760,

  0.115587,
  0.287234,
  0.571081,
  0.708603,
};

double test3[TESTSIZE]={
  0,1,-2,3,4,-5,6,7,8,9,
  8,-2,7,-1,4,6,8,3,1,-9,
  10,11,12,13,14,15,26,17,18,19,
  30,-25,-30,-1,-5,-32,4,3,-2,0};

static_codebook *testlist[]={&_vq_book_lsp20_0,
			     &_vq_book_res0a_13,NULL};
double   *testvec[]={test1,test3};

int main(){
  oggpack_buffer write;
  oggpack_buffer read;
  long ptr=0,i;
  _oggpack_writeinit(&write);
  
  fprintf(stderr,"Testing codebook abstraction...:\n");

  while(testlist[ptr]){
    codebook c;
    static_codebook s;
    double *qv=alloca(sizeof(double)*TESTSIZE);
    double *iv=alloca(sizeof(double)*TESTSIZE);
    memcpy(qv,testvec[ptr],sizeof(double)*TESTSIZE);
    memset(iv,0,sizeof(double)*TESTSIZE);

    fprintf(stderr,"\tpacking/coding %ld... ",ptr);

    /* pack the codebook, write the testvector */
    _oggpack_reset(&write);
    vorbis_book_init_encode(&c,testlist[ptr]); /* get it into memory
                                                  we can write */
    vorbis_staticbook_pack(testlist[ptr],&write);
    fprintf(stderr,"Codebook size %ld bytes... ",_oggpack_bytes(&write));
    for(i=0;i<TESTSIZE;i+=c.dim){
      int best=_best(&c,qv+i,1);
      vorbis_book_encodev(&c,best,qv+i,&write);
    }
    vorbis_book_clear(&c);
    
    fprintf(stderr,"OK.\n");
    fprintf(stderr,"\tunpacking/decoding %ld... ",ptr);

    /* transfer the write data to a read buffer and unpack/read */
    _oggpack_readinit(&read,_oggpack_buffer(&write),_oggpack_bytes(&write));
    if(vorbis_staticbook_unpack(&read,&s)){
      fprintf(stderr,"Error unpacking codebook.\n");
      exit(1);
    }
    if(vorbis_book_init_decode(&c,&s)){
      fprintf(stderr,"Error initializing codebook.\n");
      exit(1);
    }

    for(i=0;i<TESTSIZE;i+=c.dim)
      if(vorbis_book_decodevs(&c,iv+i,&read,1,-1)==-1){
	fprintf(stderr,"Error reading codebook test data (EOP).\n");
	exit(1);
      }
    for(i=0;i<TESTSIZE;i++)
      if(fabs(qv[i]-iv[i])>.000001){
	fprintf(stderr,"read (%g) != written (%g) at position (%ld)\n",
		iv[i],qv[i],i);
	exit(1);
      }
	  
    fprintf(stderr,"OK\n");
    ptr++;
  }

  /* The above is the trivial stuff; now try unquantizing a log scale codebook */

  exit(0);
}

#endif
