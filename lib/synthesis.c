/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: single-block PCM synthesis
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 07 1999

 ********************************************************************/

#include <stdio.h>
#include "codec.h"
#include "envelope.h"
#include "mdct.h"
#include "lpc.h"
#include "lsp.h"
#include "bitwise.h"
#include "spectrum.h"

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  static int frameno=0;
  vorbis_dsp_state *vd=vb->vd;
  vorbis_info      *vi=vd->vi;
  oggpack_buffer   *opb=&vb->opb;
  lpc_lookup       *vl;
  int              spectral_order;
  int              n,i;

  /* first things first.  Make sure decode is ready */
  _oggpack_readinit(opb,op->packet,op->bytes);

  /* Check the packet type */
  if(_oggpack_read(opb,1)!=0){
    /* Oops.  This is not an audio data packet */
    return(-1);
  }

  /* Encode the block size */
  vb->W=_oggpack_read(opb,1);

  /* other random setup */
  vb->frameno=op->frameno;
  vb->eofflag=op->e_o_s;
  vl=&vb->vd->vl[vb->W];
  spectral_order=vi->floororder[vb->W];

  /* The storage vectors are large enough; set the use markers */
  n=vb->pcmend=vi->blocksize[vb->W];
  vb->multend=vb->pcmend/vi->envelopesa;
  
  /* recover the time envelope */
  /*if(_ve_envelope_decode(vb)<0)return(-1);*/

  for(i=0;i<vi->channels;i++){
    double *lpc=vb->lpc[i];
    double *lsp=vb->lsp[i];
    
    /* recover the spectral envelope */
    if(_vs_spectrum_decode(vb,&vb->amp[i],lsp)<0)return(-1);
    
    /* recover the spectral residue */  
    if(_vs_residue_decode(vb,vb->pcm[i])<0)return(-1);

    /* LSP->LPC */
    vorbis_lsp_to_lpc(lsp,lpc,vl->m); 

    /* apply envelope to residue */
    
    vorbis_lpc_apply(vb->pcm[i],lpc,vb->amp[i],vl);
      
    /* MDCT->time */
    mdct_backward(&vb->vd->vm[vb->W],vb->pcm[i],vb->pcm[i]);
    
    /* apply time domain envelope */
    /*_ve_envelope_apply(vb,1);*/
  }
  
  return(0);
}


