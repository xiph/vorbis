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
 last mod: $Id: floor0.c,v 1.12 2000/03/10 13:21:18 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "registry.h"
#include "lpc.h"
#include "lsp.h"
#include "bookinternal.h"
#include "scales.h"
#include "misc.h"

typedef struct {
  long n;
  long m;
  
  double ampscale;
  double ampvals;

  vorbis_info_floor0 *vi;
  lpc_lookup lpclook;
} vorbis_look_floor0;

static void free_info(vorbis_info_floor *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_floor0));
    free(i);
  }
}

static void free_look(vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  if(i){
    lpc_clear(&look->lpclook);
    memset(look,0,sizeof(vorbis_look_floor0));
    free(look);
  }
}

static void pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  int j;
  _oggpack_write(opb,info->order,8);
  _oggpack_write(opb,info->rate,16);
  _oggpack_write(opb,info->barkmap,16);
  _oggpack_write(opb,info->ampbits,6);
  _oggpack_write(opb,info->ampdB,8);
  _oggpack_write(opb,info->stages-1,4);
  for(j=0;j<info->stages;j++)
    _oggpack_write(opb,info->books[j],8);
}

static vorbis_info_floor *unpack (vorbis_info *vi,oggpack_buffer *opb){
  int j;
  vorbis_info_floor0 *info=malloc(sizeof(vorbis_info_floor0));
  info->order=_oggpack_read(opb,8);
  info->rate=_oggpack_read(opb,16);
  info->barkmap=_oggpack_read(opb,16);
  info->ampbits=_oggpack_read(opb,6);
  info->ampdB=_oggpack_read(opb,8);
  info->stages=_oggpack_read(opb,4)+1;
  
  if(info->order<1)goto err_out;
  if(info->rate<1)goto err_out;
  if(info->barkmap<1)goto err_out;
  if(info->stages<1)goto err_out;

  for(j=0;j<info->stages;j++){
    info->books[j]=_oggpack_read(opb,8);
    if(info->books[j]<0 || info->books[j]>=vi->books)goto err_out;
  }
  return(info);  
 err_out:
  free_info(info);
  return(NULL);
}

static vorbis_look_floor *look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *i){
  vorbis_info        *vi=vd->vi;
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_look_floor0 *look=malloc(sizeof(vorbis_look_floor0));
  look->m=info->order;
  look->n=vi->blocksizes[mi->blockflag]/2;
  look->vi=info;
  lpc_init(&look->lpclook,look->n,info->barkmap,info->rate,look->m);

  return look;
}

#include <stdio.h>

static int forward(vorbis_block *vb,vorbis_look_floor *i,
		    double *in,double *out){
  long j,k,stage;
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  double amp;
  long bits=0;

  /* use 'out' as temp storage */
  /* Convert our floor to a set of lpc coefficients */ 
  amp=sqrt(vorbis_curve_to_lpc(in,out,&look->lpclook));

  /* amp is in the range 0. to 1. (well, more like .7). Log scale it */
  
  /* 0              == 0 dB
     (1<<ampbits)-1 == amp dB   = 1. amp */
  {
    long ampscale=fromdB(info->ampdB);
    long maxval=(1<<info->ampbits)-1;

    long val=todB(amp*ampscale)/info->ampdB*maxval+1;

    if(val<0)val=0;           /* likely */
    if(val>maxval)val=maxval; /* not bloody likely */

    _oggpack_write(&vb->opb,val,info->ampbits);
    if(val>0)
      amp=fromdB((val-.5)/maxval*info->ampdB)/ampscale;
    else
      amp=0;
  }

  if(amp>0){
    double *work=alloca(sizeof(double)*look->m);
    
    /* LSP <-> LPC is orthogonal and LSP quantizes more stably  */
    vorbis_lpc_to_lsp(out,out,look->m);
    memcpy(work,out,sizeof(double)*look->m);

#ifdef TRAIN
    {
      int j;
      FILE *of;
      char buffer[80];
      sprintf(buffer,"lsp0coeff_%d.vqd",vb->mode);
      of=fopen(buffer,"a");
      for(j=0;j<look->m;j++)
	fprintf(of,"%g, ",out[j]);
      fprintf(of,"\n");
      fclose(of);
    }
#endif

    /* code the spectral envelope, and keep track of the actual
       quantized values; we don't want creeping error as each block is
       nailed to the last quantized value of the previous block. */
    
    /* first stage is a bit different because quantization error must be
       handled carefully */
    for(stage=0;stage<info->stages;stage++){
      codebook *b=vb->vd->fullbooks+info->books[stage];
      
      if(stage==0){
	double last=0.;
	for(j=0;j<look->m;){
	  for(k=0;k<b->dim;k++)out[j+k]-=last;
	  bits+=vorbis_book_encodev(b,out+j,&vb->opb);
	  for(k=0;k<b->dim;k++,j++){
	    out[j]+=last;
	    work[j]-=out[j];
	  }
	  last=out[j-1];
	}
      }else{
	memcpy(out,work,sizeof(double)*look->m);
	for(j=0;j<look->m;){
	  bits+=vorbis_book_encodev(b,out+j,&vb->opb);
	  for(k=0;k<b->dim;k++,j++)work[j]-=out[j];
	}
      }
    }
    /* take the coefficients back to a spectral envelope curve */
    vorbis_lsp_to_lpc(out,out,look->m); 
    vorbis_lpc_to_curve(out,out,amp,&look->lpclook);
    fprintf(stderr,"Encoded %ld LSP coefficients in %ld bits\n",look->m,bits);
    return(1);
  }

  fprintf(stderr,"Encoded %ld LSP coefficients in %ld bits\n",look->m,bits);

  memset(out,0,sizeof(double)*look->n);
  return(0);
}

static int inverse(vorbis_block *vb,vorbis_look_floor *i,double *out){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  int j,k,stage;
  
  long ampraw=_oggpack_read(&vb->opb,info->ampbits);
  if(ampraw>0){
    long ampscale=fromdB(info->ampdB);
    long maxval=(1<<info->ampbits)-1;
    double amp=fromdB((ampraw-.5)/maxval*info->ampdB)/ampscale;

    memset(out,0,sizeof(double)*look->m);    
    for(stage=0;stage<info->stages;stage++){
      codebook *b=vb->vd->fullbooks+info->books[stage];
      for(j=0;j<look->m;j+=b->dim)
	vorbis_book_decodev(b,out+j,&vb->opb);
      if(stage==0){
	double last=0.;
	for(j=0;j<look->m;){
	  for(k=0;k<b->dim;k++,j++)out[j]+=last;
	  last=out[j-1];
	}
      }
    }
  

    /* take the coefficients back to a spectral envelope curve */
    vorbis_lsp_to_lpc(out,out,look->m); 
    vorbis_lpc_to_curve(out,out,amp,&look->lpclook);
    return(1);
  }else
    memset(out,0,sizeof(double)*look->n);
  return(0);
}

/* export hooks */
vorbis_func_floor floor0_exportbundle={
  &pack,&unpack,&look,&free_info,&free_look,&forward,&inverse
};


