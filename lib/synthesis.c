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
 last modification date: Aug 08 1999

 ********************************************************************/

#include "codec.h"
#include "envelope.h"
#include "mdct.h"
#include "lpc.h"
#include "lsp.h"

int vorbis_synthesis(vorbis_block *vb){
  int i;
  vorbis_info *vi=&vb->vd->vi;
  double *window=vb->vd->window[vb->W][vb->lW][vb->nW];
  lpc_lookup *vl=&vb->vd->vl[vb->W];

  /* get the LSP params back to LPC params. This will be for each
     spectral floor curve */

  /*for(i=0;i<vi->floorch;i++)
    vorbis_lsp_to_lpc(vb->lsp[i],vb->lpc[i],vi->floororder);*/

  /* Map the resulting floors back to the appropriate channels */

  /*for(i=0;i<vi->channels;i++)
    vorbis_lpc_apply(vb->pcm[i],vb->pcmend,vb->lpc[vi->floormap[i]],
    vb->amp[vi->floormap[i]],vi->floororder);*/
  
  for(i=0;i<vb->pcm_channels;i++)
    mdct_backward(&vb->vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);
  _ve_envelope_apply(vb,1);

}


