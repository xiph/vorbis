/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: floor backend 0 implementation
 last mod: $Id: floor0.c,v 1.46 2001/10/02 00:14:30 segher Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "lpc.h"
#include "lsp.h"
#include "codebook.h"
#include "scales.h"
#include "misc.h"
#include "os.h"

#include "misc.h"
#include <stdio.h>

typedef struct {
  long n;
  int ln;
  int  m;
  int *linearmap;

  vorbis_info_floor0 *vi;
  lpc_lookup lpclook;
  float *lsp_look;

  long bits;
  long frames;
} vorbis_look_floor0;

/* infrastructure for finding fit */
static long _f0_fit(codebook *book,
		    float *orig,
		    float *workfit,
		    int cursor){
  int dim=book->dim;
  float norm,base=0.f;
  int i,best=0;
  float *lsp=workfit+cursor;

  if(cursor)base=workfit[cursor-1];
  norm=orig[cursor+dim-1]-base;

  for(i=0;i<dim;i++)
    lsp[i]=(orig[i+cursor]-base);
  best=_best(book,lsp,1);

  memcpy(lsp,book->valuelist+best*dim,dim*sizeof(*lsp));
  for(i=0;i<dim;i++)
    lsp[i]+=base;
  return(best);
}

/***********************************************/

static vorbis_info_floor *floor0_copy_info (vorbis_info_floor *i){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_info_floor0 *ret=_ogg_malloc(sizeof(*ret));
  memcpy(ret,info,sizeof(*ret));
  return(ret);
}

static void floor0_free_info(vorbis_info_floor *i){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  if(info){
    memset(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

static void floor0_free_look(vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  if(look){

    /*fprintf(stderr,"floor 0 bit usage %f\n",
      (float)look->bits/look->frames);*/

    if(look->linearmap)_ogg_free(look->linearmap);
    if(look->lsp_look)_ogg_free(look->lsp_look);
    lpc_clear(&look->lpclook);
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

static void floor0_pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  int j;
  oggpack_write(opb,info->order,8);
  oggpack_write(opb,info->rate,16);
  oggpack_write(opb,info->barkmap,16);
  oggpack_write(opb,info->ampbits,6);
  oggpack_write(opb,info->ampdB,8);
  oggpack_write(opb,info->numbooks-1,4);
  for(j=0;j<info->numbooks;j++)
    oggpack_write(opb,info->books[j],8);
}

static vorbis_info_floor *floor0_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=vi->codec_setup;
  int j;

  vorbis_info_floor0 *info=_ogg_malloc(sizeof(*info));
  info->order=oggpack_read(opb,8);
  info->rate=oggpack_read(opb,16);
  info->barkmap=oggpack_read(opb,16);
  info->ampbits=oggpack_read(opb,6);
  info->ampdB=oggpack_read(opb,8);
  info->numbooks=oggpack_read(opb,4)+1;
  
  if(info->order<1)goto err_out;
  if(info->rate<1)goto err_out;
  if(info->barkmap<1)goto err_out;
  if(info->numbooks<1)goto err_out;
    
  for(j=0;j<info->numbooks;j++){
    info->books[j]=oggpack_read(opb,8);
    if(info->books[j]<0 || info->books[j]>=ci->books)goto err_out;
  }
  return(info);

 err_out:
  floor0_free_info(info);
  return(NULL);
}

/* initialize Bark scale and normalization lookups.  We could do this
   with static tables, but Vorbis allows a number of possible
   combinations, so it's best to do it computationally.

   The below is authoritative in terms of defining scale mapping.
   Note that the scale depends on the sampling rate as well as the
   linear block and mapping sizes */

static vorbis_look_floor *floor0_look (vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *i){
  int j;
  float scale;
  vorbis_info        *vi=vd->vi;
  codec_setup_info   *ci=vi->codec_setup;
  vorbis_info_floor0 *info=(vorbis_info_floor0 *)i;
  vorbis_look_floor0 *look=_ogg_calloc(1,sizeof(*look));
  look->m=info->order;
  look->n=ci->blocksizes[mi->blockflag]/2;
  look->ln=info->barkmap;
  look->vi=info;

  if(vd->analysisp)
    lpc_init(&look->lpclook,look->ln,look->m);

  /* we choose a scaling constant so that:
     floor(bark(rate/2-1)*C)=mapped-1
     floor(bark(rate/2)*C)=mapped */
  scale=look->ln/toBARK(info->rate/2.f);

  /* the mapping from a linear scale to a smaller bark scale is
     straightforward.  We do *not* make sure that the linear mapping
     does not skip bark-scale bins; the decoder simply skips them and
     the encoder may do what it wishes in filling them.  They're
     necessary in some mapping combinations to keep the scale spacing
     accurate */
  look->linearmap=_ogg_malloc((look->n+1)*sizeof(*look->linearmap));
  for(j=0;j<look->n;j++){
    int val=floor( toBARK((info->rate/2.f)/look->n*j) 
		   *scale); /* bark numbers represent band edges */
    if(val>=look->ln)val=look->ln; /* guard against the approximation */
    look->linearmap[j]=val;
  }
  look->linearmap[j]=-1;

  look->lsp_look=_ogg_malloc(look->ln*sizeof(*look->lsp_look));
  for(j=0;j<look->ln;j++)
    look->lsp_look[j]=2*cos(M_PI/look->ln*j);

  return look;
}

/* less efficient than the decode side (written for clarity).  We're
   not bottlenecked here anyway */

float _curve_to_lpc(float *curve,float *lpc,
		     vorbis_look_floor0 *l){
  /* map the input curve to a bark-scale curve for encoding */
  
  int mapped=l->ln;
  float *work=alloca(sizeof(*work)*mapped);
  int i,j,last=0;
  int bark=0;
  static int seq=0;

  memset(work,0,sizeof(*work)*mapped);
  
  /* Only the decode side is behavior-specced; for now in the encoder,
     we select the maximum value of each band as representative (this
     helps make sure peaks don't go out of range.  In error terms,
     selecting min would make more sense, but the codebook is trained
     numerically, so we don't actually lose.  We'd still want to
     use the original curve for error and noise estimation */
  
  for(i=0;i<l->n;i++){
    bark=l->linearmap[i];
    if(work[bark]<curve[i])work[bark]=curve[i];
    if(bark>last+1){
      /* If the bark scale is climbing rapidly, some bins may end up
         going unused.  This isn't a waste actually; it keeps the
         scale resolution even so that the LPC generator has an easy
         time.  However, if we leave the bins empty we lose energy.
         So, fill 'em in.  The decoder does not do anything with  he
         unused bins, so we can fill them anyway we like to end up
         with a better spectral curve */

      /* we'll always have a bin zero, so we don't need to guard init */
      long span=bark-last;
      for(j=1;j<span;j++){
	float del=(float)j/span;
	work[j+last]=work[bark]*del+work[last]*(1.f-del);
      }
    }
    last=bark;
  }

  /* If we're over-ranged to avoid edge effects, fill in the end of spectrum gap */
  for(i=bark+1;i<mapped;i++)
    work[i]=work[i-1];


  /**********************/

  for(i=0;i<l->n;i++)
    curve[i]-=150;

  _analysis_output("barkfloor",seq,work,bark,0,0);
  _analysis_output("barkcurve",seq++,curve,l->n,1,0);

  for(i=0;i<l->n;i++)
    curve[i]+=150;

  /**********************/
  
  return vorbis_lpc_from_curve(work,lpc,&(l->lpclook));
}

static int floor0_forward(vorbis_block *vb,vorbis_look_floor *in,
			  float *mdct, const float *logmdct,   /* in */
			  const float *logmask, const float *logmax, /* in */
			  float *codedflr){          /* out */
  long j;
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)in;
  vorbis_info_floor0 *info=look->vi;
  float amp;
  long bits=0;
  long val=0;
  static int seq=0;

#ifdef TRAIN_LSP
  FILE *of;
  FILE *ef;
  char buffer[80];

#if 1
  sprintf(buffer,"lsp0coeff_%d.vqd",vb->mode);
  of=fopen(buffer,"a");
#endif
#endif

  seq++;


  /* our floor comes in on a [-Inf...0] dB scale.  The curve has to be
     positive, so we offset it. */

  for(j=0;j<look->n;j++)
    codedflr[j]=logmask[j]+info->ampdB;

  /* use 'out' as temp storage */
  /* Convert our floor to a set of lpc coefficients */ 
  amp=sqrt(_curve_to_lpc(codedflr,codedflr,look));

  /* amp is in the range (0. to ampdB].  Encode that range using
     ampbits bits */
 
  {
    long maxval=(1L<<info->ampbits)-1;
    
    val=rint(amp/info->ampdB*maxval);

    if(val<0)val=0;           /* likely */
    if(val>maxval)val=maxval; /* not bloody likely */

    if(val>0)
      amp=(float)val/maxval*info->ampdB;
    else
      amp=0;
  }

  if(val){
    /* LSP <-> LPC is orthogonal and LSP quantizes more stably  */
    _analysis_output("lpc",seq-1,codedflr,look->m,0,0);
    if(vorbis_lpc_to_lsp(codedflr,codedflr,look->m))
      val=0;

  }

  oggpack_write(&vb->opb,val,info->ampbits);
  look->bits+=info->ampbits+1;
  look->frames++;

  if(val){
    float *lspwork=alloca(look->m*sizeof(*lspwork));

    /* the spec supports using one of a number of codebooks.  Right
       now, encode using this lib supports only one */
    backend_lookup_state *be=vb->vd->backend_state;
    codebook *b;
    int booknum;

    _analysis_output("lsp",seq-1,codedflr,look->m,0,0);

    /* which codebook to use? We do it only by range right now. */
    if(info->numbooks>1){
      float last=0.;
      for(j=0;j<look->m;j++){
	float val=codedflr[j]-last;
	if(val<info->lessthan || val>info->greaterthan)break;
	last=codedflr[j];
      }
      if(j<look->m)
	booknum=0;
      else
	booknum=1;
    }else
      booknum=0;

    b=be->fullbooks+info->books[booknum];
    oggpack_write(&vb->opb,booknum,_ilog(info->numbooks));
    look->bits+=_ilog(info->numbooks);

#ifdef TRAIN_LSP
    {
      float last=0.f;
      for(j=0;j<look->m;j++){
	fprintf(of,"%.12g, ",codedflr[j]-last);
	last=codedflr[j];
      }
    }
    fprintf(of,"\n");
    fclose(of);

    sprintf(buffer,"lsp0ent_m%d_b%d.vqd",vb->mode,booknum);
    ef=fopen(buffer,"a");

#endif

    /* code the spectral envelope, and keep track of the actual
       quantized values; we don't want creeping error as each block is
       nailed to the last quantized value of the previous block. */

    for(j=0;j<look->m;j+=b->dim){
      int entry=_f0_fit(b,codedflr,lspwork,j);
      look->bits+=vorbis_book_encode(b,entry,&vb->opb);

#ifdef TRAIN_LSP
      fprintf(ef,"%d,\n",entry);
#endif

    }

#ifdef TRAIN_LSP
    fclose(ef);
#endif

    _analysis_output("lsp2",seq-1,lspwork,look->m,0,0);

    /* take the coefficients back to a spectral envelope curve */
    for(j=0;j<look->n;j++)
      codedflr[j]=1.f;
    vorbis_lsp_to_curve(codedflr,look->linearmap,look->n,look->ln,
			lspwork,look->m,amp,info->ampdB);

    _analysis_output("barklsp",seq-1,codedflr,look->n,1,1);
    _analysis_output("lsp3",seq-1,codedflr,look->n,0,1);

    return(val);
  }

#ifdef TRAIN_LSP
    fclose(of);
#endif

  memset(codedflr,0,sizeof(*codedflr)*look->n);
  memset(mdct,0,sizeof(*mdct)*look->n);
  return(val);
}

static void *floor0_inverse1(vorbis_block *vb,vorbis_look_floor *i){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  int j,k;
  
  int ampraw=oggpack_read(&vb->opb,info->ampbits);
  if(ampraw>0){ /* also handles the -1 out of data case */
    long maxval=(1<<info->ampbits)-1;
    float amp=(float)ampraw/maxval*info->ampdB;
    int booknum=oggpack_read(&vb->opb,_ilog(info->numbooks));
    
    if(booknum!=-1 && booknum<info->numbooks){ /* be paranoid */
      backend_lookup_state *be=vb->vd->backend_state;
      codebook *b=be->fullbooks+info->books[booknum];
      float last=0.f;
      float *lsp=_vorbis_block_alloc(vb,sizeof(*lsp)*(look->m+1));
            
      for(j=0;j<look->m;j+=b->dim)
	if(vorbis_book_decodev_set(b,lsp+j,&vb->opb,b->dim)==-1)goto eop;
      for(j=0;j<look->m;){
	for(k=0;k<b->dim;k++,j++)lsp[j]+=last;
	last=lsp[j-1];
      }
      
      lsp[look->m]=amp;
      return(lsp);
    }
  }
 eop:
  return(NULL);
}

static int floor0_inverse2(vorbis_block *vb,vorbis_look_floor *i,
			   void *memo,float *out){
  vorbis_look_floor0 *look=(vorbis_look_floor0 *)i;
  vorbis_info_floor0 *info=look->vi;
  
  if(memo){
    float *lsp=(float *)memo;
    float amp=lsp[look->m];

    /* take the coefficients back to a spectral envelope curve */
    vorbis_lsp_to_curve(out,look->linearmap,look->n,look->ln,
			lsp,look->m,amp,info->ampdB);
    return(1);
  }
  memset(out,0,sizeof(*out)*look->n);
  return(0);
}

/* export hooks */
vorbis_func_floor floor0_exportbundle={
  &floor0_pack,&floor0_unpack,&floor0_look,&floor0_copy_info,&floor0_free_info,
  &floor0_free_look,&floor0_forward,&floor0_inverse1,&floor0_inverse2
};


