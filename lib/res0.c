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
 last mod: $Id: res0.c,v 1.5 2000/02/12 08:33:09 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"
#include "scales.h"

typedef struct {
  vorbis_info_residue0 *info;
  int bookpointers[64];
} vorbis_look_residue0;

void free_info(vorbis_info_residue *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_residue0));
    free(i);
  }
}

void free_look(vorbis_look_residue *i){
  if(i){
    memset(i,0,sizeof(vorbis_look_residue0));
    free(i);
  }
}

/* not yet */
void pack(vorbis_info_residue *vr,oggpack_buffer *opb){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  int j,acc=0;
  _oggpack_write(opb,info->begin,24);
  _oggpack_write(opb,info->end,24);

  _oggpack_write(opb,info->grouping-1,24);  /* residue vectors to group and 
					     code with a partitioned book */
  _oggpack_write(opb,info->partitions-1,6); /* possible partition choices */
  _oggpack_write(opb,info->groupspercode-1,6); /* partitioned group
                                                codes to encode at
                                                once */
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
  vorbis_info_residue0 *ret=calloc(1,sizeof(vorbis_info_residue0));

  ret->begin=_oggpack_read(opb,24);
  ret->end=_oggpack_read(opb,24);
  ret->grouping=_oggpack_read(opb,24)+1;
  ret->partitions=_oggpack_read(opb,6)+1;
  ret->groupspercode=_oggpack_read(opb,6)+1;
  ret->groupbook=_oggpack_read(opb,8);
  for(j=0;j<ret->partitions;j++)
    acc+=ret->secondstages[j]=_oggpack_read(opb,4);
  for(j=0;j<acc;j++)
    ret->booklist[j]=_oggpack_read(opb,8);

  if(ret->groupbook>=vi->books)goto errout;
  for(j=0;j<acc;j++)
    if(ret->booklist[j]>=vi->books)goto errout;

  return(ret);
 errout:
  free_info(ret);
  return(NULL);
}

vorbis_look_residue *look (vorbis_info *vi,vorbis_info_mode *vm,
			  vorbis_info_residue *vr){
  vorbis_info_residue0 *info=(vorbis_info_residue0 *)vr;
  vorbis_look_residue0 *look=calloc(1,sizeof(vorbis_look_residue0));
  int j,acc=0;
  look->info=info;
  for(j=0;j<info->partitions;j++){
    look->bookpointers[j]=acc;
    acc+=info->secondstages[j];
  }

  return(look);
}

int forward(vorbis_block *vb,vorbis_look_residue *l,
	    double **in,int ch){
  long i,j;
  vorbis_look_residue0 *look=(vorbis_look_residue0 *)l;
  vorbis_info_residue0 *info=look->info;
  for(i=0;i<ch;i++)
    for(j=0;j<vb->pcmend/2;j++)
      _oggpack_write(&vb->opb,rint(in[i][j])+16,5);
  return(0);
}

int inverse(vorbis_block *vb,vorbis_look_residue *l,double **in,int ch){
  long i,j;
  for(i=0;i<ch;i++)
    for(j=0;j<vb->pcmend/2;j++)
      in[i][j]*=_oggpack_read(&vb->opb,5)-16;
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
