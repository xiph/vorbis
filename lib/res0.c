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

 function: residue backend 0 implementation
 last mod: $Id: res0.c,v 1.17 2000/08/15 09:09:43 xiphmont Exp $

 ********************************************************************/

/* Slow, slow, slow, simpleminded and did I mention it was slow?  The
   encode/decode loops are coded for clarity and performance is not
   yet even a nagging little idea lurking in the shadows.  Oh and BTW,
   it's slow. */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"
#include "bookinternal.h"
#include "sharedbook.h"
#include "misc.h"
#include "os.h"

typedef struct {
  vorbis_info_residue0 *info;
  int         map;
  
  int         parts;
  codebook   *phrasebook;
  codebook ***partbooks;

  int         partvals;
  int       **decodemap;
} vorbis_look_residue0;

void free_info(vorbis_info_residue *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_residue0));
    free(i);
  }
}

void free_look(vorbis_look_residue *i){
  int j;
  if(i){
    vorbis_look_residue0 *look=(vorbis_look_residue0 *)i;
    for(j=0;j<look->parts;j++)
      if(look->partbooks[j])free(look->partbooks[j]);
    free(look->partbooks);
    for(j=0;j<look->partvals;j++)
      free(look->decodemap[j]);
    free(look->decodemap);
    memset(i,0,sizeof(vorbis_look_residue0));
    free(i);
  }
}

void pack(vorbis_info_residue *vr,oggpack_buffer *opb){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  int j,acc=0;
  _oggpack_write(opb,info->begin,24);
  _oggpack_write(opb,info->end,24);

  _oggpack_write(opb,info->grouping-1,24);  /* residue vectors to group and 
					     code with a partitioned book */
  _oggpack_write(opb,info->partitions-1,6); /* possible partition choices */
  _oggpack_write(opb,info->groupbook,8);  /* group huffman book */
  for(j=0;j<info->partitions;j++){
    _oggpack_write(opb,info->secondstages[j],4); /* zero *is* a valid choice */
    acc+=info->secondstages[j];
  }
  for(j=0;j<acc;j++)
    _oggpack_write(opb,info->booklist[j],8);

}

/* vorbis_info is for range checking */
vorbis_info_residue *unpack(vorbis_info *vi,oggpack_buffer *opb){
  int j,acc=0;
  vorbis_info_residue0 *info=calloc(1,sizeof(vorbis_info_residue0));

  info->begin=_oggpack_read(opb,24);
  info->end=_oggpack_read(opb,24);
  info->grouping=_oggpack_read(opb,24)+1;
  info->partitions=_oggpack_read(opb,6)+1;
  info->groupbook=_oggpack_read(opb,8);
  for(j=0;j<info->partitions;j++){
    int cascade=info->secondstages[j]=_oggpack_read(opb,4);
    if(cascade>1)goto errout; /* temporary!  when cascading gets
                                 reworked and actually used, we don't
                                 want old code to DTWT */
    acc+=cascade;
  }
  for(j=0;j<acc;j++)
    info->booklist[j]=_oggpack_read(opb,8);

  if(info->groupbook>=vi->books)goto errout;
  for(j=0;j<acc;j++)
    if(info->booklist[j]>=vi->books)goto errout;

  return(info);
 errout:
  free_info(info);
  return(NULL);
}

vorbis_look_residue *look (vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_residue *vr){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  vorbis_look_residue0 *look=calloc(1,sizeof(vorbis_look_residue0));
  int j,k,acc=0;
  int dim;
  look->info=info;
  look->map=vm->mapping;

  look->parts=info->partitions;
  look->phrasebook=vd->fullbooks+info->groupbook;
  dim=look->phrasebook->dim;

  look->partbooks=calloc(look->parts,sizeof(codebook **));

  for(j=0;j<look->parts;j++){
    int stages=info->secondstages[j];
    if(stages){
      look->partbooks[j]=malloc(stages*sizeof(codebook *));
      for(k=0;k<stages;k++)
	look->partbooks[j][k]=vd->fullbooks+info->booklist[acc++];
    }
  }

  look->partvals=rint(pow(look->parts,dim));
  look->decodemap=malloc(look->partvals*sizeof(int *));
  for(j=0;j<look->partvals;j++){
    long val=j;
    long mult=look->partvals/look->parts;
    look->decodemap[j]=malloc(dim*sizeof(int));
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
static int _testhack(double *vec,int n,vorbis_look_residue0 *look,
		     int auxparts,int auxpartnum){
  vorbis_info_residue0 *info=look->info;
  int i,j=0;
  double max,localmax=0.;
  double temp[128];
  double entropy[8];

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

static int _encodepart(oggpack_buffer *opb,double *vec, int n,
		       int stages, codebook **books,int mode,int part){
  int i,j,bits=0;

  for(j=0;j<stages;j++){
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
      bits+=vorbis_book_encode(books[j],entry,opb);
    }
  }

  return(bits);
}

static int _decodepart(oggpack_buffer *opb,double *work,double *vec, int n,
		       int stages, codebook **books){
  int i,j;
  
  memset(work,0,sizeof(double)*n);
  for(j=0;j<stages;j++){
    int dim=books[j]->dim;
    int step=n/dim;
    for(i=0;i<step;i++)
      if(vorbis_book_decodevs(books[j],work+i,opb,step,0)==-1)
	return(-1);
  }
  
  for(i=0;i<n;i++)
    vec[i]*=work[i];
  
  return(0);
}

int forward(vorbis_block *vb,vorbis_look_residue *vl,
	    double **in,int ch){
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
     parition channel words... */
  
  for(i=info->begin,l=0;i<info->end;){
    /* first we encode a partition codeword for each channel */
    for(j=0;j<ch;j++){
      long val=partword[j][l];
      for(k=1;k<partitions_per_word;k++)
	val= val*possible_partitions+partword[j][l+k];
      phrasebits+=vorbis_book_encode(look->phrasebook,val,&vb->opb);
    }
    /* now we encode interleaved residual values for the partitions */
    for(k=0;k<partitions_per_word;k++,l++,i+=samples_per_partition)
      for(j=0;j<ch;j++){
	resbits[partword[j][l]]+=
	  _encodepart(&vb->opb,in[j]+i,samples_per_partition,
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
int inverse(vorbis_block *vb,vorbis_look_residue *vl,double **in,int ch){
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
  double *work=alloca(sizeof(double)*samples_per_partition);
  partvals=partwords*partitions_per_word;

  /* make sure we're zeroed up to the start */
  for(j=0;j<ch;j++)
    memset(in[j],0,sizeof(double)*info->begin);

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
      memset(in[j]+i,0,sizeof(double)*(transend-i));
  }

  return(0);

 errout:
  for(j=0;j<ch;j++)
    memset(in[j],0,sizeof(double)*transend);
  return(0);
}

vorbis_func_residue residue0_exportbundle={
  &pack,
  &unpack,
  &look,
  &free_info,
  &free_look,
  &forward,
  &inverse
};
