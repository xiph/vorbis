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

 function: single-block PCM synthesis
 last mod: $Id: synthesis.c,v 1.11 2000/01/05 03:11:04 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include "vorbis/codec.h"

#include "envelope.h"
#include "mdct.h"
#include "lpc.h"
#include "lsp.h"
#include "bitwise.h"
#include "spectrum.h"

int vorbis_synthesis(vorbis_block *vb,ogg_packet *op){
  double           *window;
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

  /* Decode the block size */
  vb->W=_oggpack_read(opb,1);
  if(vb->W){
    vb->lW=_oggpack_read(opb,1);
    vb->nW=_oggpack_read(opb,1);
  }else{
    vb->lW=0;
    vb->nW=0;
  }

  window=vb->vd->window[vb->W][vb->lW][vb->nW];

  /* other random setup */
  vb->frameno=op->frameno;
  vb->sequence=op->packetno-3; /* first block is third packet */

  vb->eofflag=op->e_o_s;
  vl=&vb->vd->vl[vb->W];
  spectral_order=vi->floororder[vb->W];

  /* The storage vectors are large enough; set the use markers */
  n=vb->pcmend=vi->blocksize[vb->W];
  
  /* No envelope encoding yet */
  _oggpack_write(opb,0,1);

  for(i=0;i<vi->channels;i++){
    double *lpc=vb->lpc[i];
    double *lsp=vb->lsp[i];
    
    /* recover the spectral envelope */
    if(_vs_spectrum_decode(vb,&vb->amp[i],lsp)<0)return(-1);
    
    /* recover the spectral residue */  
    if(_vs_residue_decode(vb,vb->pcm[i])<0)return(-1);

#ifdef ANALYSIS
    {
      int j;
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"Sres%d.m",vb->sequence);
      out=fopen(buffer,"w+");
      for(j=0;j<n/2;j++)
	fprintf(out,"%g\n",vb->pcm[i][j]);
      fclose(out);
    }
#endif

    /* LSP->LPC */
    vorbis_lsp_to_lpc(lsp,lpc,vl->m); 

    /* apply envelope to residue */
    
#ifdef ANALYSIS
    {
      int j;
      FILE *out;
      char buffer[80];
      double curve[n/2];
      vorbis_lpc_to_curve(curve,lpc,vb->amp[i],vl);
      
      
      sprintf(buffer,"Smask%d.m",vb->sequence);
      out=fopen(buffer,"w+");
      for(j=0;j<n/2;j++)
	fprintf(out,"%g\n",curve[j]);
      fclose(out);

      sprintf(buffer,"Slsp%d.m",vb->sequence);
      out=fopen(buffer,"w+");
      for(j=0;j<vl->m;j++)
	fprintf(out,"%g\n",lsp[j]);
      fclose(out);

      sprintf(buffer,"Slpc%d.m",vb->sequence);
      out=fopen(buffer,"w+");
      for(j=0;j<vl->m;j++)
	fprintf(out,"%g\n",lpc[j]);
      fclose(out);
    }
#endif

    vorbis_lpc_apply(vb->pcm[i],lpc,vb->amp[i],vl);

#ifdef ANALYSIS
    {
      int j;
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"Sspectrum%d.m",vb->sequence);
      out=fopen(buffer,"w+");
      for(j=0;j<n/2;j++)
	fprintf(out,"%g\n",vb->pcm[i][j]);
      fclose(out);
    }
#endif
      

    /* MDCT->time */
    mdct_backward(&vb->vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);
    
  }
  return(0);
}


