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

 function: residue backend 0 implementation
 last mod: $Id: res0.c,v 1.22 2000/11/17 11:47:18 xiphmont Exp $

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
#include "bitbuffer.h"

typedef struct {
  vorbis_info_residue0 *info;
  int         map;
  
  int         parts;
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

void res0_pack(vorbis_info_residue *vr,oggpack_buffer *opb){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  int j,acc=0;
  oggpack_write(opb,info->begin,24);
  oggpack_write(opb,info->end,24);

  oggpack_write(opb,info->grouping-1,24);  /* residue vectors to group and 
					     code with a partitioned book */
  oggpack_write(opb,info->partitions-1,6); /* possible partition choices */
  oggpack_write(opb,info->groupbook,8);  /* group huffman book */
  for(j=0;j<info->partitions;j++){
    oggpack_write(opb,info->secondstages[j],4); /* zero *is* a valid choice */
    acc+=info->secondstages[j];
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
    int cascade=info->secondstages[j]=oggpack_read(opb,4);
    if(cascade>1)goto errout; /* temporary!  when cascading gets
                                 reworked and actually used, we don't
                                 want old code to DTWT */
    acc+=cascade;
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
  look->info=info;
  look->map=vm->mapping;

  look->parts=info->partitions;
  look->phrasebook=be->fullbooks+info->groupbook;
  dim=look->phrasebook->dim;

  look->partbooks=_ogg_calloc(look->parts,sizeof(codebook **));

  for(j=0;j<look->parts;j++){
    int stages=info->secondstages[j];
    if(stages){
      look->partbooks[j]=_ogg_malloc(stages*sizeof(codebook *));
      for(k=0;k<stages;k++)
	look->partbooks[j][k]=be->fullbooks+info->booklist[acc++];
    }
  }

  look->partvals=rint(pow(look->parts,dim));
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
static int _testhack(float *vec,int n,vorbis_look_residue0 *look,
		     int auxparts,int auxpartnum){
  vorbis_info_residue0 *info=look->info;
  int i,j=0;
  float max,localmax=0.;
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
    j++;

    if(n<=0)break;
    for(i=0;i<n;i++){
      temp[i]+=temp[i+n];
    }
    localmax=0.;
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

static int _encodepart(vorbis_bitbuffer *vbb,float *vec, int n,
		       int stages, codebook **books,int mode,int part){
  int i,j=0,bits=0;
  if(stages){
    int dim=books[j]->dim;
    int step=n/dim;
    for(i=0;i<step;i++){
      int entry=vorbis_book_besterror(books[j],vec+i,step,0);
#ifdef TRAIN_RESENT      
      {
	char buf[80];
	FILE *f;
	sprintf(buf,"res0_%da%d_%d.vqd",mode,j,part);
	f=fopen(buf,"a");
	fprintf(f,"%d\n",entry);
	fclose(f);
      }
#endif
      bits+=vorbis_book_bufencode(books[j],entry,vbb);
    }
  }
  return(bits);
}

static int _decodepart(oggpack_buffer *opb,float *work,float *vec, int n,
		       int stages, codebook **books){
  int i;
  
  memset(work,0,sizeof(float)*n);
  for(i=0;i<stages;i++){
    int dim=books[i]->dim;
    int step=n/dim;
    if(s_vorbis_book_decodevs(books[i],work,opb,step,0)==-1)
      return(-1);
  }
  
  for(i=0;i<n;i++)
    vec[i]*=work[i];
  
  return(0);
}

int res0_forward(vorbis_block *vb,vorbis_look_residue *vl,
	    float **in,int ch,vorbis_bitbuffer *vbb){
  long i,j,k,l;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;
  long phrasebits=0,resbitsT=0;
  long *resbits=alloca(sizeof(long)*possible_partitions);
  long *resvals=alloca(sizeof(long)*possible_partitions);

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  long **partword=_vorbis_block_alloc(vb,ch*sizeof(long *));

  partvals=partwords*partitions_per_word;

  /* we find the patition type for each partition of each
     channel.  We'll go back and do the interleaved encoding in a
     bit.  For now, clarity */
  
  memset(resbits,0,sizeof(long)*possible_partitions);
  memset(resvals,0,sizeof(long)*possible_partitions);

  for(i=0;i<ch;i++){
    partword[i]=_vorbis_block_alloc(vb,n/samples_per_partition*sizeof(long));
    memset(partword[i],0,n/samples_per_partition*sizeof(long));
  }

  for(i=info->begin,l=0;i<info->end;i+=samples_per_partition,l++){
    for(j=0;j<ch;j++)
      /* do the partition decision based on the number of 'bits'
         needed to encode the block */
      partword[j][l]=
	_testhack(in[j]+i,samples_per_partition,look,possible_partitions,l);
  
  }
  /* we code the partition words for each channel, then the residual
     words for a partition per channel until we've written all the
     residual words for that partition word.  Then write the next
     partition channel words... */
  
  for(i=info->begin,l=0;i<info->end;){
    
    /* first we encode a partition codeword for each channel */
    for(j=0;j<ch;j++){
      long val=partword[j][l];
      for(k=1;k<partitions_per_word;k++)
	val= val*possible_partitions+partword[j][l+k];
      phrasebits+=vorbis_book_bufencode(look->phrasebook,val,vbb);
    }
    /* now we encode interleaved residual values for the partitions */
    for(k=0;k<partitions_per_word;k++,l++,i+=samples_per_partition)
      for(j=0;j<ch;j++){
	/*resbits[partword[j][l]]+=*/
	resbitsT+=_encodepart(vbb,in[j]+i,samples_per_partition,
			      info->secondstages[partword[j][l]],
			      look->partbooks[partword[j][l]],look->map,partword[j][l]);
	resvals[partword[j][l]]+=samples_per_partition;
      }
      
  }

  for(i=0;i<possible_partitions;i++)resbitsT+=resbits[i];
  /*fprintf(stderr,
    "Encoded %ld res vectors in %ld phrasing and %ld res bits\n\t",
    ch*(info->end-info->begin),phrasebits,resbitsT);
    for(i=0;i<possible_partitions;i++)
    fprintf(stderr,"%ld(%ld):%ld ",i,resvals[i],resbits[i]);
    fprintf(stderr,"\n");*/
 
  return(0);
}

/* a truncated packet here just means 'stop working'; it's not an error */
int res0_inverse(vorbis_block *vb,vorbis_look_residue *vl,float **in,int ch){
  long i,j,k,l,transend=vb->pcmend/2;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int **partword=alloca(ch*sizeof(long *));
  float *work=alloca(sizeof(float)*samples_per_partition);
  partvals=partwords*partitions_per_word;

  /* make sure we're zeroed up to the start */
  for(j=0;j<ch;j++)
    memset(in[j],0,sizeof(float)*info->begin);

  for(i=info->begin,l=0;i<info->end;){
    /* fetch the partition word for each channel */
    for(j=0;j<ch;j++){
      int temp=vorbis_book_decode(look->phrasebook,&vb->opb);
      if(temp==-1)goto eopbreak;
      partword[j]=look->decodemap[temp];
      if(partword[j]==NULL)goto errout;
    }
    
    /* now we decode interleaved residual values for the partitions */
    for(k=0;k<partitions_per_word;k++,l++,i+=samples_per_partition)
      for(j=0;j<ch;j++){
	int part=partword[j][k];
	if(_decodepart(&vb->opb,work,in[j]+i,samples_per_partition,
		    info->secondstages[part],
		       look->partbooks[part])==-1)goto eopbreak;
      }
  }

 eopbreak:
  if(i<transend){
    for(j=0;j<ch;j++)
      memset(in[j]+i,0,sizeof(float)*(transend-i));
  }

  return(0);

 errout:
  for(j=0;j<ch;j++)
    memset(in[j],0,sizeof(float)*transend);
  return(0);
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
