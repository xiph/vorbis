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
 last mod: $Id: codebook.c,v 1.1 2000/01/11 16:12:30 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include "vorbis/codec.h"
#include "vorbis/codebook.h"
#include "bitwise.h"

/**** pack/unpack helpers ******************************************/
static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static long _float24_pack(double val){
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
    long entry=marker[[i]];

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
      marker[j]=marker[j-1]<<1;
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

/* unpack the quantized list of values for encode/decode ***********/
static double *_book_unquantize(codebook *b){
  long j,k;
  double mindel=float24_unpack(b->q_min);
  double delta=float24_unpack(b->q_delta);
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
}

/**** Defend the abstraction ****************************************/

/* some elements in the codebook structure are assumed to be pointers
   to static/shared storage (the pointers are duped, and free below
   does not touch them.  The fields are unused by decode):

   quantlist,
   lengthlist,
   encode_tree

*/ 

codebook *vorbis_book_dup(const codebook *b){
  codebook *r=malloc(1,sizeof(codebook));
  memcpy(r,b,sizeof(codebook));

  /* handle non-flat storage */
  if(b->valuelist){
    r->valuelist=malloc(sizeof(double)*b->dim*b->entries);
    memcpy(r->valuelist,b->valuelist,sizeof(double)*b->dim*b->entries);
  }
  if(b->codelist){
    r->codelist=malloc(sizeof(long)*b->entries);
    memcpy(r->codelist,b->codelist,sizeof(long)**b->entries);
  }

  /* encode tree is assumed to be static storage; don't free it */

  if(b->decode_tree){
    long aux=b->decode_tree->aux;
    r->decode_tree=malloc(sizeof(decode_aux));
    r->decode_tree->aux=aux;
    r->decode_tree->ptr0=malloc(sizeof(long)*aux);
    r->decode_tree->ptr1=malloc(sizeof(long)*aux);

    memcpy(r->decode_tree->ptr0,b->decode_tree->ptr0,sizeof(long)*aux);
    memcpy(r->decode_tree->ptr1,b->decode_tree->ptr1,sizeof(long)*aux);
  }
  return(r);
}

void vorbis_book_free(codebook *b){
  if(b){
    if(b->decode_tree){
      free(b->decode_tree->ptr0);
      free(b->decode_tree->ptr1);
      memset(b->decode_tree,0,sizeof(decode_aux));
      free(b->decode_tree);
    }
    if(b->valuelist)free(b->valuelist);
    if(b->codelist)free(b->codelist);
    memset(b,0,sizeof(codebook));
    free(b);
  }
}

/* packs the given codebook into the bitstream
   side effects: populates the valuelist and codeword members ***********/
int vorbis_book_pack(codebook *c,oggpack_buffer *b){
  long i,j;
  int ordered=0;

  /* first the basic parameters */
  _oggpack_write(b,0x564342,24);
  _oggpack_write(b,c->dim,16);
  _oggpack_write(b,c->entries,24);

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
    _oggpack_write(b,1,1);  /* ordered */
    _oggpack_write(b,c->lengthlist[0]-1,5); /* 1 to 32 */

    for(i=1;i<c->entries;i++){
      long this=c->lengthlist[i];
      long last=c->lengthlist[i-1];
      if(this>last){
	for(j=last;j<this;j++){
	  _oggpack_write(b,i-count,ilog(c->entries-count));
	  count=i;
	}
      }
    }
    _oggpack_write(b,i-count,ilog(count));

  }else{
    /* length random.  Again, we don't code the codeword itself, just
       the length.  This time, though, we have to encode each length */
    _oggpack_write(b,0,1);   /* unordered */
    for(i=0;i<c->entries;i++)
      _oggpack_write(b,c->lengthlist[i]-1,5);
  }

  /* is the entry number the desired return value, or do we have a
     mapping? */
  if(codebook->quantlist){
    /* we have a mapping.  bundle it out. */
    _oggpack_write(b,1,1);

    /* values that define the dequantization */
    _oggpack_write(b,c->q_min,24);
    _oggpack_write(b,c->q_delta,24);
    _oggpack_write(b,c->q_quant-1,4);
    _oggpack_write(b,c->q_sequencep,1);

    /* quantized values */
    for(i=0;i<c->entries*c->dim;i++)
      _oggpack_write(b,c->quantlist[i],c->q_quant);

  }else{
    /* no mapping. */
    _oggpack_write(b,0,1);
  }

  c->codelist=_make_words(c->lengthlist,c->entries);
  c->valuelist=_book_unquantize(c);
  
  return(0);
}

/* unpacks a codebook from the packet buffer into the codebook struct,
   readies the codebook auxiliary structures for decode *************/
codebook *vorbis_book_unpack(oggpack_buffer *b){
  codebook *c=calloc(1,sizeof(codebook));

  /* make sure alignment is correct */
  if(_oggpack_read(b,24)!=0x564342)goto _eofout;

  /* first the basic parameters */
  c->dim=_oggpack_read(b,16);
  c->entries=_oggpack_read(b,24);
  if(c->entries==-1)goto _eofout;

  /* codeword ordering.... length ordered or unordered? */
  switch(_oggpack_read(b,1)){
  case 0:
    /* unordered */
    c->lengthlist=malloc(sizeof(long)*c->entries);
    for(i=0;i<c->entries;i++){
      long num=_oggpack_read(b,5);
      if(num==-1)goto _eofout;
      c->lengthlist[i]=num+1;
    }

    break;
  case 1:
    /* ordered */
    {
      long length=_oggpack_read(b,5)+1;
      c->lengthlist=malloc(sizeof(long)*c->entries);

      for(i=0;i<c->entries){
	long num=_oggpack_read(b,ilog(c->entries-i));
	if(num==-1)goto _eofout;
	for(j=0;j<num;j++,i++)
	  c->lengthlist[i]=length;
	length++;
      }
    }
    break;
  default:
    /* EOF */
    return(NULL);
  }
  
  /* now we generate the codewords for the given lengths */
  c->codelist=_make_words(c->lengthlist,c->entries);
  if(c->codelist==NULL)goto _errout;

  /* ...and the decode helper tree from the codewords */
  {
    long top=0;
    decode_aux *t=c->decode_tree=malloc(sizeof(decode_aux));
    long *ptr0=c->decode_tree->ptr0=calloc(c->entries*2,sizeof(long));
    long *ptr1=c->decode_tree->ptr1=calloc(c->entries*2,sizeof(long));
    c->decode_tree->aux=c->entries*2;

    for(i=0;i<c->entries;i++){
      long ptr=0;
      for(j=0;j<c->lengthlist[i]-1;j++){
	int bit=(c->codelist[i]>>j)&1;
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
      if(!((c->codelist[i]>>j)&1))
	ptr0[ptr]=-i;
      else
	ptr1[ptr]=-i;
    }
  }
  /* no longer needed */
  free(c->lengthlist);c->lengthlist=NULL;
  free(c->codelist);c->codelist=NULL;

  /* Do we have a mapping to unpack? */
  if(_oggpack_read(b,1)){

    /* values that define the dequantization */
    c->q_min=_oggpack_read(b,24);
    c->q_delta=_oggpack_read(b,24);
    c->q_quant=_oggpack_read(b,4)+1;
    c->q_sequencep=_oggpack_read(b,1);

    /* quantized values */
    c->quantlist=malloc(sizeof(double)*c->entries*c->dim);
    for(i=0;i<c->entries*c->dim;i++)
      c->quantlist[i]=_oggpack_read(b,c->q_quant);
    if(c->quantlist[i-1]==-1)goto _eofout;
    vorbis_book_unquantize(c);
    free(c->quantlist);c->quantlist=NULL;
  }

  /* all set */
  return(c);

 _errout:
 _eofout:
  if(c->lengthlist)free(c->lengthlist);c->lengthlist=NULL;
  if(c->quantlist)free(c->quantlist);c->quantlist=NULL;
  vorbis_book_free(c);
  return(NULL);
 
}

/* returns the number of bits ***************************************/
int vorbis_book_encode(codebook *book, int a, oggpack_buffer *b){
  _oggpack_write(b,book->codelist[a],book->lengthlist[a]);
  return(book->lengthlist[a]);
}

/* returns the number of bits and *modifies a* to the entry value *****/
int vorbis_book_encodev(codebook *book, double *a, oggpack_buffer *b){
  encode_aux *t=book->encode_tree;
  int dim=book->dim;
  int ptr=0,k;

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
  memcpy(a,book->valuelist-ptr*dim,dim*sizeof(double));
  return(vorbis_book_encode(book,-ptr,b));
}

/* returns the entry number or -1 on eof ****************************/
long vorbis_book_decode(codebook *book, oggpack_buffer *b){
  long ptr=0;
  decode_aux *t=book->decode_tree;
  do{
    switch(__oggpack_read1(b)){
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

/* returns the entry number or -1 on eof ****************************/
long vorbis_book_decodev(codebook *book, double *a, oggpack_buffer *b){
  long entry=vorbis_book_decode(book,b);
  if(entry==-1)return(-1);
  memcpy(a,b->valuelist+entry*b->dim,sizeof(double)*b->dim);
  return(entry);
}

