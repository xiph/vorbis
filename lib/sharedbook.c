/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: basic shared codebook operations
 last mod: $Id: sharedbook.c,v 1.12 2000/11/14 00:05:31 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ogg/ogg.h>
#include "os.h"
#include "vorbis/codec.h"
#include "codebook.h"
#include "scales.h"

/**** pack/unpack helpers ******************************************/
int _ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
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
  exp= floor(log(val)/log(2));
  mant=rint(ldexp(val,(VQ_FMAN-1)-exp));
  exp=(exp+VQ_FEXP_BIAS)<<VQ_FMAN;

  return(sign|exp|mant);
}

float _float32_unpack(long val){
  float mant=val&0x1fffff;
  float sign=val&0x80000000;
  float exp =(val&0x7fe00000)>>VQ_FMAN;
  if(sign)mant= -mant;
  return(ldexp(mant,exp-(VQ_FMAN-1)-VQ_FEXP_BIAS));
}

/* given a list of word lengths, generate a list of codewords.  Works
   for length ordered or unordered, always assigns the lowest valued
   codewords first.  Extended to handle unused entries (length 0) */
long *_make_words(long *l,long n){
  long i,j;
  long marker[33];
  long *r=_ogg_malloc(n*sizeof(long));
  memset(marker,0,sizeof(marker));

  for(i=0;i<n;i++){
    long length=l[i];
    if(length>0){
      long entry=marker[length];
      
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
  long top=0,i,j,n;
  decode_aux *t=_ogg_malloc(sizeof(decode_aux));
  long *ptr0=t->ptr0=_ogg_calloc(c->entries*2,sizeof(long));
  long *ptr1=t->ptr1=_ogg_calloc(c->entries*2,sizeof(long));
  long *codelist=_make_words(s->lengthlist,s->entries);

  if(codelist==NULL)return(NULL);
  t->aux=c->entries*2;

  for(i=0;i<c->entries;i++){
    if(s->lengthlist[i]>0){
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
  }
  _ogg_free(codelist);

  t->tabn = _ilog(c->entries)-4; /* this is magic */
  if(t->tabn<5)t->tabn=5;
  n = 1<<t->tabn;
  t->tab = _ogg_malloc(n*sizeof(long));
  t->tabl = _ogg_malloc(n*sizeof(int));
  for (i = 0; i < n; i++) {
    long p = 0;
    for (j = 0; j < t->tabn && (p > 0 || j == 0); j++) {
      if (i & (1 << j))
	p = ptr1[p];
      else
	p = ptr0[p];
    }
    /* now j == length, and p == -code */
    t->tab[i] = p;
    t->tabl[i] = j;
  }

  return(t);
}

/* there might be a straightforward one-line way to do the below
   that's portable and totally safe against roundoff, but I haven't
   thought of it.  Therefore, we opt on the side of caution */
long _book_maptype1_quantvals(const static_codebook *b){
  long vals=floor(pow(b->entries,1./b->dim));

  /* the above *should* be reliable, but we'll not assume that FP is
     ever reliable when bitstream sync is at stake; verify via integer
     means that vals really is the greatest value of dim for which
     vals^b->bim <= b->entries */
  /* treat the above as an initial guess */
  while(1){
    long acc=1;
    long acc1=1;
    int i;
    for(i=0;i<b->dim;i++){
      acc*=vals;
      acc1*=vals+1;
    }
    if(acc<=b->entries && acc1>b->entries){
      return(vals);
    }else{
      if(acc>b->entries){
	vals--;
      }else{
	vals++;
      }
    }
  }
}

/* unpack the quantized list of values for encode/decode ***********/
/* we need to deal with two map types: in map type 1, the values are
   generated algorithmically (each column of the vector counts through
   the values in the quant vector). in map type 2, all the values came
   in in an explicit list.  Both value lists must be unpacked */
float *_book_unquantize(const static_codebook *b){
  long j,k;
  if(b->maptype==1 || b->maptype==2){
    int quantvals;
    float mindel=_float32_unpack(b->q_min);
    float delta=_float32_unpack(b->q_delta);
    float *r=_ogg_calloc(b->entries*b->dim,sizeof(float));

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
      quantvals=_book_maptype1_quantvals(b);
      for(j=0;j<b->entries;j++){
	float last=0.;
	int indexdiv=1;
	for(k=0;k<b->dim;k++){
	  int index= (j/indexdiv)%quantvals;
	  float val=b->quantlist[index];
	  val=fabs(val)*delta+mindel+last;
	  if(b->q_sequencep)last=val;	  
	  r[j*b->dim+k]=val;
	  indexdiv*=quantvals;
	}
      }
      break;
    case 2:
      for(j=0;j<b->entries;j++){
	float last=0.;
	for(k=0;k<b->dim;k++){
	  float val=b->quantlist[j*b->dim+k];
	  val=fabs(val)*delta+mindel+last;
	  if(b->q_sequencep)last=val;	  
	  r[j*b->dim+k]=val;
	}
      }
      break;
    }

    return(r);
  }
  return(NULL);
}

void vorbis_staticbook_clear(static_codebook *b){
  if(b->allocedp){
    if(b->quantlist)_ogg_free(b->quantlist);
    if(b->lengthlist)_ogg_free(b->lengthlist);
    if(b->nearest_tree){
      _ogg_free(b->nearest_tree->ptr0);
      _ogg_free(b->nearest_tree->ptr1);
      _ogg_free(b->nearest_tree->p);
      _ogg_free(b->nearest_tree->q);
      memset(b->nearest_tree,0,sizeof(encode_aux_nearestmatch));
      _ogg_free(b->nearest_tree);
    }
    if(b->thresh_tree){
      _ogg_free(b->thresh_tree->quantthresh);
      _ogg_free(b->thresh_tree->quantmap);
      memset(b->thresh_tree,0,sizeof(encode_aux_threshmatch));
      _ogg_free(b->thresh_tree);
    }

    memset(b,0,sizeof(static_codebook));
  }
}

void vorbis_staticbook_destroy(static_codebook *b){
  if(b->allocedp){
    vorbis_staticbook_clear(b);
    _ogg_free(b);
  }
}

void vorbis_book_clear(codebook *b){
  /* static book is not cleared; we're likely called on the lookup and
     the static codebook belongs to the info struct */
  if(b->decode_tree){
    _ogg_free(b->decode_tree->tab);
    _ogg_free(b->decode_tree->tabl);

    _ogg_free(b->decode_tree->ptr0);
    _ogg_free(b->decode_tree->ptr1);
    memset(b->decode_tree,0,sizeof(decode_aux));
    _ogg_free(b->decode_tree);
  }
  if(b->valuelist)_ogg_free(b->valuelist);
  if(b->codelist)_ogg_free(b->codelist);
  memset(b,0,sizeof(codebook));
}

int vorbis_book_init_encode(codebook *c,const static_codebook *s){
  long j,k;
  memset(c,0,sizeof(codebook));
  c->c=s;
  c->entries=s->entries;
  c->dim=s->dim;
  c->codelist=_make_words(s->lengthlist,s->entries);
  c->valuelist=_book_unquantize(s);

  /* set the 'zero entry' */
  c->zeroentry=-1;
  if(c->valuelist){
    for(j=0;j<s->entries;j++){
      int flag=1;
      for(k=0;k<s->dim;k++){
	if(fabs(c->valuelist[j*s->dim+k])>1e-12){
	  flag=0;
	  break;
	}
      }
      if(flag)
	c->zeroentry=j;
    }
  }

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

static float _dist(int el,float *ref, float *b,int step){
  int i;
  float acc=0.;
  for(i=0;i<el;i++){
    float val=(ref[i]-b[i*step]);
    acc+=val*val;
  }
  return(acc);
}

#include <stdio.h>
int _best(codebook *book, float *a, int step){
  encode_aux_nearestmatch *nt=book->c->nearest_tree;
  encode_aux_threshmatch *tt=book->c->thresh_tree;
  encode_aux_pigeonhole *pt=book->c->pigeon_tree;
  int dim=book->dim;
  int ptr=0,k,o;
  /*int savebest=-1;
    float saverr;*/

  /* do we have a threshhold encode hint? */
  if(tt){
    int index=0;
    /* find the quant val of each scalar */
    for(k=0,o=step*(dim-1);k<dim;k++,o-=step){
      int i;
      /* linear search the quant list for now; it's small and although
	 with > 8 entries, it would be faster to bisect, this would be
	 a misplaced optimization for now */
      for(i=0;i<tt->threshvals-1;i++)
	if(a[o]<tt->quantthresh[i])break;

      index=(index*tt->quantvals)+tt->quantmap[i];
    }
    /* regular lattices are easy :-) */
    if(book->c->lengthlist[index]>0) /* is this unused?  If so, we'll
					use a decision tree after all
					and fall through*/
      return(index);
  }

  /* do we have a pigeonhole encode hint? */
  if(pt){
    const static_codebook *c=book->c;
    int i,besti=-1;
    float best;
    int entry=0;

    /* dealing with sequentialness is a pain in the ass */
    if(c->q_sequencep){
      int pv;
      long mul=1;
      float qlast=0;
      for(k=0,o=0;k<dim;k++,o+=step){
	pv=(int)((a[o]-qlast-pt->min)/pt->del);
	if(pv<0 || pv>=pt->mapentries)break;
	entry+=pt->pigeonmap[pv]*mul;
	mul*=pt->quantvals;
	qlast+=pv*pt->del+pt->min;
      }
    }else{
      for(k=0,o=step*(dim-1);k<dim;k++,o-=step){
	int pv=(int)((a[o]-pt->min)/pt->del);
	if(pv<0 || pv>=pt->mapentries)break;
	entry=entry*pt->quantvals+pt->pigeonmap[pv];
      }
    }

    /* must be within the pigeonholable range; if we quant outside (or
       in an entry that we define no list for), brute force it */
    if(k==dim && pt->fitlength[entry]){
      /* search the abbreviated list */
      long *list=pt->fitlist+pt->fitmap[entry];
      for(i=0;i<pt->fitlength[entry];i++){
	float this=_dist(dim,book->valuelist+list[i]*dim,a,step);
	if(besti==-1 || this<best){
	  best=this;
	  besti=list[i];
	}
      }

      return(besti); 
    }
  }

  if(nt){
    /* optimized using the decision tree */
    while(1){
      float c=0.;
      float *p=book->valuelist+nt->p[ptr];
      float *q=book->valuelist+nt->q[ptr];
      
      for(k=0,o=0;k<dim;k++,o+=step)
	c+=(p[k]-q[k])*(a[o]-(p[k]+q[k])*.5);
      
      if(c>0.) /* in A */
	ptr= -nt->ptr0[ptr];
      else     /* in B */
	ptr= -nt->ptr1[ptr];
      if(ptr<=0)break;
    }
    return(-ptr);
  }

  /* brute force it! */
  {
    const static_codebook *c=book->c;
    int i,besti=-1;
    float best;
    float *e=book->valuelist;
    for(i=0;i<book->entries;i++){
      if(c->lengthlist[i]>0){
	float this=_dist(dim,e,a,step);
	if(besti==-1 || this<best){
	  best=this;
	  besti=i;
	}
      }
      e+=dim;
    }

    /*if(savebest!=-1 && savebest!=besti){
      fprintf(stderr,"brute force/pigeonhole disagreement:\n"
	      "original:");
      for(i=0;i<dim*step;i+=step)fprintf(stderr,"%g,",a[i]);
      fprintf(stderr,"\n"
	      "pigeonhole (entry %d, err %g):",savebest,saverr);
      for(i=0;i<dim;i++)fprintf(stderr,"%g,",
				(book->valuelist+savebest*dim)[i]);
      fprintf(stderr,"\n"
	      "bruteforce (entry %d, err %g):",besti,best);
      for(i=0;i<dim;i++)fprintf(stderr,"%g,",
				(book->valuelist+besti*dim)[i]);
      fprintf(stderr,"\n");
      }*/
    return(besti);
  }
}

/* returns the entry number and *modifies a* to the remainder value ********/
int vorbis_book_besterror(codebook *book,float *a,int step,int addmul){
  int dim=book->dim,i,o;
  int best=_best(book,a,step);
  switch(addmul){
  case 0:
    for(i=0,o=0;i<dim;i++,o+=step)
      a[o]-=(book->valuelist+best*dim)[i];
    break;
  case 1:
    for(i=0,o=0;i<dim;i++,o+=step){
      float val=(book->valuelist+best*dim)[i];
      if(val==0){
	a[o]=0;
      }else{
	a[o]/=val;
      }
    }
    break;
  }
  return(best);
}

long vorbis_book_codeword(codebook *book,int entry){
  return book->codelist[entry];
}

long vorbis_book_codelen(codebook *book,int entry){
  return book->c->lengthlist[entry];
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

static long full_quantlist1[]={0,1,2,3,    4,5,6,7, 8,3,6,1};
static long partial_quantlist1[]={0,7,2};

/* no mapping */
static_codebook test1={
  4,16,
  NULL,
  0,
  0,0,0,0,
  NULL,
  NULL,NULL
};
static float *test1_result=NULL;
  
/* linear, full mapping, nonsequential */
static_codebook test2={
  4,3,
  NULL,
  2,
  -533200896,1611661312,4,0,
  full_quantlist1,
  NULL,NULL
};
static float test2_result[]={-3,-2,-1,0, 1,2,3,4, 5,0,3,-2};

/* linear, full mapping, sequential */
static_codebook test3={
  4,3,
  NULL,
  2,
  -533200896,1611661312,4,1,
  full_quantlist1,
  NULL,NULL
};
static float test3_result[]={-3,-5,-6,-6, 1,3,6,10, 5,5,8,6};

/* linear, algorithmic mapping, nonsequential */
static_codebook test4={
  3,27,
  NULL,
  1,
  -533200896,1611661312,4,0,
  partial_quantlist1,
  NULL,NULL
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
static_codebook test5={
  3,27,
  NULL,
  1,
  -533200896,1611661312,4,1,
  partial_quantlist1,
  NULL,NULL
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

void run_test(static_codebook *b,float *comp){
  float *out=_book_unquantize(b);
  int i;

  if(comp){
    if(!out){
      fprintf(stderr,"_book_unquantize incorrectly returned NULL\n");
      exit(1);
    }

    for(i=0;i<b->entries*b->dim;i++)
      if(fabs(out[i]-comp[i])>.0001){
	fprintf(stderr,"disagreement in unquantized and reference data:\n"
		"position %d, %g != %g\n",i,out[i],comp[i]);
	exit(1);
      }

  }else{
    if(out){
      fprintf(stderr,"_book_unquantize returned a value array: \n"
	      " correct result should have been NULL\n");
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
