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
 last mod: $Id: mapping0.c,v 1.8 2000/02/09 22:04:14 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"
#include "bitwise.h"
#include "bookinternal.h"
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

} vorbis_look_mapping0;

static void free_info(vorbis_info_mapping *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_mapping0));
    free(i);
  }
}

static void free_look(vorbis_look_mapping *look){
  int i;
  vorbis_look_mapping0 *l=(vorbis_look_mapping0 *)look;
  if(l){
    for(i=0;i<l->map->submaps;i++){
      l->time_func[i]->free_look(l->time_look[i]);
      l->floor_func[i]->free_look(l->floor_look[i]);
      l->residue_func[i]->free_look(l->residue_look[i]);
      if(l->psy_look)_vp_psy_clear(l->psy_look+i);
    }
    free(l->time_func);
    free(l->floor_func);
    free(l->residue_func);
    free(l->time_look);
    free(l->floor_look);
    free(l->residue_look);
    if(l->psy_look)free(l->psy_look);
    memset(l,0,sizeof(vorbis_look_mapping0));
    free(l);
  }
}

static vorbis_look_mapping *look(vorbis_info *vi,vorbis_info_mode *vm,
			  vorbis_info_mapping *m){
  int i;
  vorbis_look_mapping0 *ret=calloc(1,sizeof(vorbis_look_mapping0));
  vorbis_info_mapping0 *map=ret->map=(vorbis_info_mapping0 *)m;
  ret->mode=vm;
  
  ret->time_look=calloc(map->submaps,sizeof(vorbis_look_time *));
  ret->floor_look=calloc(map->submaps,sizeof(vorbis_look_floor *));
  ret->residue_look=calloc(map->submaps,sizeof(vorbis_look_residue *));
  if(vi->psys)ret->psy_look=calloc(map->submaps,sizeof(vorbis_look_psy));

  ret->time_func=calloc(map->submaps,sizeof(vorbis_func_time *));
  ret->floor_func=calloc(map->submaps,sizeof(vorbis_func_floor *));
  ret->residue_func=calloc(map->submaps,sizeof(vorbis_func_residue *));
  
  for(i=0;i<map->submaps;i++){
    int timenum=map->timesubmap[i];
    int floornum=map->floorsubmap[i];
    int resnum=map->residuesubmap[i];

    ret->time_func[i]=_time_P[vi->time_type[timenum]];
    ret->time_look[i]=ret->time_func[i]->
      look(vi,vm,vi->time_param[timenum]);
    ret->floor_func[i]=_floor_P[vi->floor_type[floornum]];
    ret->floor_look[i]=ret->floor_func[i]->
      look(vi,vm,vi->floor_param[floornum]);
    ret->residue_func[i]=_residue_P[vi->residue_type[resnum]];
    ret->residue_look[i]=ret->residue_func[i]->
      look(vi,vm,vi->residue_param[resnum]);
    
    if(vi->psys){
      int psynum=map->psysubmap[i];
      _vp_psy_init(ret->psy_look+i,vi->psy_param[psynum],
		   vi->blocksizes[vm->blockflag]/2,vi->rate);
    }
  }

  return(ret);
}

static void pack(vorbis_info *vi,vorbis_info_mapping *vm,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *d=(vorbis_info_mapping0 *)vm;

  _oggpack_write(opb,d->submaps-1,4);
  /* we don't write the channel submappings if we only have one... */
  if(d->submaps>1){
    for(i=0;i<vi->channels;i++)
      _oggpack_write(opb,d->chmuxlist[i],4);
  }
  for(i=0;i<d->submaps;i++){
    _oggpack_write(opb,d->timesubmap[i],8);
    _oggpack_write(opb,d->floorsubmap[i],8);
    _oggpack_write(opb,d->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
static vorbis_info_mapping *unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *d=calloc(1,sizeof(vorbis_info_mapping0));
  memset(d,0,sizeof(vorbis_info_mapping0));

  d->submaps=_oggpack_read(opb,4)+1;

  if(d->submaps>1){
    for(i=0;i<vi->channels;i++){
      d->chmuxlist[i]=_oggpack_read(opb,4);
      if(d->chmuxlist[i]>=d->submaps)goto err_out;
    }
  }
  for(i=0;i<d->submaps;i++){
    d->timesubmap[i]=_oggpack_read(opb,8);
    if(d->timesubmap[i]>=vi->times)goto err_out;
    d->floorsubmap[i]=_oggpack_read(opb,8);
    if(d->floorsubmap[i]>=vi->floors)goto err_out;
    d->residuesubmap[i]=_oggpack_read(opb,8);
    if(d->residuesubmap[i]>=vi->residues)goto err_out;
  }

  return d;

 err_out:
  free_info(d);
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
static int forward(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *map=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   n=vb->pcmend;
  int i,j;
  double *window=vd->window[vb->W][vb->lW][vb->nW][mode->windowtype];

  double **pcmbundle=alloca(sizeof(double *)*vi->channels);
  int **auxbundle=alloca(sizeof(int *)*vi->channels);
  int *nonzero=alloca(sizeof(int)*vi->channels);
  
  /* time domain pre-window: NONE IMPLEMENTED */

  /* window the PCM data: takes PCM vector, vb; modifies PCM vector */

  for(i=0;i<vi->channels;i++){
    double *pcm=vb->pcm[i];
    for(j=0;j<n;j++)
      pcm[j]*=window[j];
  }
	    
  /* time-domain post-window: NONE IMPLEMENTED */

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++){
    double *pcm=vb->pcm[i];
    mdct_forward(vd->transform[vb->W][0],pcm,pcm);
  }

  {
    double *decfloor=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    double *floor=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    double *mask=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    int **pcmaux=_vorbis_block_alloc(vb,vi->channels*sizeof(int *));
    
    for(i=0;i<vi->channels;i++){
      double *pcm=vb->pcm[i];
      int submap=map->chmuxlist[i];

      pcmaux[i]=_vorbis_block_alloc(vb,n/2*sizeof(int));
 
      /* perform psychoacoustics; takes PCM vector; 
	 returns two curves: the desired transform floor and the masking curve */
      memset(floor,0,sizeof(double)*n/2);
      memset(mask,0,sizeof(double)*n/2);
      _vp_mask_floor(look->psy_look+submap,pcm,mask,floor);
 
      /* perform floor encoding; takes transform floor, returns decoded floor */
      nonzero[i]=look->floor_func[submap]->
	forward(vb,look->floor_look[submap],floor,decfloor);
      
      /* no iterative residue/floor tuning at the moment */
      
      /* perform residue prequantization.  Do it now so we have all
         the values if we need them */
      _vp_quantize(look->psy_look+submap,pcm,mask,decfloor,pcmaux[i],0,n/2,n/2);
      
    }
  
    /* perform residue encoding with residue mapping; this is
       multiplexed.  All the channels belonging to one submap are
       encoded (values interleaved), then the next submap, etc */
    
    for(i=0;i<map->submaps;i++){
      int ch_in_bundle=0;
      for(j=0;j<vi->channels;j++){
	if(map->chmuxlist[j]==i && nonzero[j]==1){
	  pcmbundle[ch_in_bundle]=vb->pcm[j];
	  auxbundle[ch_in_bundle++]=pcmaux[j];
	}
      }
      
      look->residue_func[i]->forward(vb,look->residue_look[i],pcmbundle,auxbundle,
				 ch_in_bundle);
    }
  }

  return(0);
}

static int inverse(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *map=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   i,j;
  long                  n=vb->pcmend=vi->blocksizes[vb->W];

  double *window=vd->window[vb->W][vb->lW][vb->nW][mode->windowtype];
  double **pcmbundle=alloca(sizeof(double *)*vi->channels);
  int *nonzero=alloca(sizeof(int)*vi->channels);
  
  /* time domain information decode (note that applying the
     information would have to happen later; we'll probably add a
     function entry to the harness for that later */
  /* NOT IMPLEMENTED */

  /* recover the spectral envelope; store it in the PCM vector for now */
  for(i=0;i<vi->channels;i++){
    double *pcm=vb->pcm[i];
    int submap=map->chmuxlist[i];
    nonzero[i]=look->floor_func[submap]->
      inverse(vb,look->floor_look[submap],pcm);
  }

  /* recover the residue, apply directly to the spectral envelope */

  for(i=0;i<map->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(map->chmuxlist[j]==i && nonzero[j])
	pcmbundle[ch_in_bundle++]=vb->pcm[j];
    }

    look->residue_func[i]->inverse(vb,look->residue_look[i],pcmbundle,ch_in_bundle);
  }

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++){
    double *pcm=vb->pcm[i];
    mdct_backward(vd->transform[vb->W][0],pcm,pcm);
  }

  /* now apply the decoded pre-window time information */
  /* NOT IMPLEMENTED */
  
  /* window the data */
  for(i=0;i<vi->channels;i++){
    double *pcm=vb->pcm[i];
    if(nonzero[i])
      for(j=0;j<n;j++)
	pcm[j]*=window[j];
    else
      for(j=0;j<n;j++)
	pcm[j]=0.;
  }
	    
  /* now apply the decoded post-window time information */
  /* NOT IMPLEMENTED */

  /* all done! */
  return(0);
}

/* export hooks */
vorbis_func_mapping mapping0_exportbundle={
  &pack,&unpack,&look,&free_info,&free_look,&forward,&inverse
};



