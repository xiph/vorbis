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

 function: residue backend 0, 1 and 2 implementation
 last mod: $Id: res0.c,v 1.35 2001/08/13 11:33:39 xiphmont Exp $

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

  long      postbits;
  long      phrasebits;
  long      frames;

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

    /*fprintf(stderr,"residue bit usage %f:%f (%f total)\n",
	    (float)look->phrasebits/look->frames,
	    (float)look->postbits/look->frames,
	    (float)(look->postbits+look->phrasebits)/look->frames);*/

    /*vorbis_info_residue0 *info=look->info;

    fprintf(stderr,
	    "%ld frames encoded in %ld phrasebits and %ld residue bits "
	    "(%g/frame) \n",look->frames,look->phrasebits,
	    look->resbitsflat,
	    (look->phrasebits+look->resbitsflat)/(float)look->frames);
    
    for(j=0;j<look->parts;j++){
      long acc=0;
      fprintf(stderr,"\t[%d] == ",j);
      for(k=0;k<look->stages;k++)
	if((info->secondstages[j]>>k)&1){
	  fprintf(stderr,"%ld,",look->resbits[j][k]);
	  acc+=look->resbits[j][k];
	}

      fprintf(stderr,":: (%ld vals) %1.2fbits/sample\n",look->resvals[j],
	      acc?(float)acc/(look->resvals[j]*info->grouping):0);
    }
    fprintf(stderr,"\n");*/

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
    int cascade=oggpack_read(opb,3);
    if(oggpack_read(opb,1))
      cascade|=(oggpack_read(opb,5)<<3);
    info->secondstages[j]=cascade;

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
  int i;
  float max=0.f;
  float temp[128];
  float entropy=0.f;

  /* setup */
  for(i=0;i<n;i++)temp[i]=fabs(vec[i]);

  for(i=0;i<n;i++)
    if(temp[i]>max)max=temp[i];

  for(i=0;i<n;i++)temp[i]=rint(temp[i]);

  for(i=0;i<n;i++)
    entropy+=temp[i];

  for(i=0;i<auxparts-1;i++)
    if(auxpartnum<info->blimit[i] &&
       entropy<=info->entmax[i] &&
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

static long **_01class(vorbis_block *vb,vorbis_look_residue *vl,
		       float **in,int ch,
		       int (*classify)(float *,int,vorbis_look_residue0 *,
				       int,int)){
  long i,j;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  long **partword=_vorbis_block_alloc(vb,ch*sizeof(long *));

  /* we find the partition type for each partition of each
     channel.  We'll go back and do the interleaved encoding in a
     bit.  For now, clarity */
 
  for(i=0;i<ch;i++){
    partword[i]=_vorbis_block_alloc(vb,n/samples_per_partition*sizeof(long));
    memset(partword[i],0,n/samples_per_partition*sizeof(long));
  }

  for(i=0;i<partvals;i++){
    for(j=0;j<ch;j++)
      /* do the partition decision based on the 'entropy'
         int the block */
      partword[j][i]=
	classify(in[j]+i*samples_per_partition+info->begin,
		 samples_per_partition,look,possible_partitions,i);
  
  }

#ifdef TRAIN_RES
  {
    FILE *of;
    char buffer[80];
  
    for(i=0;i<ch;i++){
      sprintf(buffer,"resaux_%d.vqd",vb->mode);
      of=fopen(buffer,"a");
      for(j=0;j<partvals;j++)
	fprintf(of,"%ld, ",partword[i][j]);
      fprintf(of,"\n");
      fclose(of);
    }
  }
#endif
  look->frames++;

  return(partword);
}

static long **_2class(vorbis_block *vb,vorbis_look_residue *vl,
		       float **in,int ch,
		       int (*classify)(float *,int,vorbis_look_residue0 *,
				       int,int)){
  long i,j,k,l;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  long **partword=_vorbis_block_alloc(vb,sizeof(long *));
  float *work=alloca(sizeof(float)*samples_per_partition);
  
  partword[0]=_vorbis_block_alloc(vb,n*ch/samples_per_partition*sizeof(long));
  memset(partword[0],0,n*ch/samples_per_partition*sizeof(long));

  for(i=0,j=0,k=0,l=info->begin;i<partvals;i++){
    for(k=0;k<samples_per_partition;k++){
      work[k]=in[j][l];
      j++;
      if(j>=ch){
	j=0;
	l++;
      }
    }
    partword[0][i]=
      classify(work,samples_per_partition,look,possible_partitions,i);
  }  

#ifdef TRAIN_RES
  {
    FILE *of;
    char buffer[80];
  
    sprintf(buffer,"resaux_%d.vqd",vb->mode);
    of=fopen(buffer,"a");
    for(i=0;i<partvals;i++)
      fprintf(of,"%ld, ",partword[0][i]);
    fprintf(of,"\n");
    fclose(of);
  }
#endif
  look->frames++;

  return(partword);
}

static int _01forward(vorbis_block *vb,vorbis_look_residue *vl,
		      float **in,int ch,
		      int pass,long **partword,
		      int (*encode)(oggpack_buffer *,float *,int,
				    codebook *,vorbis_look_residue0 *)){
  long i,j,k,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int possible_partitions=info->partitions;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  long resbits[128];
  long resvals[128];

#ifdef TRAIN_RES
  FILE *of;
  char buffer[80];
  int m;
  
  for(i=0;i<ch;i++){
    sprintf(buffer,"residue_%d#%d.vqd",vb->mode,pass);
    of=fopen(buffer,"a");
    for(m=0;m<info->end;m++)
      fprintf(of,"%.2f, ",in[i][m]);
    fprintf(of,"\n");
    fclose(of);
  }
#endif      

  memset(resbits,0,sizeof(resbits));
  memset(resvals,0,sizeof(resvals));
  
  /* we code the partition words for each channel, then the residual
     words for a partition per channel until we've written all the
     residual words for that partition word.  Then write the next
     partition channel words... */

  for(s=(pass==0?0:info->passlimit[pass-1]);s<info->passlimit[pass];s++){
    for(i=0;i<partvals;){
      
      /* first we encode a partition codeword for each channel */
      if(s==0){
	for(j=0;j<ch;j++){
	  long val=partword[j][i];
	  long ret;
	  for(k=1;k<partitions_per_word;k++){
	    val*=possible_partitions;
	    if(i+k<partvals)
	      val+=partword[j][i+k];
	  }	

	  /* training hack */
	  if(val<look->phrasebook->entries)
	    ret=vorbis_book_encode(look->phrasebook,val,&vb->opb);
	  /*else
	    fprintf(stderr,"!");*/
	  
	  look->phrasebits+=ret;
	
	}
      }
      
      /* now we encode interleaved residual values for the partitions */
      for(k=0;k<partitions_per_word && i<partvals;k++,i++){
	long offset=i*samples_per_partition+info->begin;
	
	for(j=0;j<ch;j++){
	  if(s==0)resvals[partword[j][i]]+=samples_per_partition;
	  if(info->secondstages[partword[j][i]]&(1<<s)){
	    codebook *statebook=look->partbooks[partword[j][i]][s];
	    if(statebook){
	      int ret=encode(&vb->opb,in[j]+offset,samples_per_partition,
			     statebook,look);
	      look->postbits+=ret;
	      resbits[partword[j][i]]+=ret;
	    }
	  }
	}
      }
    }
  }

  /*{
    long total=0;
    long totalbits=0;
    fprintf(stderr,"%d :: ",vb->mode);
    for(k=0;k<possible_partitions;k++){
      fprintf(stderr,"%ld/%1.2g, ",resvals[k],(float)resbits[k]/resvals[k]);
      total+=resvals[k];
      totalbits+=resbits[k];
    }
    
    fprintf(stderr,":: %ld:%1.2g\n",total,(double)totalbits/total);
    }*/
  return(0);
}

/* a truncated packet here just means 'stop working'; it's not an error */
static int _01inverse(vorbis_block *vb,vorbis_look_residue *vl,
		      float **in,int ch,
		      long (*decodepart)(codebook *, float *, 
					 oggpack_buffer *,int)){

  long i,j,k,l,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;
  
  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int ***partword=alloca(ch*sizeof(int **));

  for(j=0;j<ch;j++)
    partword[j]=_vorbis_block_alloc(vb,partwords*sizeof(int *));

  for(s=0;s<look->stages;s++){

    /* each loop decodes on partition codeword containing 
       partitions_pre_word partitions */
    for(i=0,l=0;i<partvals;l++){
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
      for(k=0;k<partitions_per_word && i<partvals;k++,i++)
	for(j=0;j<ch;j++){
	  long offset=info->begin+i*samples_per_partition;
	  if(info->secondstages[partword[j][l][k]]&(1<<s)){
	    codebook *stagebook=look->partbooks[partword[j][l][k]][s];
	    if(stagebook){
	      if(decodepart(stagebook,in[j]+offset,&vb->opb,
			    samples_per_partition)==-1)goto eopbreak;
	    }
	  }
	}
    } 
  }
  
 errout:
 eopbreak:
  return(0);
}

/* residue 0 and 1 are just slight variants of one another. 0 is
   interleaved, 1 is not */
long **res0_class(vorbis_block *vb,vorbis_look_residue *vl,
		  float **in,int *nonzero,int ch){
  /* we encode only the nonzero parts of a bundle */
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    /*return(_01class(vb,vl,in,used,_interleaved_testhack));*/
    return(_01class(vb,vl,in,used,_testhack));
  else
    return(0);
}

int res0_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,float **out,int *nonzero,int ch,
		 int pass, long **partword){
  /* we encode only the nonzero parts of a bundle */
  int i,j,used=0,n=vb->pcmend/2;
  for(i=0;i<ch;i++)
    if(nonzero[i]){
      for(j=0;j<n;j++)
	out[i][j]+=in[i][j];
      in[used++]=in[i];
    }
  if(used){
    int ret=_01forward(vb,vl,in,used,pass,partword,
		      _interleaved_encodepart);
    used=0;
    for(i=0;i<ch;i++)
      if(nonzero[i]){
	for(j=0;j<n;j++)
	  out[i][j]-=in[used][j];
	used++;
      }
    return(ret);
  }else
    return(0);
}

int res0_inverse(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01inverse(vb,vl,in,used,vorbis_book_decodevs_add));
  else
    return(0);
}

int res1_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,float **out,int *nonzero,int ch,
		 int pass, long **partword){
  int i,j,used=0,n=vb->pcmend/2;
  for(i=0;i<ch;i++)
    if(nonzero[i]){
      for(j=0;j<n;j++)
	out[i][j]+=in[i][j];
      in[used++]=in[i];
    }

  if(used){
    int ret=_01forward(vb,vl,in,used,pass,partword,_encodepart);
    used=0;
    for(i=0;i<ch;i++)
      if(nonzero[i]){
	for(j=0;j<n;j++)
	  out[i][j]-=in[used][j];
	used++;
      }
    return(ret);
  }else
    return(0);
}

long **res1_class(vorbis_block *vb,vorbis_look_residue *vl,
		  float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01class(vb,vl,in,used,_testhack));
  else
    return(0);
}

int res1_inverse(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_01inverse(vb,vl,in,used,vorbis_book_decodev_add));
  else
    return(0);
}

long **res2_class(vorbis_block *vb,vorbis_look_residue *vl,
		  float **in,int *nonzero,int ch){
  int i,used=0;
  for(i=0;i<ch;i++)
    if(nonzero[i])
      in[used++]=in[i];
  if(used)
    return(_2class(vb,vl,in,used,_testhack));
  else
    return(0);
}

/* res2 is slightly more different; all the channels are interleaved
   into a single vector and encoded. */

int res2_forward(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,float **out,int *nonzero,int ch,
		 int pass,long **partword){
  long i,j,k,n=vb->pcmend/2,used=0;

  /* don't duplicate the code; use a working vector hack for now and
     reshape ourselves into a single channel res1 */
  /* ugly; reallocs for each coupling pass :-( */
  float *work=_vorbis_block_alloc(vb,ch*n*sizeof(float));
  for(i=0;i<ch;i++){
    float *pcm=in[i];
    if(nonzero[i])used++;
    for(j=0,k=i;j<n;j++,k+=ch)
      work[k]=pcm[j];
  }
  
  if(used){
    int ret=_01forward(vb,vl,&work,1,pass,partword,_encodepart);
    /* update the sofar vector */
    for(i=0;i<ch;i++){
      float *pcm=in[i];
      float *sofar=out[i];
      for(j=0,k=i;j<n;j++,k+=ch)
	sofar[j]+=pcm[j]-work[k];
    }

    return(ret);
  }else
    return(0);
}

/* duplicate code here as speed is somewhat more important */
int res2_inverse(vorbis_block *vb,vorbis_look_residue *vl,
		 float **in,int *nonzero,int ch){
  long i,k,l,s;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)vl;
  vorbis_info_residue0 *info=look->info;

  /* move all this setup out later */
  int samples_per_partition=info->grouping;
  int partitions_per_word=look->phrasebook->dim;
  int n=info->end-info->begin;

  int partvals=n/samples_per_partition;
  int partwords=(partvals+partitions_per_word-1)/partitions_per_word;
  int **partword=_vorbis_block_alloc(vb,partwords*sizeof(int *));

  for(i=0;i<ch;i++)if(nonzero[i])break;
  if(i==ch)return(0); /* no nonzero vectors */

  for(s=0;s<look->stages;s++){
    for(i=0,l=0;i<partvals;l++){

      if(s==0){
	/* fetch the partition word */
	int temp=vorbis_book_decode(look->phrasebook,&vb->opb);
	if(temp==-1)goto eopbreak;
	partword[l]=look->decodemap[temp];
	if(partword[l]==NULL)goto errout;
      }

      /* now we decode residual values for the partitions */
      for(k=0;k<partitions_per_word && i<partvals;k++,i++)
	if(info->secondstages[partword[l][k]]&(1<<s)){
	  codebook *stagebook=look->partbooks[partword[l][k]][s];
	  
	  if(stagebook){
	    if(vorbis_book_decodevv_add(stagebook,in,
					i*samples_per_partition+info->begin,ch,
					&vb->opb,samples_per_partition)==-1)
	      goto eopbreak;
	  }
	}
    } 
  }
  
 errout:
 eopbreak:
  return(0);
}


vorbis_func_residue residue0_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res0_class,
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
  &res1_class,
  &res1_forward,
  &res1_inverse
};

vorbis_func_residue residue2_exportbundle={
  &res0_pack,
  &res0_unpack,
  &res0_look,
  &res0_copy_info,
  &res0_free_info,
  &res0_free_look,
  &res2_class,
  &res2_forward,
  &res2_inverse
};
