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

 function: single-block PCM analysis mode dispatch
 last mod: $Id: analysis.c,v 1.43 2001/02/26 03:50:41 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "registry.h"
#include "scales.h"
#include "os.h"

/* decides between modes, dispatches to the appropriate mapping. */
int vorbis_analysis(vorbis_block *vb,ogg_packet *op){
  vorbis_dsp_state     *vd=vb->vd;
  backend_lookup_state *b=vd->backend_state;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  int                   type,ret;
  int                   mode=0;

  vb->glue_bits=0;
  vb->time_bits=0;
  vb->floor_bits=0;
  vb->res_bits=0;

  /* first things first.  Make sure encode is ready */
  oggpack_reset(&vb->opb);
  /* Encode the packet type */
  oggpack_write(&vb->opb,0,1);

  /* currently lazy.  Short block dispatches to 0, long to 1. */

  if(vb->W &&ci->modes>1)mode=1;
  type=ci->map_type[ci->mode_param[mode]->mapping];
  vb->mode=mode;

  /* Encode frame mode, pre,post windowsize, then dispatch */
  oggpack_write(&vb->opb,mode,b->modebits);
  if(vb->W){
    oggpack_write(&vb->opb,vb->lW,1);
    oggpack_write(&vb->opb,vb->nW,1);
    /*fprintf(stderr,"*");
  }else{
  fprintf(stderr,".");*/
  }

  if((ret=_mapping_P[type]->forward(vb,b->mode[mode])))
    return(ret);
  
  /* set up the packet wrapper */
  
  op->packet=oggpack_get_buffer(&vb->opb);
  op->bytes=oggpack_bytes(&vb->opb);
  op->b_o_s=0;
  op->e_o_s=vb->eofflag;
  op->granulepos=vb->granulepos;
  op->packetno=vb->sequence; /* for sake of completeness */
  
  return(0);
}

/* there was no great place to put this.... */
void _analysis_output(char *base,int i,float *v,int n,int bark,int dB){
#ifdef ANALYSIS
  int j;
  FILE *of;
  char buffer[80];
  sprintf(buffer,"%s_%d.m",base,i);
  of=fopen(buffer,"w");

  if(!of)perror("failed to open data dump file");

  for(j=0;j<n;j++){
    if(dB && v[j]==0)
          fprintf(of,"\n\n");
    else{
      if(bark)
	fprintf(of,"%g ",toBARK(22050.f*j/n));
      else
	fprintf(of,"%g ",(double)j);
      
      if(dB){
	fprintf(of,"%g\n",todB(fabs(v[j])));
      }else{
	fprintf(of,"%g\n",v[j]);
      }
    }
  }
  fclose(of);
#endif
}
