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
 last mod: $Id: mapping0.c,v 1.11.2.2.2.7 2000/05/08 08:25:43 xiphmont Exp $

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

  int ch;
  double **decay;
  long lastframe; /* if a different mode is called, we need to 
		     invalidate decay */
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
    if(l->decay){
      for(i=0;i<l->ch;i++){
	if(l->decay[i])free(l->decay[i]);
      }
      free(l->decay);
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

static vorbis_look_mapping *look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_mapping *m){
  int i;
  vorbis_info *vi=vd->vi;
  vorbis_look_mapping0 *look=calloc(1,sizeof(vorbis_look_mapping0));
  vorbis_info_mapping0 *info=look->map=(vorbis_info_mapping0 *)m;
  look->mode=vm;
  
  look->time_look=calloc(info->submaps,sizeof(vorbis_look_time *));
  look->floor_look=calloc(info->submaps,sizeof(vorbis_look_floor *));
  look->residue_look=calloc(info->submaps,sizeof(vorbis_look_residue *));
  if(vi->psys)look->psy_look=calloc(info->submaps,sizeof(vorbis_look_psy));

  look->time_func=calloc(info->submaps,sizeof(vorbis_func_time *));
  look->floor_func=calloc(info->submaps,sizeof(vorbis_func_floor *));
  look->residue_func=calloc(info->submaps,sizeof(vorbis_func_residue *));
  
  for(i=0;i<info->submaps;i++){
    int timenum=info->timesubmap[i];
    int floornum=info->floorsubmap[i];
    int resnum=info->residuesubmap[i];

    look->time_func[i]=_time_P[vi->time_type[timenum]];
    look->time_look[i]=look->time_func[i]->
      look(vd,vm,vi->time_param[timenum]);
    look->floor_func[i]=_floor_P[vi->floor_type[floornum]];
    look->floor_look[i]=look->floor_func[i]->
      look(vd,vm,vi->floor_param[floornum]);
    look->residue_func[i]=_residue_P[vi->residue_type[resnum]];
    look->residue_look[i]=look->residue_func[i]->
      look(vd,vm,vi->residue_param[resnum]);
    
    if(vi->psys){
      int psynum=info->psysubmap[i];
      _vp_psy_init(look->psy_look+i,vi->psy_param[psynum],
		   vi->blocksizes[vm->blockflag]/2,vi->rate);
    }
  }

  look->ch=vi->channels;
  if(vi->psys){
    look->decay=calloc(vi->channels,sizeof(double *));
    for(i=0;i<vi->channels;i++)
      look->decay[i]=calloc(vi->blocksizes[vm->blockflag]/2,sizeof(double));
  }

  return(look);
}

static void pack(vorbis_info *vi,vorbis_info_mapping *vm,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;

  _oggpack_write(opb,info->submaps-1,4);
  /* we don't write the channel submappings if we only have one... */
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++)
      _oggpack_write(opb,info->chmuxlist[i],4);
  }
  for(i=0;i<info->submaps;i++){
    _oggpack_write(opb,info->timesubmap[i],8);
    _oggpack_write(opb,info->floorsubmap[i],8);
    _oggpack_write(opb,info->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
static vorbis_info_mapping *unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=calloc(1,sizeof(vorbis_info_mapping0));
  memset(info,0,sizeof(vorbis_info_mapping0));

  info->submaps=_oggpack_read(opb,4)+1;

  if(info->submaps>1){
    for(i=0;i<vi->channels;i++){
      info->chmuxlist[i]=_oggpack_read(opb,4);
      if(info->chmuxlist[i]>=info->submaps)goto err_out;
    }
  }
  for(i=0;i<info->submaps;i++){
    info->timesubmap[i]=_oggpack_read(opb,8);
    if(info->timesubmap[i]>=vi->times)goto err_out;
    info->floorsubmap[i]=_oggpack_read(opb,8);
    if(info->floorsubmap[i]>=vi->floors)goto err_out;
    info->residuesubmap[i]=_oggpack_read(opb,8);
    if(info->residuesubmap[i]>=vi->residues)goto err_out;
  }

  return info;

 err_out:
  free_info(info);
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
#include "scales.h"

/* no time mapping implementation for now */
static long seq=0;
static int forward(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;
  vorbis_info_mode     *mode=look->mode;
  int                   n=vb->pcmend;
  int i,j;
  double *window=vd->window[vb->W][vb->lW][vb->nW][mode->windowtype];

  double **pcmbundle=alloca(sizeof(double *)*vi->channels);
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
    double *floor=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    double *mask=_vorbis_block_alloc(vb,n*sizeof(double)/2);
    
    for(i=0;i<vi->channels;i++){
      double *pcm=vb->pcm[i];
      double *decay=look->decay[i];
      int submap=info->chmuxlist[i];

      /* if some other mode/mapping was called last frame, our decay
         accumulator is out of date.  Clear it. */
      if(look->lastframe+1 != vb->sequence)
	memset(decay,0,n*sizeof(double)/2);

      /* perform psychoacoustics; do masking */
      _vp_compute_mask(look->psy_look+submap,pcm,floor,mask,decay);
 
      _analysis_output("mdct",seq,pcm,n/2,0,1);
      _analysis_output("lmdct",seq,pcm,n/2,0,0);
      _analysis_output("prefloor",seq,floor,n/2,0,1);

      /* perform floor encoding */
      nonzero[i]=look->floor_func[submap]->
	forward(vb,look->floor_look[submap],floor,floor);

      _analysis_output("floor",seq,floor,n/2,0,1);

      /* apply the floor, do optional noise levelling */
      _vp_apply_floor(look->psy_look+submap,pcm,floor,mask);
      
      _analysis_output("res",seq++,pcm,n/2,0,0);
      
#ifdef TRAIN
      if(nonzero[i]){
	FILE *of;
	char buffer[80];
	int i;
	
	sprintf(buffer,"residue_%d.vqd",vb->mode);
	of=fopen(buffer,"a");
	for(i=0;i<n/2;i++)
	  fprintf(of,"%g, ",pcm[i]);
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
	if(info->chmuxlist[j]==i && nonzero[j]==1){
	  pcmbundle[ch_in_bundle++]=vb->pcm[j];
	}
      }
      
      look->residue_func[i]->forward(vb,look->residue_look[i],
				     pcmbundle,ch_in_bundle);
    }
  }

  look->lastframe=vb->sequence;
  return(0);
}

static int inverse(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state     *vd=vb->vd;
  vorbis_info          *vi=vd->vi;
  vorbis_look_mapping0 *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0 *info=look->map;
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
    double *pcm=vb->pcm[i];
    _analysis_output("out",seq++,pcm,n/2,0,0);
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



