/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: residue backend 0 and 1 implementation
 last mod: $Id: res0.c,v 1.29 2001/06/04 05:56:42 xiphmont Exp $

 ********************************************************************/

/* Slow, slow, slow, simpleminded and did I mention it was slow?  The
   encode/decode loops are coded for clarity and performance is not
   yet even a nagging little idea lurking in the shadows.  Oh and BTW,
   it's slow. */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"
#include "os.h"

typedef struct {
  vorbis_info_residue0 *info;
  int         map;
  
  int         parts;
  int         stages;
  codebook   *fullbooks;
  codebook   *phrasebook;
  codebook ***partbooks;

  int         partvals;
  int       **decodemap;

} vorbis_look_residue0;

vorbis_info_residue *res0_copy_info(vorbis_info_residue *vr){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  vorbis_info_residue0 *ret=_ogg_malloc(sizeof(vorbis_info_residue0));
  memcpy(ret,info,sizeof(vorbis_info_residue0));
  return(ret);
}

void res0_free_info(vorbis_info_residue *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_residue0));
    _ogg_free(i);
  }
}

void res0_free_look(vorbis_look_residue *i){
  int j;
  if(i){
    vorbis_look_residue0 *look=(vorbis_look_residue0 *)i;
    long resbitsT=0;
    long resvalsT=0;

    for(j=0;j<look->parts;j++)
      if(look->partbooks[j])_ogg_free(look->partbooks[j]);
    _ogg_free(look->partbooks);
    for(j=0;j<look->partvals;j++)
      _ogg_free(look->decodemap[j]);
    _ogg_free(look->decodemap);
    memset(i,0,sizeof(vorbis_look_residue0));
    _ogg_free(i);
  }
}

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static int icount(unsigned int v){
  int ret=0;
  while(v){
    ret+=v&1;
    v>>=1;
  }
  return(ret);
}


void res0_pack(vorbis_info_residue *vr,oggpack_buffer *opb){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  int j,acc=0;
  oggpack_write(opb,info->begin,24);
  oggpack_write(opb,info->end,24);

  oggpack_write(opb,info->grouping-1,24);  /* residue vectors to group and 
					     code with a partitioned book */
  oggpack_write(opb,info->partitions-1,6); /* possible partition choices */
  oggpack_write(opb,info->groupbook,8);  /* group huffman book */

  /* secondstages is a bitmask; as encoding progresses pass by pass, a
     bitmask of one indicates this partition class has bits to write
     this pass */
  for(j=0;j<info->partitions;j++){
    if(ilog(info->secondstages[j])>3){
      /* yes, this is a minor hack due to not thinking ahead */
      oggpack_write(opb,info->secondstages[j],3); 
      oggpack_write(opb,1,1);
      oggpack_write(opb,info->secondstages[j]>>3,5); 
    }else
      oggpack_write(opb,info->secondstages[j],4); /* trailing zero */
    acc+=icount(info->secondstages[j]);
  }
  for(j=0;j<acc;j++)
    oggpack_write(opb,info->booklist[j],8);

}

/* vorbis_info is for range checking */
vorbis_info_residue *res0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int j,acc=0;
  vorbis_info_residue0 *info=_ogg_calloc(1,sizeof(vorbis_info_residue0));
  codec_setup_info     *ci=vi->codec_setup;

  info->begin=oggpack_read(opb,24);
  info->end=oggpack_read(opb,24);
  info->grouping=oggpack_read(opb,24)+1;
  info->partitions=oggpack_read(opb,6)+1;
  info->groupbook=oggpack_read(opb,8);

  for(j=0;j<info->partitions;j++){
    int cascade=info->secondstages[j]=oggpack_read(opb,3);
    if(oggpack_read(opb,1))
      cascade|=(oggpack_read(opb,5)<<3);
    acc+=icount(cascade);
  }
  for(j=0;j<acc;j++)
    info->booklist[j]=oggpack_read(opb,8);

  if(info->groupbook>=ci->books)goto errout;
  for(j=0;j<acc;j++)
    if(info->booklist[j]>=ci->books)goto errout;

  return(info);
 errout:
  res0_free_info(info);
  return(NULL);
}

vorbis_look_residue *res0_look (vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_residue *vr){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  vorbis_look_residue0 *look=_ogg_calloc(1,sizeof(vorbis_look_residue0));
  backend_lookup_state *be=vd->backend_state;

  int j,k,acc=0;
  int dim;
  int maxstage=0;
  look->info=info;
  look->map=vm->mapping;

  look->parts=info->partitions;
  look->fullbooks=be->fullbooks;
  look->phrasebook=be->fullbooks+info->groupbook;
  dim=look->phrasebook->dim;

  look->partbooks=_ogg_calloc(look->parts,sizeof(codebook **));

  for(j=0;j<look->parts;j++){
    int stages=ilog(info->secondstages[j]);
    if(stages){
      if(stages>maxstage)maxstage=stages;
      look->partbooks[j]=_ogg_calloc(stages,sizeof(codebook *));
      for(k=0;k<stages;k++)
	if(info->secondstages[j]&(1<<k))
	  look->partbooks[j][k]=be->fullbooks+info->booklist[acc++];
    }
  }

  look->partvals=rint(pow(look->parts,dim));
  look->stages=maxstage;
  look->decodemap=_ogg_malloc(look->partvals*sizeof(int *));
  for(j=0;j<look->partvals;j++){
    long val=j;
    long mult=look->partvals/look->parts;
    look->decodemap[j]=_ogg_malloc(dim*sizeof(int));
    for(k=0;k<dim;k++){
      long deco=val/mult;
      val-=deco*mult;
      mult/=look->parts;
      look->decodemap[j][k]=deco;
    }
  }

  return(look);
}


/* does not guard against invalid settings; eg, a subn of 16 and a
   subgroup request of 32.  Max subn of 128 */
static int _interleaved_testhack(float *vec,int n,vorbis_look_residue0 *look,
				 int auxparts,int auxpartnum){
  vorbis_info_residue0 *info=look->info;
  int i,j=0;
  float max,localmax=0.f;
  float temp[128];
  float entropy[8];

  /* setup */
  for(i=0;i<n;i++)temp[i]=fabs(vec[i]);

  /* handle case subgrp==1 outside */
  for(i=0;i<n;i++)
    if(temp[i]>localmax)localmax=temp[i];
  max=localmax;

  for(i=0;i<n;i++)temp[i]=rint(temp[i]);
  
  while(1){
    entropy[j]=localmax;
    n>>=1;
    if(!n)break;
    j++;

    for(i=0;i<n;i++){
      temp[i]+=temp[i+n];
    }
    localmax=0.f;
    for(i=0;i<n;i++)
      if(temp[i]>localmax)localmax=temp[i];
  }

  for(i=0;i<auxparts-1;i++)
    if(auxpartnum<info->blimit[i] &&
       entropy[info->subgrp[i]]<=info->entmax[i] &&
       max<=info->ampmax[i])
      break;

  return(i);
}

static int _testhack(float *vec,int n,vorbis_look_residue0 *look,
		     int auxparts,int auxpartnum){
  vorbis_info_residue0 *info=look->info;
  int i,j=0;
  float max,localmax=0.f;
  float temp[128];
  float entropy[8];

  /* setup */
  for(i=0;i<n;i++)temp[i]=fabs(vec[i]);

  /* handle case subgrp==1 outside */
  for(i=0;i<n;i++)
    if(temp[i]>localmax)localmax=temp[i];
  max=localmax;

  for(i=0;i<n;i++)temp[i]=rint(temp[i]);
  
  while(n){
    entropy[j]=localmax;
    n>>=1;
    j++;
    if(!n)break;
    for(i=0;i<n;i++){
      temp[i]=temp[i*2]+temp[i*2+1];
    }
    localmax=0.f;
    for(i=0;i<n;i++)
      if(temp[i]>localmax)localmax=temp[i];
  }

  for(i=0;i<auxparts-1;i++)
    if(auxpartnum<info->blimit[i] &&
       entropy[info->subgrp[i]]<=info->entmax[i] &&
       max<=info->ampmax[i])
      break;

  return(i);
}

static int _interleaved_encodepart(oggpack_buffer *opb,float *vec, int n,
				   codebook *book,vorbis_look_residue0 *look){
  int i,bits=0;
  int dim=book->dim;
  int step=n/dim;
#ifdef TRAIN_RESENT      
  char buf[80];
  FILE *f;
  sprintf(buf,"res0_b%d.vqd",book-look->fullbooks);
  f=fopen(buf,"a");
#endif

  for(i=0;i<step;i++){
    int entry=vorbis_book_besterror(book,vec+i,step,0);

#ifdef TRAIN_RESENT      
    fprintf(f,"%d\n",entry);
#endif

    bits+=vorbis_book_encode(book,entry,opb);
  }

#ifdef TRAIN_RESENT      
  fclose(f);
#endif
  return(bits);
}
 
static int _encodepart(oggpack_buffer *opb,float *vec, int n,
		       codebook *book,vorbis_look_residue0 *look){
  int i,bits=0;
  int dim=book->dim;
  int step=n/dim;
#ifdef TRAIN_RESENT      
  char buf[80];
  FILE *f;
  sprintf(buf,"res0_b%d.vqd",book-look->fullbooks);
  f=fopen(buf,"a");
#endif

  for(i=0;i<step;i++){
    int entry=vorbis_book_besterror(book,vec+i*dim,1,0);

#ifdef TRAIN_RESENT      
    fprintf(f,"%d\n",entry);
#endif

    bits+=vorbis_book_encode(book,entry,opb);
  }

#ifdef TRAIN_RESENT      
  fclose(f);
#endif
  return(bits);
}

static int _01forward(vorbis_block *vb,vorbis_look_residue *vl,
		      float **in,int ch,
		      int (*classify)(float *,int,vorbis_look_residue0 *,
				      int,int),
		      int (*encode)(oggpack_buffer *,float *,int,
				    codebook *,vorbis_look_residue0 *)){
  long i,j,k,l,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  long **partword=_vorbis_block_alloc(vb,ch*sizeof(long *));

  partvals=partwords*partitions_per_word;

  /* we find the patition type for each partition of each
     channel.  We'll go back and do the interleaved encoding in a
     bit.  For now, clarity */
 
  for(i=0;i<ch;i++){
    partword[i]=_vorbis_block_alloc(vb,n/samples_per_partition*sizeof(long));
    memset(partword[i],0,n/samples_per_partition*sizeof(long));
  }

  for(i=info->begin,l=0;i<info->end;i+=samples_per_partition,l++){
    for(j=0;j<ch;j++)
      /* do the partition decision based on the 'entropy'
         int the block */
      partword[j][l]=
	classify(in[j]+i,samples_per_partition,look,possible_partitions,l);
  
  }

  /* we code the partition words for each channel, then the residual
     words for a partition per channel until we've written all the
     residual words for that partition word.  Then write the next
     partition channel words... */

  for(s=0;s<look->stages;s++){
    for(i=info->begin,l=0;i<info->end;){

      /* first we encode a partition codeword for each channel */
      if(s==0){
	for(j=0;j<ch;j++){
	  long val=partword[j][l];
	  for(k=1;k<partitions_per_word;k++)
	    val= val*possible_partitions+partword[j][l+k];
	  look->phrase+=vorbis_book_encode(look->phrasebook,val,&vb->opb);
	}
      }

      /* now we encode interleaved residual values for the partitions */
      for(k=0;k<partitions_per_word;k++,l++,i+=samples_per_partition){
	
	for(j=0;j<ch;j++){
	  if(info->secondstages[partword[j][l]]&(1<<s)){
	    codebook *statebook=look->partbooks[partword[j][l]][s];
	    if(statebook){
	      int ret=encode(&vb->opb,in[j]+i,samples_per_partition,
			       statebook,look);
	      look->bits[partword[j][l]]+=ret;
	      if(s==0)look->vals[partword[j][l]]+=samples_per_partition;
	    }
	  }
	}
      }
    }
  }

  return(0);
}

/* a truncated packet here just means 'stop working'; it's not an error */
static int _01inverse(vorbis_block *vb,vorbis_look_residue *vl,
		      float **in,int ch,
		      long (*decodepart)(codebook *, float *, 
					 oggpack_buffer *,int,int)){

  long i,j,k,l,s,transend=vb->pcmend/2;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int ***partword=alloca(ch*sizeof(int **));
  float **work=alloca(ch*sizeof(float *));
  partvals=partwords*partitions_per_word;

  /* make sure we're zeroed up to the start */
  for(j=0;j<ch;j++){
    work[j]=_vorbis_block_alloc(vb,n*sizeof(float));
    partword[j]=_vorbis_block_alloc(vb,partwords*sizeof(int *));
    memset(work[j],0,sizeof(float)*n);
  }

  for(s=0;s<look->stages;s++){
    for(i=info->begin,l=0;i<info->end;l++){

      if(s==0){
	/* fetch the partition word for each channel */
	for(j=0;j<ch;j++){
	  int temp=vorbis_book_decode(look->phrasebook,&vb->opb);
	  if(temp==-1)goto eopbreak;
	  partword[j][l]=look->decodemap[temp];
	  if(partword[j][l]==NULL)goto errout;
	}
      }
      
      /* now we decode residual values for the partitions */
      for(k=0;k<partitions_per_word;k++,i+=samples_per_partition)
	for(j=0;j<ch;j++){
	  if(info->secondstages[partword[j][l][k]]&(1<<s)){
	    codebook *stagebook=look->partbooks[partword[j][l][k]][s];
	    if(stagebook){
	      if(decodepart(stagebook,work[j]+i,&vb->opb,
			    samples_per_partition,0)==-1)goto eopbreak;
	    }
	  }
	}
    } 
  }

 eopbreak:
  
  for(j=0;j<ch;j++){
    for(i=0;i<n;i++)
      in[j][i]*=work[j][i];
    for(;i<transend;i++)
      in[j][i]=0;
  }

  return(0);

 errout:
  for(j=0;j<ch;j++)
    memset(in[j],0,sizeof(float)*transend);
  return(0);
}

/* residue 0 and 1 are just slight variants of one another. 0 is
   interleaved, 1 is not */
int res0_forward(vorbis_block *vb,vorbis_look_residue *vl,
	    float **in,int ch){
  return(_01forward(vb,vl,in,ch,_interleaved_testhack,_interleaved_encodepart));
}

int res0_inverse(vorbis_block *vb,vorbis_look_residue *vl,float **in,int ch){
  return(_01inverse(vb,vl,in,ch,vorbis_book_decodevs));
}

int res1_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int ch){
  return(_01forward(vb,vl,in,ch,_testhack,_encodepart));
}

int res1_inverse(vorbis_block *vb,vorbis_look_residue *vl,float **in,int ch){
  return(_01inverse(vb,vl,in,ch,vorbis_book_decodev));
}

vorbis_func_residue residue0_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res0_forward,
  &res0_inverse
};

vorbis_func_residue residue1_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res1_forward,
  &res1_inverse
};
