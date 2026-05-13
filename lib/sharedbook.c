/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2015             *
 * by the Xiph.Org Foundation https://xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: basic shared codebook operations

 ********************************************************************/

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include "os.h"
#include "misc.h"
#include "vorbis/codec.h"
#include "codebook.h"
#include "scales.h"

/**** pack/unpack helpers ******************************************/

int ov_ilog(ogg_uint32_t v){
  int ret;
  for(ret=0;v;ret++)v>>=1;
  return ret;
}

/* 32 bit float (not IEEE; nonnormalized mantissa +
   biased exponent) : neeeeeee eeemmmmm mmmmmmmm mmmmmmmm
   Why not IEEE?  It's just not that important here. */

#define VQ_FEXP 10
#define VQ_FMAN 21
#define VQ_FEXP_BIAS 768 /* bias toward values smaller than 1. */

/* doesn't currently guard under/overflow */
long _float32_pack(float val){
  int sign=0;
  long exp;
  long mant;
  if(val<0){
    sign=0x80000000;
    val= -val;
  }
  exp= floor(log(val)/log(2.f)+.001); /* +epsilon */
  mant=rint(ldexp(val,(VQ_FMAN-1)-exp));
  exp=(exp+VQ_FEXP_BIAS)<<VQ_FMAN;

  return(sign|exp|mant);
}

float _float32_unpack(long val){
  double mant=val&0x1fffff;
  int    sign=val&0x80000000;
  long   exp =(val&0x7fe00000L)>>VQ_FMAN;
  if(sign)mant= -mant;
  exp=exp-(VQ_FMAN-1)-VQ_FEXP_BIAS;
  /* clamp excessive exponent values */
  if (exp>63){
    exp=63;
  }
  if (exp<-63){
    exp=-63;
  }
  return(ldexp(mant,exp));
}

/* given a list of word lengths, generate a list of codewords.  Works
   for length ordered or unordered, always assigns the lowest valued
   codewords first.  Extended to handle unused entries (length 0) */
ogg_uint32_t *_make_words(char *l,long n,long sparsecount){
  long i,j,count=0;
  ogg_uint32_t marker[33];
  ogg_uint32_t *r=_ogg_malloc((sparsecount?sparsecount:n)*sizeof(*r));
  memset(marker,0,sizeof(marker));

  for(i=0;i<n;i++){
    long length=l[i];
    if(length>0){
      ogg_uint32_t entry=marker[length];

      /* when we claim a node for an entry, we also claim the nodes
         below it (pruning off the imagined tree that may have dangled
         from it) as well as blocking the use of any nodes directly
         above for leaves */

      /* update ourself */
      if(length<32 && (entry>>length)){
        /* error condition; the lengths must specify an overpopulated tree */
        _ogg_free(r);
        return(NULL);
      }
      r[count++]=entry;

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
    }else
      if(sparsecount==0)count++;
  }

  /* any underpopulated tree must be rejected. */
  /* Single-entry codebooks are a retconned extension to the spec.
     They have a single codeword '0' of length 1 that results in an
     underpopulated tree.  Shield that case from the underformed tree check. */
  if(!(count==1 && marker[2]==2)){
    for(i=1;i<33;i++)
      if(marker[i] & (0xffffffffUL>>(32-i))){
        _ogg_free(r);
        return(NULL);
      }
  }

  /* bitreverse the words because our bitwise packer/unpacker is LSb
     endian */
  for(i=0,count=0;i<n;i++){
    ogg_uint32_t temp=0;
    for(j=0;j<l[i];j++){
      temp<<=1;
      temp|=(r[count]>>j)&1;
    }

    if(sparsecount){
      if(l[i])
        r[count++]=temp;
    }else
      r[count++]=temp;
  }

  return(r);
}

/* given a list of word lengths, generate a list of packed MSb codewords and
   entry numbers.  This is like the above, except that the codewords are MSb
   first and aligned to be 32-bits in size, and the lower 24 bits include the
   entry number the codeword corresponds to. */
static ogg_int64_t *dec_make_words(signed char *l,long n,long sparsecount){
  long i,j,count=0;
  ogg_uint32_t marker[33];
  ogg_int64_t *r=_ogg_malloc((sparsecount?sparsecount:n)*sizeof(*r));
  if(r==NULL)return(NULL);
  memset(marker,0,sizeof(marker));

  for(i=0;i<n;i++){
    long length=l[i];
    if(length>0){
      ogg_uint32_t entry=marker[length];

      /* when we claim a node for an entry, we also claim the nodes
         below it (pruning off the imagined tree that may have dangled
         from it) as well as blocking the use of any nodes directly
         above for leaves */

      /* update ourself */
      if(length<32 && (entry>>length)){
        /* error condition; the lengths must specify an overpopulated tree */
        _ogg_free(r);
        return(NULL);
      }
      r[count++]=(ogg_int64_t)entry<<(32-l[i]+24)|i;

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
    }else
      if(sparsecount==0)count++;
  }

  /* any underpopulated tree must be rejected. */
  /* Single-entry codebooks are a retconned extension to the spec.
     They have a single codeword '0' of length 1 that results in an
     underpopulated tree.  Shield that case from the underformed tree check. */
  if(!(count==1 && marker[2]==2)){
    for(i=1;i<33;i++)
      if(marker[i] & (0xffffffffUL>>(32-i))){
        _ogg_free(r);
        return(NULL);
      }
  }

  return(r);
}

/* there might be a straightforward one-line way to do the below
   that's portable and totally safe against roundoff, but I haven't
   thought of it.  Therefore, we opt on the side of caution */
long _book_maptype1_quantvals(long dim, long entries){
  long vals;
  if(entries<1){
    return(0);
  }
  vals=floor(pow((float)entries,1.f/dim));

  /* the above *should* be reliable, but we'll not assume that FP is
     ever reliable when bitstream sync is at stake; verify via integer
     means that vals really is the greatest value of dim for which
     vals^dim <= entries */
  /* treat the above as an initial guess */
  if(vals<1){
    vals=1;
  }
  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<dim;i++){
      if(entries/vals<acc)break;
      acc*=vals;
      if(LONG_MAX/(vals+1)<acc1)acc1=LONG_MAX;
      else acc1*=vals+1;
    }
    if(i>=dim && acc<=entries && acc1>entries){
      return(vals);
    }else{
      if(i<dim || acc>entries){
        vals--;
      }else{
        vals++;
      }
    }
  }
}

/* unpack the quantized list of values for decode ***********/
/* we need to deal with two map types: in map type 1, the values are
   generated algorithmically (each column of the vector counts through
   the values in the quant vector). in map type 2, all the values came
   in in an explicit list.  Both value lists must be unpacked */
void _book_unquantize(float *r,const dec_codebook *b,ogg_int32_t n,
                      ogg_int64_t *sparsemap){
  ogg_int32_t i,j,k;
  if(b->maptype==1 || b->maptype==2){
    ogg_int32_t quantvals;
    float mindel=_float32_unpack(b->q_min);
    float delta=_float32_unpack(b->q_delta);

    /* maptype 1 and 2 both use a quantized value vector, but
       different sizes */
    switch(b->maptype){
    case 1:
      /* most of the time, entries%dimensions == 0, but we need to be
         well defined.  We define that the possible vales at each
         scalar is values == entries/dim.  If entries%dim != 0, we'll
         have 'too few' values (values*dim<entries), which means that
         we'll have 'left over' entries; left over entries use zeroed
         values (and are wasted).  So don't generate codebooks like
         that */
      quantvals=(ogg_int32_t)_book_maptype1_quantvals(b->dim, b->entries);
      for(i=0;i<n;i++){
        float last=0.f;
        int indexdiv=1;
        j=(long)(sparsemap?sparsemap[i]&0xffffff:i);
        for(k=0;k<b->dim;k++){
          ogg_int32_t index=(j/indexdiv)%quantvals;
          float val=b->quantlist[index];
          val=fabs(val)*delta+mindel+last;
          if(b->q_sequencep)last=val;
          r[i*b->dim+k]=val;
          indexdiv*=quantvals;
        }
      }
      break;
    case 2:
      for(i=0;i<n;i++){
        float last=0.f;
        j=(ogg_int32_t)(sparsemap?sparsemap[i]&0xffffff:i);
        for(k=0;k<b->dim;k++){
          float val=b->quantlist[j*b->dim+k];
          val=fabs(val)*delta+mindel+last;
          if(b->q_sequencep)last=val;
          r[i*b->dim+k]=val;
        }
      }
      break;
    }

  }
}

void vorbis_staticbook_destroy(static_codebook *b){
  if(b->allocedp){
    if(b->quantlist)_ogg_free(b->quantlist);
    if(b->lengthlist)_ogg_free(b->lengthlist);
    memset(b,0,sizeof(*b));
    _ogg_free(b);
  } /* otherwise, it is in static memory */
}

void vorbis_book_clear(codebook *b){
  /* static book is not cleared; we're likely called on the lookup and
     the static codebook belongs to the info struct */
  if(b->valuelist)_ogg_free(b->valuelist);
  if(b->codelist)_ogg_free(b->codelist);

  memset(b,0,sizeof(*b));
}

void vorbis_decbook_clear(dec_codebook *b){
  if(b->quantlist)_ogg_free(b->quantlist);
  if(b->firsttable)_ogg_free(b->firsttable);
  if(b->codelist)_ogg_free(b->codelist);
  if(b->codelengths)_ogg_free(b->codelengths);
  if(b->index)_ogg_free(b->index);
  if(b->valuelist)_ogg_free(b->valuelist);
  memset(b,0,sizeof(*b));
}

int vorbis_book_init_encode(codebook *c,const static_codebook *s){

  memset(c,0,sizeof(*c));
  c->c=s;
  c->entries=s->entries;
  c->used_entries=s->entries;
  c->dim=s->dim;
  c->codelist=_make_words(s->lengthlist,s->entries,0);
  c->quantvals=_book_maptype1_quantvals(s->dim, s->entries);
  c->minval=(int)rint(_float32_unpack(s->q_min));
  c->delta=(int)rint(_float32_unpack(s->q_delta));

  return(0);
}

static ogg_uint32_t bitreverse(ogg_uint32_t x){
  x=    ((x>>16)&0x0000ffffUL) | ((x<<16)&0xffff0000UL);
  x=    ((x>> 8)&0x00ff00ffUL) | ((x<< 8)&0xff00ff00UL);
  x=    ((x>> 4)&0x0f0f0f0fUL) | ((x<< 4)&0xf0f0f0f0UL);
  x=    ((x>> 2)&0x33333333UL) | ((x<< 2)&0xccccccccUL);
  return((x>> 1)&0x55555555UL) | ((x<< 1)&0xaaaaaaaaUL);
}

static int sort64a(const void *a,const void *b){
  return ( *(ogg_int64_t *)a>*(ogg_int64_t *)b)-
    ( *(ogg_int64_t *)a<*(ogg_int64_t *)b);
}

/* decode codebook arrangement is more heavily optimized than encode */
int vorbis_book_init_decode(dec_codebook *c){
  ogg_int32_t i,j,n=0;
  int tabn;
  ogg_int64_t *codes=NULL;

  /* only initialize once */
  if(c->codelist)return(0);

  /* count actually used entries and find max length */
  for(i=0;i<c->entries;i++)
    if(c->codelengths[i]>0)
      n++;

  c->used_entries=n;

  if(n>0){
    signed char *codelengths;
    /* two different remappings go on here.

    First, we collapse the likely sparse codebook down only to
    actually represented values/words.  This collapsing needs to be
    indexed as map-valueless books are used to encode original entry
    positions as integers.

    Second, we reorder all vectors, including the entry index above,
    by sorted bitreversed codeword to allow treeless decode. */

    /* perform sort */
    codes=dec_make_words(c->codelengths,c->entries,c->used_entries);
    if(codes==NULL)goto err_out;

    qsort(codes,n,sizeof(*codes),sort64a);

    c->codelist=_ogg_malloc(n*sizeof(*c->codelist));
    if(c->codelist==NULL)goto err_out;

    for(i=0;i<n;i++){
      c->codelist[i]=(ogg_uint32_t)(codes[i]>>24);
    }

    if(c->maptype==1 || c->maptype==2){
      c->valuelist=_ogg_malloc(c->dim*n*sizeof(*c->valuelist));
      if(c->valuelist==NULL)goto err_out;
      _book_unquantize(c->valuelist,c,n,codes);
      _ogg_free(c->quantlist);
      c->quantlist=NULL;
    }
    c->index=_ogg_malloc(n*sizeof(*c->index));
    if(c->index==NULL)goto err_out;
    codelengths=_ogg_malloc(n*sizeof(*c->codelengths));
    if(codelengths==NULL)goto err_out;

    c->maxlength=0;
    for(i=0;i<n;i++){
      j=(ogg_int32_t)(codes[i]&0xffffff);
      c->index[i]=j;
      codelengths[i]=c->codelengths[j];
      if(codelengths[i]>c->maxlength)
        c->maxlength=codelengths[i];
    }
    _ogg_free(codes);
    codes=NULL;
    _ogg_free(c->codelengths);
    c->codelengths=codelengths;

    if(n==1 && c->maxlength==1){
      /* special case the 'single entry codebook' with a single bit
       fastpath table (that always returns entry 0 )in order to use
       unmodified decode paths. */
      c->firsttablen=1;
      c->firsttable=_ogg_calloc(2,sizeof(*c->firsttable));
      if(c->firsttable==NULL)goto err_out;
      c->firsttable[0]=c->firsttable[1]=1;

    }else{
      c->firsttablen=(signed char)(ov_ilog(n)-4); /* this is magic */
      if(c->firsttablen<5)c->firsttablen=5;
      if(c->firsttablen>8)c->firsttablen=8;

      tabn=1<<c->firsttablen;
      c->firsttable=_ogg_calloc(tabn,sizeof(*c->firsttable));
      if(c->firsttable==NULL)goto err_out;

      for(i=0;i<n;i++){
        if(c->codelengths[i]<=c->firsttablen){
          ogg_uint32_t orig=bitreverse(c->codelist[i]);
          for(j=0;j<(1<<(c->firsttablen-c->codelengths[i]));j++)
            /* pack the length into the table, too, to avoid an extra lookup.
               This also guarantees the table value is non-zero, since the
               length of any used entry is positive. */
            c->firsttable[orig|(j<<c->codelengths[i])]=
             i<<6|c->codelengths[i];
        }
      }

      /* now fill in 'unused' entries in the firsttable with hi/lo search
         hints for the non-direct-hits */
      {
        ogg_uint32_t mask=0xfffffffeUL<<(31-c->firsttablen);
        long lo=0,hi=0;

        for(i=0;i<tabn;i++){
          ogg_uint32_t word=((ogg_uint32_t)i<<(32-c->firsttablen));
          if(c->firsttable[bitreverse(word)]==0){
            while((lo+1)<n && c->codelist[lo+1]<=word)lo++;
            while(    hi<n && word>=(c->codelist[hi]&mask))hi++;

            /* we only actually have 15 bits per hint to play with here.
               In order to overflow gracefully (nothing breaks, efficiency
               just drops), encode as the difference from the extremes. */
            {
              unsigned long loval=lo;
              unsigned long hival=n-hi;

              if(loval>0x7fff)loval=0x7fff;
              if(hival>0x7fff)hival=0x7fff;
              c->firsttable[bitreverse(word)]=
                0x80000000UL | (loval<<15) | hival;
            }
          }
        }
      }
    }
  }

  return(0);
 err_out:
  if(codes)_ogg_free(codes);
  /* our caller will clean up the partially initialized book */
  return(-1);
}

long vorbis_book_codeword(codebook *book,int entry){
  if(book->c) /* only use with encode; decode optimizations are
                 allowed to break this */
    return book->codelist[entry];
  return -1;
}

long vorbis_book_codelen(codebook *book,int entry){
  if(book->c) /* only use with encode; decode optimizations are
                 allowed to break this */
    return book->c->lengthlist[entry];
  return -1;
}

#ifdef _V_SELFTEST

/* Unit tests of the dequantizer; this stuff will be OK
   cross-platform, I simply want to be sure that special mapping cases
   actually work properly; a bug could go unnoticed for a while */

#include <stdio.h>

/* cases:

   no mapping
   full, explicit mapping
   algorithmic mapping

   nonsequential
   sequential
*/

static ogg_uint16_t full_quantlist1[]={0,1,2,3,    4,5,6,7, 8,3,6,1};
static ogg_uint16_t partial_quantlist1[]={0,7,2};

/* no mapping */
dec_codebook test1={
  4,0,0,16,0,
  0,0,0,0,0,
  NULL,
  NULL,NULL,NULL,NULL,NULL
};
static float *test1_result=NULL;

/* linear, full mapping, nonsequential */
dec_codebook test2={
  4,0,0,3,0,
  2,4,0,3761766400U,1611661312,
  full_quantlist1,
  NULL,NULL,NULL,NULL,NULL
};
static float test2_result[]={-3,-2,-1,0, 1,2,3,4, 5,0,3,-2};

/* linear, full mapping, sequential */
dec_codebook test3={
  4,0,0,3,0,
  2,4,1,3761766400U,1611661312,
  full_quantlist1,
  NULL,NULL,NULL,NULL,NULL
};
static float test3_result[]={-3,-5,-6,-6, 1,3,6,10, 5,5,8,6};

/* linear, algorithmic mapping, nonsequential */
dec_codebook test4={
  3,0,0,27,0,
  1,4,0,3761766400U,1611661312,
  partial_quantlist1,
  NULL,NULL,NULL,NULL,NULL
};
static float test4_result[]={-3,-3,-3, 4,-3,-3, -1,-3,-3,
                              -3, 4,-3, 4, 4,-3, -1, 4,-3,
                              -3,-1,-3, 4,-1,-3, -1,-1,-3,
                              -3,-3, 4, 4,-3, 4, -1,-3, 4,
                              -3, 4, 4, 4, 4, 4, -1, 4, 4,
                              -3,-1, 4, 4,-1, 4, -1,-1, 4,
                              -3,-3,-1, 4,-3,-1, -1,-3,-1,
                              -3, 4,-1, 4, 4,-1, -1, 4,-1,
                              -3,-1,-1, 4,-1,-1, -1,-1,-1};

/* linear, algorithmic mapping, sequential */
dec_codebook test5={
  3,0,0,27,0,
  1,4,1,3761766400U,1611661312,
  partial_quantlist1,
  NULL,NULL,NULL,NULL,NULL,
};
static float test5_result[]={-3,-6,-9, 4, 1,-2, -1,-4,-7,
                              -3, 1,-2, 4, 8, 5, -1, 3, 0,
                              -3,-4,-7, 4, 3, 0, -1,-2,-5,
                              -3,-6,-2, 4, 1, 5, -1,-4, 0,
                              -3, 1, 5, 4, 8,12, -1, 3, 7,
                              -3,-4, 0, 4, 3, 7, -1,-2, 2,
                              -3,-6,-7, 4, 1, 0, -1,-4,-5,
                              -3, 1, 0, 4, 8, 7, -1, 3, 2,
                              -3,-4,-5, 4, 3, 2, -1,-2,-3};

void run_test(dec_codebook *b,float *comp){
  float out[3*27];
  int i;

  if(comp){
    _book_unquantize(out,b,b->entries,NULL);
    for(i=0;i<b->entries*b->dim;i++)
      if(fabs(out[i]-comp[i])>.0001){
        fprintf(stderr,"disagreement in unquantized and reference data:\n"
                "position %d, %g != %g\n",i,out[i],comp[i]);
        exit(1);
      }

  }
}

int main(){
  /* run the nine dequant tests, and compare to the hand-rolled results */
  fprintf(stderr,"Dequant test 1... ");
  run_test(&test1,test1_result);
  fprintf(stderr,"OK\nDequant test 2... ");
  run_test(&test2,test2_result);
  fprintf(stderr,"OK\nDequant test 3... ");
  run_test(&test3,test3_result);
  fprintf(stderr,"OK\nDequant test 4... ");
  run_test(&test4,test4_result);
  fprintf(stderr,"OK\nDequant test 5... ");
  run_test(&test5,test5_result);
  fprintf(stderr,"OK\n\n");

  return(0);
}

#endif
