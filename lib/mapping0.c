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

 function: channel mapping 0 implementation
 last mod: $Id: mapping0.c,v 1.1 2000/01/20 04:43:02 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"
#include "registry.h"
#include "mapping0.h"

extern _vi_info_map *_vorbis_map0_dup(vi_info *vi,_vi_info_mapping *source){
  vorbis_info_mapping0 *d=malloc(sizeof(vorbis_info_mapping0));
  vorbis_info_mapping0 *s=(vorbis_info_mapping0 *)source;
  memcpy(d,s,sizeof(vorbis_info_mapping0));

  d->floorsubmap=malloc(sizeof(int)*vi->channels);
  d->residuesubmap=malloc(sizeof(int)*vi->channels);
  d->psysubmap=malloc(sizeof(int)*vi->channels);
  memcpy(d->floorsubmap,s->floorsubmap,sizeof(int)*vi->channels);
  memcpy(d->residuesubmap,s->residuesubmap,sizeof(int)*vi->channels);
  memcpy(d->psysubmap,s->psysubmap,sizeof(int)*vi->channels);

  return(d);
}

extern void _vorbis_map0_free(_vi_info_mapping *i){
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)i;

  if(d){
    if(d->floorsubmap)free(d->floorsubmap);
    if(d->residuesubmap)free(d->residuesubmap);
    if(d->psysubmap)free(d->psysubmap);
    memset(d,0,sizeof(vorbis_info_mapping0));
    free(d);
  }
}

extern void _vorbis_map0_pack(vorbis_info *vi,oggpack_buffer *opb,
			      _vi_info_map *i){
  int i;
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)i;

  _oggpack_write(opb,d->timesubmap,8);

  /* we abbreviate the channel submappings if they all map to the same
     floor/res/psy */
  for(i=1;i<vi->channels;i++)if(d->floorsubmap[i]!=d->floorsubmap[i-1])break;
  if(i==vi->channels){
    _oggpack_write(opb,0,1);
    _oggpack_write(opb,d->floorsubmap[0],8);
  }else{
    _oggpack_write(opb,1,1);
    for(i=0;i<vi->channels;i++)
      _oggpack_write(opb,d->floorsubmap[i],8);
  }

  for(i=1;i<vi->channels;i++)
    if(d->residuesubmap[i]!=d->residuesubmap[i-1])break;
  if(i==vi->channels){
    /* no channel submapping */
    _oggpack_write(opb,0,1);
    _oggpack_write(opb,d->residuesubmap[0],8);
  }else{
    /* channel submapping */
    _oggpack_write(opb,1,1);
    for(i=0;i<vi->channels;i++)
      _oggpack_write(opb,d->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
extern _vi_info_map *_vorbis_map0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 d=calloc(1,sizeof(vorbis_info_mapping0));
  memset(d,0,sizeof(vorbis_info_mapping0));

  d->timesubmap=_oggpack_read(opb,8);
  if(d->timesubmap>=vi->times)goto err_out;

  d->floorsubmap=malloc(sizeof(int)*vi->channels);
  if(_oggpack_read(opb,1)){
    /* channel submap */
    for(i=0;i<vi->channels;i++)
      d->floorsubmap[i]=_oggpack_read(opb,8);
  }else{
    int temp=_oggpack_read(opb,8);
    for(i=0;i<vi->channels;i++)
      d->floorsubmap[i]=temp;
  }

  d->residuesubmap=malloc(sizeof(int)*vi->channels);
  if(_oggpack_read(opb,1)){
    /* channel submap */
    for(i=0;i<vi->channels;i++)
      d->residuesubmap[i]=_oggpack_read(opb,8);
  }else{
    int temp=_oggpack_read(opb,8);
    for(i=0;i<vi->channels;i++)
      d->residuesubmap[i]=temp;
  }

  for(i=0;i<vi->channels;i++){
    if(d->floorsubmap[i]<0 || d->floorsubmap[i]>=vi->floors)goto err_out;
    if(d->residuesubmap[i]<0 || d->residuesubmap[i]>=vi->residuess)
      goto err_out;
  }

  return d;

 err_out:
  _vorbis_map0_free(d);
  return(NULL);
}

#include <stdio.h>
#include "os.h"
#include "lpc.h"
#include "lsp.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"
#include "bitwise.h"
#include "spectrum.h"

/* no time mapping implementation for now */
int *_vorbis_map0_analysis(vorbis_block *vb,vorbis_info_map *i,
			   ogg_packet *opb){
  int i;
  vorbis_dsp_state     *vd=vb->vd;
  double               *window=vd->window[vb->W][vb->lW][vb->nW];
  oggpack_buffer       *opb=&vb->opb;
  vorbis_info_mapping0 *vi=i;
  int                   n=vb->pcmend;

  /* time domain pre-window: NONE IMPLEMENTED */

  /* window the PCM data: takes PCM vector, vb; modifies PCM vector */

  /* time-domain post-window: NONE IMPLEMENTED */

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */

  /* perform psychoacoustics; takes PCM vector; returns transform floor 
     and resolution floor, modifies PCM vector */

  /* perform floor encoding; takes transform floor, returns decoded floor*/

  /* perform residue encoding with residue mapping */



  psy_lookup       *vp=&vb->vd->vp[vb->W];
  lpc_lookup       *vl=&vb->vd->vl[vb->W];


  vb->gluebits=0;
  vb->time_envelope_bits=0;
  vb->spectral_envelope_bits=0;
  vb->spectral_residue_bits=0;

  /*lpc_lookup       *vbal=&vb->vd->vbal[vb->W];
    double balance_v[vbal->m];
    double balance_amp;*/

  /* first things first.  Make sure encode is ready*/
  _oggpack_reset(opb);
  /* Encode the packet type */
  _oggpack_write(opb,0,1);

  /* Encode the block size */
  _oggpack_write(opb,vb->W,1);
  if(vb->W){
    _oggpack_write(opb,vb->lW,1);
    _oggpack_write(opb,vb->nW,1);
  }

  /* No envelope encoding yet */
  _oggpack_write(opb,0,1);
  
  /* time domain PCM -> MDCT domain */
  for(i=0;i<vi->channels;i++)
    mdct_forward(&vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);

  /* no balance yet */
    
  /* extract the spectral envelope and residue */
  /* just do by channel.  No coupling yet */
  {
    for(i=0;i<vi->channels;i++){
      static int frameno=0;
      int j;
      double *floor=alloca(n/2*sizeof(double));
      double *curve=alloca(n/2*sizeof(double));
      double *lpc=vb->lpc[i];
      double *lsp=vb->lsp[i];

      memset(floor,0,sizeof(double)*n/2);
      
#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"Aspectrum%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",vb->pcm[i][j]);
	fclose(out);

      }
#endif

      _vp_mask_floor(vp,vb->pcm[i],floor);

#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"Apremask%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",floor[j]);
	fclose(out);
      }
#endif

      /* Convert our floor to a set of lpc coefficients */
      vb->amp[i]=sqrt(vorbis_curve_to_lpc(floor,lpc,vl));

      /* LSP <-> LPC is orthogonal and LSP quantizes more stably */
      vorbis_lpc_to_lsp(lpc,lsp,vl->m);

      /* code the spectral envelope; mutates the lsp coeffs to reflect
         what was actually encoded */
      _vs_spectrum_encode(vb,vb->amp[i],lsp);

      /* Generate residue from the decoded envelope, which will be
         slightly different to the pre-encoding floor due to
         quantization.  Slow, yes, but perhaps more accurate */

      vorbis_lsp_to_lpc(lsp,lpc,vl->m); 
      vorbis_lpc_to_curve(curve,lpc,vb->amp[i],vl);

      /* this may do various interesting massaging too...*/
      if(vb->amp[i])_vs_residue_train(vb,vb->pcm[i],curve,n/2);
      _vs_residue_quantize(vb->pcm[i],curve,vi,n/2);

#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"Alpc%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<vl->m;j++)
	  fprintf(out,"%g\n",lpc[j]);
	fclose(out);

	sprintf(buffer,"Alsp%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<vl->m;j++)
	  fprintf(out,"%g\n",lsp[j]);
	fclose(out);

	sprintf(buffer,"Amask%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",curve[j]);
	fclose(out);

	sprintf(buffer,"Ares%d.m",vb->sequence);
	out=fopen(buffer,"w+");
	for(j=0;j<n/2;j++)
	  fprintf(out,"%g\n",vb->pcm[i][j]);
	fclose(out);
      }
#endif

      /* encode the residue */
      _vs_residue_encode(vb,vb->pcm[i]);

    }
  }

  /* set up the packet wrapper */

  op->packet=opb->buffer;
  op->bytes=_oggpack_bytes(opb);
  op->b_o_s=0;
  op->e_o_s=vb->eofflag;
  op->frameno=vb->frameno;
  op->packetno=vb->sequence; /* for sake of completeness */

  return(0);
}




/* commented out, relocated balance stuff */
  /*{
    double *C=vb->pcm[0];
    double *D=vb->pcm[1];
    
    balance_amp=_vp_balance_compute(D,C,balance_v,vbal);
    
    {
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"com%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
        fprintf(out," 0. 0.\n");
	fprintf(out,"%g %g\n",C[i],D[i]);
	fprintf(out,"\n");
      }
      fclose(out);
      
      sprintf(buffer,"L%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",C[i]);
      }
      fclose(out);
      sprintf(buffer,"R%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",D[i]);
      }
      fclose(out);
      
    }
    
    _vp_balance_apply(D,C,balance_v,balance_amp,vbal,1);
      
    {
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"bal%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out," 0. 0.\n");
	fprintf(out,"%g %g\n",C[i],D[i]);
	fprintf(out,"\n");
      }
      fclose(out);
      sprintf(buffer,"C%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",C[i]);
      }
      fclose(out);
      sprintf(buffer,"D%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",D[i]);
      }
      fclose(out);
      
    }
  }*/


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
  _oggpack_read(opb,0,1);

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


