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
 last mod: $Id: res0.c,v 1.2 2000/01/28 09:05:16 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"

/* unfinished as of 20000118 */

void free_info(vorbis_info_residue *i){
  vorbis_info_residue0 *d=(vorbis_info_residue0 *)i;
  if(d){
    if(d->books)free(d->books);
    memset(d,0,sizeof(vorbis_info_residue0));
    free(d);
  }
}

void free_look(vorbis_look_residue *i){
}

/* not yet */
void pack(vorbis_info_residue *vr,oggpack_buffer *opb){
}

/* vorbis_info is for range checking */
vorbis_info_residue *unpack(vorbis_info *vi,oggpack_buffer *opb){
}

vorbis_look_residue *look (vorbis_info *vi,vorbis_info_mode *vm,
			  vorbis_info_residue *vr){
}

int forward(vorbis_block *vb,vorbis_look_residue *l,double **in,int **aux,int ch){
}
int inverse(vorbis_block *vb,vorbis_look_residue *l,double **in,int ch){
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
