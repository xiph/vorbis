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

 function: floor backend 0 implementation
 last mod: $Id: floor0.c,v 1.4 2000/01/28 14:31:25 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"

static void free_info(vorbis_info_floor *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_floor0));
    free(i);
  }
}
static void free_look(vorbis_look_floor *i){
}

static void pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor0 *d=(vorbis_info_floor0 *)i;
  int j;
  _oggpack_write(opb,d->order,8);
  _oggpack_write(opb,d->rate,16);
  _oggpack_write(opb,d->barkmap,16);
  _oggpack_write(opb,d->stages,4);
  for(j=0;j<d->stages;j++)
    _oggpack_write(opb,d->books[j],8);
}

static vorbis_info_floor *unpack (vorbis_info *vi,oggpack_buffer *opb){
  int j;
  vorbis_info_floor0 *d=malloc(sizeof(vorbis_info_floor0));
  d->order=_oggpack_read(opb,8);
  d->rate=_oggpack_read(opb,16);
  d->barkmap=_oggpack_read(opb,16);
  d->stages=_oggpack_read(opb,4);
  
  if(d->order<1)goto err_out;
  if(d->rate<1)goto err_out;
  if(d->barkmap<1)goto err_out;
  if(d->stages<1)goto err_out;

  for(j=0;j<d->stages;j++){
    d->books[j]=_oggpack_read(opb,8);
    if(d->books[j]<0 || d->books[j]>=vi->books)goto err_out;
  }
  return(d);  
 err_out:
  free_info(d);
  return(NULL);
}

static vorbis_look_floor *look (vorbis_info *vi,vorbis_info_mode *mi,
                              vorbis_info_floor *i){

}

static int forward(vorbis_block *vb,vorbis_look_floor *i,
		    double *in,double *out){


      /* Convert our floor to a set of lpc coefficients 
      vb->amp[i]=sqrt(vorbis_curve_to_lpc(floor,lpc,vl));

      LSP <-> LPC is orthogonal and LSP quantizes more stably 
      vorbis_lpc_to_lsp(lpc,lsp,vl->m);

      code the spectral envelope; mutates the lsp coeffs to reflect
      what was actually encoded 
      _vs_spectrum_encode(vb,vb->amp[i],lsp);

      Generate residue from the decoded envelope, which will be
         slightly different to the pre-encoding floor due to
         quantization.  Slow, yes, but perhaps more accurate 

      vorbis_lsp_to_lpc(lsp,lpc,vl->m); 
      vorbis_lpc_to_curve(curve,lpc,vb->amp[i],vl);*/
  return(0);
}
static int inverse(vorbis_block *vb,vorbis_look_floor *i,
		    double *buf){
  return(0);
}

/* export hooks */
vorbis_func_floor floor0_exportbundle={
  &pack,&unpack,&look,&free_info,&free_look,&forward,&inverse
};


