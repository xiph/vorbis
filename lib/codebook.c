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
 last mod: $Id: codebook.c,v 1.12 2000/03/10 13:21:18 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/codebook.h"
#include "bitwise.h"
#include "bookinternal.h"
#include "misc.h"

/**** pack/unpack helpers ******************************************/
static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

/* code that packs the 24 bit float can be found in vq/bookutil.c */

static double _float24_unpack(long val){
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
static double *_book_unquantize(const static_codebook *b){
  long j,k;
  if(b->quantlist){
    double mindel=_float24_unpack(b->q_min);
    double delta=_float24_unpack(b->q_delta);
    double *r=malloc(sizeof(double)*b->entries*b->dim);
    
    for(j=0;j<b->entries;j++){
      double last=0.;
      for(k=0;k<b->dim;k++){
	double val=b->quantlist[j*b->dim+k]*delta+last+mindel;
	r[j*b->dim+k]=val;
	if(b->q_sequencep)last=val;
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
	  _oggpack_write(opb,i-count,ilog(c->entries-count));
	  count=i;
	}
      }
    }
    _oggpack_write(opb,i-count,ilog(c->entries-count));

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
    _oggpack_write(opb,c->q_min,24);
    _oggpack_write(opb,c->q_delta,24);
    _oggpack_write(opb,c->q_quant-1,4);
    _oggpack_write(opb,c->q_sequencep,1);

    /* quantized values */
    for(i=0;i<c->entries*c->dim;i++)
      _oggpack_write(opb,c->quantlist[i],c->q_quant);

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
	long num=_oggpack_read(opb,ilog(s->entries-i));
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
    s->q_min=_oggpack_read(opb,24);
    s->q_delta=_oggpack_read(opb,24);
    s->q_quant=_oggpack_read(opb,4)+1;
    s->q_sequencep=_oggpack_read(opb,1);

    /* quantized values */
    s->quantlist=malloc(sizeof(double)*s->entries*s->dim);
    for(i=0;i<s->entries*s->dim;i++)
      s->quantlist[i]=_oggpack_read(opb,s->q_quant);
    if(s->quantlist[i-1]==-1)goto _eofout;
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

static int  _best(codebook *book, double *a){
  encode_aux *t=book->c->encode_tree;
  int dim=book->dim;
  int ptr=0,k;
  /* optimized, using the decision tree */
  while(1){
    double c=0.;
    double *p=book->valuelist+t->p[ptr];
    double *q=book->valuelist+t->q[ptr];
    
    for(k=0;k<dim;k++)
      c+=(p[k]-q[k])*(a[k]-(p[k]+q[k])*.5);
    
    if(c>0.) /* in A */
      ptr= -t->ptr0[ptr];
    else     /* in B */
      ptr= -t->ptr1[ptr];
    if(ptr<=0)break;
  }
  return(-ptr);
}

/* returns the number of bits and *modifies a* to the quantization value *****/
int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b){
  int dim=book->dim;
  int best=_best(book,a);
  memcpy(a,book->valuelist+best*dim,dim*sizeof(double));
  return(vorbis_book_encode(book,best,b));}

/* returns the number of bits and *modifies a* to the quantization error *****/
int vorbis_book_encodevE(codebook *book, double *a, oggpack_buffer *b){
  int dim=book->dim,k;
  int best=_best(book,a);
  for(k=0;k<dim;k++)
    a[k]-=(book->valuelist+best*dim)[k];
  return(vorbis_book_encode(book,best,b));
}

/* returns the total squared quantization error for best match and sets each 
   element of a to local error ***************/
double vorbis_book_vE(codebook *book, double *a){
  int dim=book->dim,k;
  int best=_best(book,a);
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
long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b){
  long entry=vorbis_book_decode(book,b);
  int i;
  if(entry==-1)return(-1);
  for(i=0;i<book->dim;i++)a[i]+=(book->valuelist+entry*book->dim)[i];
  return(entry);
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
  exit(0);
}

#endif
