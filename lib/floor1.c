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

 function: floor backend 1 implementation
 last mod: $Id: floor1.c,v 1.1.2.1 2001/04/05 00:22:48 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"

#include <stdio.h>

#define floor1_rangedB 140 /* floor 0 fixed at -140dB to 0dB range */


typedef struct {
  int lo_neighbor[66];
  int hi_neighbor[66];
  int n;
  vorbis_info_floor1 *vi;
} vorbis_look_floor1;

/***********************************************/

static vorbis_info_floor *floor1_copy_info (vorbis_info_floor *i){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  vorbis_info_floor1 *ret=_ogg_malloc(sizeof(vorbis_info_floor1));
  memcpy(ret,info,sizeof(vorbis_info_floor1));
  return(ret);
}

static void floor1_free_info(vorbis_info_floor *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_floor1));
    _ogg_free(i);
  }
}

static void floor1_free_look(vorbis_look_floor *i){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)i;
  if(i){
    memset(look,0,sizeof(vorbis_look_floor1));
    free(i);
  }
}

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

static void floor1_pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  int j,k;
  int count=0;
  int rangebits;

  oggpack_write(opb,info->maxrange,24);
  rangebits=ilog2(info->maxrange);
  
  oggpack_write(opb,info->ranges,4);     /* only 0 to 8 legal now */
  oggpack_write(opb,info->rangeres-1,3); /* only 6,7 or 8 legal now */ 
  for(j=0,k=0;j<info->ranges;j++){
    oggpack_write(opb,info->rangesizes[j]-1,3);
    oggpack_write(opb,info->positionbook[j],8);
    oggpack_write(opb,info->ampbook[j],8);
    count+=info->rangesizes[j];
    for(;k<count;k++)
      oggpack_write(opb,info->rangelist[k],rangebits);
  }
}

static vorbis_info_floor *floor1_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=vi->codec_setup;
  int j,k,count;

  vorbis_info_floor1 *info=_ogg_malloc(sizeof(vorbis_info_floor1));
  int rangebits;

  info->maxrange=oggpack_read(opb,24);
  rangebits=ilog2(info->maxrange);

  info->ranges=oggpack_read(opb,4);
  if(info->ranges>8)goto err_out;

  info->rangeres=oggpack_read(opb,3)+1;
  if(info->rangeres<6)goto err_out;
  if(info->rangeres>8)goto err_out;

  for(j=0,k=0;j<info->ranges;j++){
    info->rangesizes[j]=oggpack_read(opb,3)+1;
    info->positionbook[j]=oggpack_read(opb,8);
    if(info->positionbook[j]<0 || info->positionbook[j]>=ci->books)
      goto err_out;

    info->ampbook[j]=oggpack_read(opb,8);
    if(info->ampbook[j]<0 || info->ampbook[j]>=ci->books)
      goto err_out;
    count+=info->rangesizes[j];
    for(;k<count;k++){
      info->rangelist[k]=oggpack_read(opb,rangebits);
      if(info->rangelist[k]<0 || info->rangelist[k]>=maxrange)
	goto err_out;
    }
  }
  
 err_out:
  floor1_free_info(info);
  return(NULL);
}

static vorbis_look_floor *floor1_look(vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *i){

  vorbis_info        *vi=vd->vi;
  codec_setup_info   *ci=vi->codec_setup;
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  vorbis_look_floor1 *look=_ogg_calloc(1,sizeof(vorbis_look_floor1));
  int i,j,n=0;

  look->vi=vi;
  look->n=ci->blocksizes[mi->blockflag]/2;

  /* we drop each position value in-between already decoded values,
     and use linear interpolation to predict each new value past the
     edges.  The positions are read in the order of the position
     list... we precompute the bounding positions in the lookup.  Of
     course, the neighbors can change (if a position is declined), but
     this is an initial mapping */

  for(i=0;i<info->ranges;i++)n+=info->rangesizes[j];
  for(i=0;i<info->ranges;i++){
    int hi=-1;
    int lo=-1;
    for(j=0;j<i;j++){
      if(info->rangelist[j]<info->rangelist[i] && 
	 (lo==-1 || info->rangelist[j]>info->rangelist[lo]))
	lo=j;
      if(info->rangelist[j]>info->rangelist[i] && 
	 (hi==-1 || info->rangelist[j]<info->rangelist[hi]))
	hi=j;
    }
    look->lo_neighbor[i]=lo;
    look->hi_neighbor[i]=hi;
  }

  return(look);
}

static int render_point_dB(int x0,int x1,int y0,int y1,int x){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int err=(ady>>1)+ady*(x-x0);
  
  int off=err%adx;
  if(dy<0)return(y0-off);
  return(y0+off);
}

static int render_line_dB(int x0,int x1,int y0,int y1,int *d){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err;

  ady-=base*adx;
  err=ady>>1;

  d[x]=y;
  while(++x<x1){
    err=err+ady;
    if(err>adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }
    d[x]=y;
  }
}

/* the floor has already been filtered to only include relevant sections */
static int fit_line(int *floor,int x0, int x1,int *y0, int *y1){
  long i,n=0;
  long x=0,y=0,x2=0,y2=0,xy=0;
  
  if(*y0>-900){  /* hint used to break degenerate cases */
    x+=x0;
    y+=*y0;
    x2+=(x0*x0);
    y2+= *y0 * *y0;
    xy= *y0*x0;
    n++;
  }

  if(*y1>-900){  /* hint used to break degenerate cases */
    x+=x1;
    y+=*y1;
    x2+=(x1*x1);
    y2+= *y1 * *y1;
    xy= *y1*x1;
    n++;
  }

  for(i=x0;i<x1;i++)
    if(floor[i]>-900){
      x+=i;
      y+=floor[i];
      x2+=(i*i);
      y2+=floor[i]*floor[i];
      xy=i*floor[i];
      n++;
    }
  if(n<2)return(-1);
  
  {
    float a=(float)(y*x2-xy*x)/(n*x2-x*x);
    float b=(float)(n*xy-x*y)/(n*x2-x*x);
    *y0=rint(a+b*x0);
    *y1=rint(a+b*x1);
  }
  return(0);
}


/* didn't need in->out seperation, modifies the flr[] vector; takes in
   a dB scale floor, puts out linear */
static int floor1_forward(vorbis_block *vb,vorbis_look_floor *i,
		    float *flr,float *mdct){
  long i,j;
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)i;
  vorbis_info_floor1 *info=look->vi;
  long n=look->n;
  /* first, quantize the floor into the units we're using,
     silmultaneously weeding out values for x positions where the
     residue will likely quantize to zero */
  int workfloor=alloca(sizeof(int)*n);
  memset(workfloor,0,sizeof(int)*n);
  for(i=0;i<n;i++){
    if


  return(val);
}

static int floor1_inverse(vorbis_block *vb,vorbis_look_floor *i,float *out){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)i;
  vorbis_info_floor1 *info=look->vi;


 eop:
  memset(out,0,sizeof(float)*look->n);
  return(0);
}

/* export hooks */
vorbis_func_floor floor1_exportbundle={
  &floor1_pack,&floor1_unpack,&floor1_look,&floor1_copy_info,&floor1_free_info,
  &floor1_free_look,&floor1_forward,&floor1_inverse
};


