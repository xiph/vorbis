/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: channel mapping 0 implementation
 last mod: $Id: mapping0.c,v 1.22 2000/12/21 21:04:39 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "codebook.h"
#include "bitbuffer.h"
#include "registry.h"
#include "psy.h"
#include "misc.h"

/* simplistic, wasteful way of doing this (unique lookup for each
   mode/submapping); there should be a central repository for
   identical lookups.  That will require minor work, so I'm putting it
   off as low priority.

   Why a lookup for each backend in a given mode?  Because the
   blocksize is set by the mode, and low backend lookups may require
   parameters from other areas of the mode/mapping */

typedef struct {
  vorbis_info_mode *mode;
  vorbis_info_mapping0 *map;

  vorbis_look_time **time_look;
  vorbis_look_floor **floor_look;

  vorbis_look_residue **residue_look;
  vorbis_look_psy *psy_look;

  vorbis_func_time **time_func;
  vorbis_func_floor **floor_func;
  vorbis_func_residue **residue_func;

  int ch;
  float **decay;
  long lastframe; /* if a different mode is called, we need to 
		     invalidate decay */
} vorbis_look_mapping0;

static vorbis_info_mapping *mapping0_copy_info(vorbis_info_mapping *vm){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;
  vorbis_info_mapping0 *ret=_ogg_malloc(sizeof(vorbis_info_mapping0));
  memcpy(ret,info,sizeof(vorbis_info_mapping0));
  return(ret);
}

static void mapping0_free_info(vorbis_info_mapping *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_mapping0));
    _ogg_free(i);
  }
}

static void mapping0_free_look(vorbis_look_mapping *look){
  int i;
  vorbis_look_mapping0 *l=(vorbis_look_mapping0 *)look;
  if(l){
    for(i=0;i<l->map->submaps;i++){
      l->time_func[i]->free_look(l->time_look[i]);
      l->floor_func[i]->free_look(l->floor_look[i]);
      l->residue_func[i]->free_look(l->residue_look[i]);
      if(l->psy_look)_vp_psy_clear(l->psy_look+i);
    }

    if(l->decay){
      for(i=0;i<l->ch;i++){
	if(l->decay[i])_ogg_free(l->decay[i]);
      }
      _ogg_free(l->decay);
    }

    _ogg_free(l->time_func);
    _ogg_free(l->floor_func);
    _ogg_free(l->residue_func);
    _ogg_free(l->time_look);
    _ogg_free(l->floor_look);
    _ogg_free(l->residue_look);
    if(l->psy_look)_ogg_free(l->psy_look);
    memset(l,0,sizeof(vorbis_look_mapping0));
    _ogg_free(l);
  }
}

static vorbis_look_mapping *mapping0_look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_mapping *m){
  int i;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  vorbis_look_mapping0 *look=_ogg_calloc(1,sizeof(vorbis_look_mapping0));
  vorbis_info_mapping0 *info=look->map=(vorbis_info_mapping0 *)m;
  look->mode=vm;
  
  look->time_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_time *));
  look->floor_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_floor *));

  look->residue_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_residue *));
  if(ci->psys)look->psy_look=_ogg_calloc(info->submaps,sizeof(vorbis_look_psy));

  look->time_func=_ogg_calloc(info->submaps,sizeof(vorbis_func_time *));
  look->floor_func=_ogg_calloc(info->submaps,sizeof(vorbis_func_floor *));
  look->residue_func=_ogg_calloc(info->submaps,sizeof(vorbis_func_residue *));
  
  for(i=0;i<info->submaps;i++){
    int timenum=info->timesubmap[i];
    int floornum=info->floorsubmap[i];
    int resnum=info->residuesubmap[i];

    look->time_func[i]=_time_P[ci->time_type[timenum]];
    look->time_look[i]=look->time_func[i]->
      look(vd,vm,ci->time_param[timenum]);
    look->floor_func[i]=_floor_P[ci->floor_type[floornum]];
    look->floor_look[i]=look->floor_func[i]->
      look(vd,vm,ci->floor_param[floornum]);
    look->residue_func[i]=_residue_P[ci->residue_type[resnum]];
    look->residue_look[i]=look->residue_func[i]->
      look(vd,vm,ci->residue_param[resnum]);
    
    if(ci->psys && vd->analysisp){
      int psynum=info->psysubmap[i];
      _vp_psy_init(look->psy_look+i,ci->psy_param[psynum],
		   ci->blocksizes[vm->blockflag]/2,vi->rate);
    }
  }

  look->ch=vi->channels;
  if(ci->psys){
    look->decay=_ogg_calloc(vi->channels,sizeof(float *));
    for(i=0;i<vi->channels;i++)
      look->decay[i]=_ogg_calloc(ci->blocksizes[vm->blockflag]/2,sizeof(float));
  }

  return(look);
}

static void mapping0_pack(vorbis_info *vi,vorbis_info_mapping *vm,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;

  oggpack_write(opb,info->submaps-1,4);
  /* we don't write the channel submappings if we only have one... */
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++)
      oggpack_write(opb,info->chmuxlist[i],4);
  }
  for(i=0;i<info->submaps;i++){
    oggpack_write(opb,info->timesubmap[i],8);
    oggpack_write(opb,info->floorsubmap[i],8);
    oggpack_write(opb,info->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
static vorbis_info_mapping *mapping0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=_ogg_calloc(1,sizeof(vorbis_info_mapping0));
  codec_setup_info     *ci=vi->codec_setup;
  memset(info,0,sizeof(vorbis_info_mapping0));

  info->submaps=oggpack_read(opb,4)+1;

  if(info->submaps>1){
    for(i=0;i<vi->channels;i++){
      info->chmuxlist[i]=oggpack_read(opb,4);
      if(info->chmuxlist[i]>=info->submaps)goto err_out;
    }
  }
  for(i=0;i<info->submaps;i++){
    info->timesubmap[i]=oggpack_read(opb,8);
    if(info->timesubmap[i]>=ci->times)goto err_out;
    info->floorsubmap[i]=oggpack_read(opb,8);
    if(info->floorsubmap[i]>=ci->floors)goto err_out;
    info->residuesubmap[i]=oggpack_read(opb,8);
    if(info->residuesubmap[i]>=ci->residues)goto err_out;
  }

  return info;

 err_out:
  mapping0_free_info(info);
  return(NULL);
}

#include "os.h"
#include "lpc.h"
#include "lsp.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"
#include "scales.h"

/* no time mapping implementation for now */
static long seq=0;
static int mapping0_forward(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  backend_lookup_state *b=vb->vd->backend_state;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   n=vb->pcmend;
  int i,j;
  float *window=b->window[vb->W][vb->lW][vb->nW][mode->windowtype];

  float **pcmbundle=alloca(sizeof(float *)*vi->channels);

  int    *nonzero=alloca(sizeof(int)*vi->channels);
  float **floor=_vorbis_block_alloc(vb,vi->channels*sizeof(float *));
  float **nonquant=_vorbis_block_alloc(vb,vi->channels*sizeof(float *));

  vorbis_bitbuffer      vbb_res;
  vorbis_bitbuffer      *vbb_flr=_vorbis_block_alloc(vb,
						     sizeof(vorbis_bitbuffer)*
						     vi->channels);

  for(i=0;i<vi->channels;i++)
    bitbuf_init(vbb_flr+i,vb);
  bitbuf_init(&vbb_res,vb);
  
  /* time domain pre-window: NONE IMPLEMENTED */

  /* window the PCM data: takes PCM vector, vb; modifies PCM vector */

  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    for(j=0;j<n;j++)
      pcm[j]*=window[j];
  }
	    
  /* time-domain post-window: NONE IMPLEMENTED */

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    float *outq=nonquant[i]=_vorbis_block_alloc(vb,n*sizeof(float));
    mdct_forward(b->transform[vb->W][0],pcm,pcm);

    memcpy(pcm+n/2,pcm,n*sizeof(float)/2);
    mdct_backward(b->transform[vb->W][0],pcm,outq);
    for(j=0;j<n;j++)
      outq[j]*=window[j];
    
  }

  {
    for(i=0;i<vi->channels;i++){
      float *pcm=vb->pcm[i];
      float *decay=look->decay[i];
      int submap=info->chmuxlist[i];
      floor[i]=_vorbis_block_alloc(vb,n*sizeof(float)/2);

      /* if some other mode/mapping was called last frame, our decay
         accumulator is out of date.  Clear it. */
      if(look->lastframe+1 != vb->sequence)
	memset(decay,0,n*sizeof(float)/2);

      /* perform psychoacoustics; do masking */
      _vp_compute_mask(look->psy_look+submap,pcm,floor[i],decay);
 
      _analysis_output("decay",seq,decay,n/2,0,1);
      _analysis_output("mdct",seq,pcm,n/2,0,1);
      _analysis_output("lmdct",seq,pcm,n/2,0,0);
      _analysis_output("prefloor",seq,floor[i],n/2,0,1);

      /* perform floor encoding */
      nonzero[i]=look->floor_func[submap]->
	forward(vb,look->floor_look[submap],floor[i],floor[i],vbb_flr+i);

      _analysis_output("floor",seq,floor[i],n/2,0,1);

      /* apply the floor, do optional noise levelling */
      _vp_apply_floor(look->psy_look+submap,pcm,floor[i]);
      
      _analysis_output("res",seq++,pcm,n/2,0,0);
      
#ifdef TRAIN_RES
      if(nonzero[i]){
	FILE *of;
	char buffer[80];
	int i;
	
	sprintf(buffer,"residue_%d.vqd",vb->mode);
	of=fopen(buffer,"a");
	for(i=0;i<n/2;i++)
	  fprintf(of,"%.2f, ",pcm[i]);
	fprintf(of,"\n");
	fclose(of);
      }
#endif      

    }
    
    /* perform residue encoding with residue mapping; this is
       multiplexed.  All the channels belonging to one submap are
       encoded (values interleaved), then the next submap, etc */
    
    for(i=0;i<info->submaps;i++){
      int ch_in_bundle=0;
      for(j=0;j<vi->channels;j++){
	if(info->chmuxlist[j]==i && nonzero[j]>0){
	  pcmbundle[ch_in_bundle++]=vb->pcm[j];
	}
      }
      
      look->residue_func[i]->forward(vb,look->residue_look[i],
				     pcmbundle,ch_in_bundle,&vbb_res);
    }

    /* go back and compute the original MDCT inverse and our quantized
       inverse; we'll want to give the floor mapping a shot at
       massaging things before we write out the final version. */

    {
      for(i=0;i<vi->channels;i++){
	float *pcm=vb->pcm[i];
	float *pcmori=vb->pcm[i]+n/2;
	float *orig=nonquant[i];
	float *quant=floor[i];
	float num=0.f,den=0.f;
	int submap=info->chmuxlist[i];
	
	if(nonzero[i]){

	  for(j=0;j<n/2;j++)
	    pcm[j]=pcmori[j]-pcm[j]*quant[j];
	  
	  mdct_backward(b->transform[vb->W][0],pcm,pcm);
	  for(j=0;j<n;j++)
	    pcm[j]*=window[j];
	  
	  /* weighted compare twixt original and quantized mdct output */
	  for(j=0;j<n;j++){
	    den+=(pcm[j]*pcm[j]);
	    num+=(orig[j]*orig[j]);
	  }
	  
	  num=sqrt(num);
	  den=sqrt(den);

	}

	num=look->floor_func[submap]->
	  forward2(vb,look->floor_look[submap],
		   nonzero[i],den==0.f?0.:num/den,vbb_flr+i);

	/*for(j=0;j<n;j++)
	  pcm[j]*=num;
	_analysis_output("i_orig",seq,orig,n,0,0);
	_analysis_output("i_quant",seq++,pcm,n,0,0);
	*/

      }
    }

    bitbuf_pack(&vb->opb,&vbb_res);

  }

  look->lastframe=vb->sequence;
  return(0);
}

static int mapping0_inverse(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  backend_lookup_state *b=vd->backend_state;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   i,j;
  long                  n=vb->pcmend=ci->blocksizes[vb->W];

  float *window=b->window[vb->W][vb->lW][vb->nW][mode->windowtype];
  float **pcmbundle=alloca(sizeof(float *)*vi->channels);
  int *nonzero=alloca(sizeof(int)*vi->channels);
  
  /* time domain information decode (note that applying the
     information would have to happen later; we'll probably add a
     function entry to the harness for that later */
  /* NOT IMPLEMENTED */

  /* recover the spectral envelope; store it in the PCM vector for now */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    int submap=info->chmuxlist[i];
    nonzero[i]=look->floor_func[submap]->
      inverse(vb,look->floor_look[submap],pcm);
    _analysis_output("ifloor",seq+i,pcm,n/2,0,1);
  }

  /* recover the residue, apply directly to the spectral envelope */

  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(info->chmuxlist[j]==i && nonzero[j])
	pcmbundle[ch_in_bundle++]=vb->pcm[j];
    }

    look->residue_func[i]->inverse(vb,look->residue_look[i],pcmbundle,ch_in_bundle);
  }

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    _analysis_output("out",seq+i,pcm,n/2,0,1);
    mdct_backward(b->transform[vb->W][0],pcm,pcm);
  }

  /* now apply the decoded pre-window time information */
  /* NOT IMPLEMENTED */
  
  /* window the data */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    if(nonzero[i])
      for(j=0;j<n;j++)
	pcm[j]*=window[j];
    else
      for(j=0;j<n;j++)
	pcm[j]=0.f;
    _analysis_output("final",seq++,pcm,n,0,0);
  }
	    
  /* now apply the decoded post-window time information */
  /* NOT IMPLEMENTED */

  /* all done! */
  return(0);
}

/* export hooks */
vorbis_func_mapping mapping0_exportbundle={
  &mapping0_pack,&mapping0_unpack,&mapping0_look,&mapping0_copy_info,
  &mapping0_free_info,&mapping0_free_look,&mapping0_forward,&mapping0_inverse
};



