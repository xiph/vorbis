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
 last mod: $Id: codebook.c,v 1.11.4.4 2000/04/06 15:59:36 xiphmont Exp $

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
    for(i=0;i<c->entries;i++)
      _oggpack_write(opb,c->lengthlist[i]-1,5);
  }

  /* is the entry number the desired return value, or do we have a
     mapping? */
  if(c->quantlist){
    /* we have a mapping.  bundle it out. */
    _oggpack_write(opb,1,1);

    /* values that define the dequantization */
    _oggpack_write(opb,c->q_min,32);
    _oggpack_write(opb,c->q_delta,32);
    _oggpack_write(opb,c->q_quant-1,4);
    _oggpack_write(opb,c->q_sequencep,1);
    _oggpack_write(opb,c->q_log,1);
    if(c->q_log){
      _oggpack_write(opb,c->q_zeroflag,1);
      _oggpack_write(opb,c->q_negflag,1);
    }

    /* quantized values */
    for(i=0;i<c->entries*c->dim;i++){
      if(c->quantlist[i]==0){
	if(c->q_zeroflag&&c->q_log)
	  _oggpack_write(opb,0,1);
	else{
	  /* serious failure; this is an invalid codebook */
	  return(1);
	}
      }else{
	if(c->q_zeroflag&&c->q_log)
	  _oggpack_write(opb,1,1);

	_oggpack_write(opb,labs(c->quantlist[i])-1,c->q_quant);
	if(c->q_negflag&&c->q_log){
	  if(c->quantlist[i]>0){
	    _oggpack_write(opb,0,1);
	  }else{
	    _oggpack_write(opb,1,1);
	  }
	}
      }
    }
  }else{
    /* no mapping. */
    _oggpack_write(opb,0,1);
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
    for(i=0;i<s->entries;i++){
      long num=_oggpack_read(opb,5);
      if(num==-1)goto _eofout;
      s->lengthlist[i]=num+1;
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
  if(_oggpack_read(opb,1)){

    /* values that define the dequantization */
    s->q_min=_oggpack_read(opb,32);
    s->q_delta=_oggpack_read(opb,32);
    s->q_quant=_oggpack_read(opb,4)+1;
    s->q_sequencep=_oggpack_read(opb,1);
    s->q_log=_oggpack_read(opb,1);
    if(s->q_log){
      s->q_zeroflag=_oggpack_read(opb,1);
      s->q_negflag=_oggpack_read(opb,1);
    }else{
      /* zero/negflag must be zero if log is */
      s->q_zeroflag=0;
      s->q_negflag=0;
    }

    /* quantized values */
    s->quantlist=malloc(sizeof(double)*s->entries*s->dim);
    for(i=0;i<s->entries*s->dim;i++){
      if(s->q_zeroflag)
	if(_oggpack_read(opb,1)==0){
	  s->quantlist[i]=0;
	  continue;
	}

      s->quantlist[i]=_oggpack_read(opb,s->q_quant);
      if(s->quantlist[i]==-1)goto _eofout;
      s->quantlist[i]++;

      /* if we're log scale, a negative dB value is a positive linear
         value (just < 1.)  We need an additional bit to set
         positive/negative linear side. */
      if(s->q_negflag)
	if(_oggpack_read(opb,1))
	  s->quantlist[i]= -s->quantlist[i];
    }
  }

  /* all set */
  return(0);

 _errout:
 _eofout:
  vorbis_staticbook_clear(s);
  return(-1); 
}

/* returns the number of bits ***************************************/
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  _oggpack_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

/* returns the number of bits and *modifies a* to the quantization value *****/
int vorbis_book_encodevs(codebook *book,double *a,oggpack_buffer *b,int step){
  int dim=book->dim,k,o;
  int best=(book->c->q_log?_logbest(book,a,step):_best(book,a,step));
  for(k=0,o=0;k<dim;k++,o+=step)
    a[o]=(book->valuelist+best*dim)[k];
  return(vorbis_book_encode(book,best,b));
}

int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b){
  return vorbis_book_encodevs(book,a,b,1);
}

/* returns the number of bits and *modifies a* to the quantization error *****/
int vorbis_book_encodevEs(codebook *book,double *a,oggpack_buffer *b,int step){
  int dim=book->dim,k,o;
  int best=(book->c->q_log?_logbest(book,a,step):_best(book,a,step));
  for(k=0,o=0;k<dim;k++,o+=step)
    a[o]-=(book->valuelist+best*dim)[k];
  return(vorbis_book_encode(book,best,b));
}

int vorbis_book_encodevE(codebook *book,double *a,oggpack_buffer *b){
  return vorbis_book_encodevEs(book,a,b,1);
}

/* returns the total squared quantization error for best match and sets each 
   element of a to local error ***************/
double vorbis_book_vE(codebook *book, double *a){
  int dim=book->dim,k;
  int best=(book->c->q_log?_logbest(book,a,1):_best(book,a,1));
  double acc=0.;
  for(k=0;k<dim;k++){
    double val=(book->valuelist+best*dim)[k];
    a[k]-=val;
    acc+=a[k]*a[k];
  }
  return(acc);
}

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
long vorbis_book_decodevs(codebook *book,double *a,oggpack_buffer *b,int step){
  long entry=vorbis_book_decode(book,b);
  int i,o;
  if(entry==-1)return(-1);
  for(i=0,o=0;i<book->dim;i++,o+=step)
    a[o]+=(book->valuelist+entry*book->dim)[i];
  return(entry);
}

long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b){
  return vorbis_book_decodevs(book,a,b,1);
}


#ifdef _V_SELFTEST

/* Simple enough; pack a few candidate codebooks, unpack them.  Code a
   number of vectors through (keeping track of the quantized values),
   and decode using the unpacked book.  quantized version of in should
   exactly equal out */

#include <stdio.h>
#include "vorbis/book/lsp20_0.vqh"
#include "vorbis/book/lsp32_0.vqh"
#define TESTSIZE 40
#define TESTDIM 4

double test1[40]={
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

double test2[40]={
  0.088654,
  0.165742,
  0.279013,
  0.395894,

  0.110812,
  0.218422,
  0.283423,
  0.371719,

  0.136985,
  0.186066,
  0.309814,
  0.381521,

  0.123925,
  0.211707,
  0.314771,
  0.433026,

  0.088619,
  0.192276,
  0.277568,
  0.343509,

  0.068400,
  0.132901,
  0.223999,
  0.302538,

  0.202159,
  0.306131,
  0.360362,
  0.416066,

  0.072591,
  0.178019,
  0.304315,
  0.376516,

  0.094336,
  0.188401,
  0.325119,
  0.390264,

  0.091636,
  0.223099,
  0.282899,
  0.375124,
};

static_codebook *testlist[]={&_vq_book_lsp20_0,&_vq_book_lsp32_0,NULL};
double   *testvec[]={test1,test2};

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
    for(i=0;i<TESTSIZE;i+=TESTDIM)
      vorbis_book_encodev(&c,qv+i,&write);
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

    for(i=0;i<TESTSIZE;i+=TESTDIM)
      if(vorbis_book_decodev(&c,iv+i,&read)==-1){
	fprintf(stderr,"Error reading codebook test data (EOP).\n");
	exit(1);
      }
    for(i=0;i<TESTSIZE;i++)
      if(fabs(qv[i]-iv[i])>.000001){
	fprintf(stderr,"input (%g) != output (%g) at position (%ld)\n",
		iv[i],testvec[ptr][i]-qv[i],i);
	exit(1);
      }
	  
    fprintf(stderr,"OK\n");
    ptr++;
  }

  /* The above is the trivial stuff; now try unquantizing a log scale codebook */

  exit(0);
}

#endif
