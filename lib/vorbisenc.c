/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: simple programmatic interface for encoder mode setup
 last mod: $Id: vorbisenc.c,v 1.39 2002/03/24 21:04:01 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#include "vorbis/codec.h"
#include "vorbis/vorbisenc.h"

#include "codec_internal.h"
#include "registry-api.h"

#include "os.h"
#include "misc.h"

/* careful with this; it's using static array sizing to make managing
   all the modes a little less annoying.  If we use a residue backend
   with > 10 partition types, or a different division of iteration,
   this needs to be updated. */
typedef struct {
  vorbis_info_residue0 *res[2];
  static_codebook *book_aux[2];
  static_codebook *books_base[5][10][3];
  static_codebook *books_stereo_backfill[5][10];
  static_codebook *books_residue_backfill[5][10][2];
} vorbis_residue_template;

static double stereo_threshholds[]={0.0, 2.5, 4.5, 8.5, 16.5};

typedef struct vp_adjblock{
  int block[P_BANDS][P_LEVELS];
} vp_adjblock;

#include "modes/residue_44.h"
#include "modes/psych_44.h"
#include "modes/floor_44.h"

/* a few static coder conventions */
static vorbis_info_time0 _time_dummy={0};
static vorbis_info_mode _mode_set_short={0,0,0,0};
static vorbis_info_mode _mode_set_long={1,0,0,1};

/* mapping conventions:
   only one submap (this would change for efficient 5.1 support for example)*/
/* Four psychoacoustic profiles are used, one for each blocktype */
static vorbis_info_mapping0 _mapping_set_short={
  1, {0,0}, {0}, {0}, {0}, {0,1}, 0,{0},{0}};
static vorbis_info_mapping0 _mapping_set_long={
  1, {0,0}, {0}, {1}, {1}, {2,3}, 0,{0},{0}};

static int vorbis_encode_toplevel_setup(vorbis_info *vi,int small,int large,int ch,long rate){
  if(vi && vi->codec_setup){
    codec_setup_info *ci=vi->codec_setup;

    vi->version=0;
    vi->channels=ch;
    vi->rate=rate;
    
    ci->blocksizes[0]=small;
    ci->blocksizes[1]=large;

    /* time mapping hooks are unused in vorbis I */
    ci->times=1;
    ci->time_type[0]=0;
    ci->time_param[0]=_ogg_calloc(1,sizeof(_time_dummy));
    memcpy(ci->time_param[0],&_time_dummy,sizeof(_time_dummy));

    /* by convention, two modes: one for short, one for long blocks.
       short block mode uses mapping sero, long block uses mapping 1 */
    ci->modes=2;
    ci->mode_param[0]=_ogg_calloc(1,sizeof(_mode_set_short));
    memcpy(ci->mode_param[0],&_mode_set_short,sizeof(_mode_set_short));
    ci->mode_param[1]=_ogg_calloc(1,sizeof(_mode_set_long));
    memcpy(ci->mode_param[1],&_mode_set_long,sizeof(_mode_set_long));

    /* by convention two mappings, both mapping type zero (polyphonic
       PCM), first for short, second for long blocks */
    ci->maps=2;
    ci->map_type[0]=0;
    ci->map_param[0]=_ogg_calloc(1,sizeof(_mapping_set_short));
    memcpy(ci->map_param[0],&_mapping_set_short,sizeof(_mapping_set_short));
    ci->map_type[1]=0;
    ci->map_param[1]=_ogg_calloc(1,sizeof(_mapping_set_long));
    memcpy(ci->map_param[1],&_mapping_set_long,sizeof(_mapping_set_long));

    return(0);
  }
  return(OV_EINVAL);
}

static int vorbis_encode_floor_setup(vorbis_info *vi,double q,int block,
				    static_codebook    ***books, 
				    vorbis_info_floor1 *in, 
				    ...){
  int x[11],i,k,iq=rint(q*10);
  vorbis_info_floor1 *f=_ogg_calloc(1,sizeof(*f));
  codec_setup_info *ci=vi->codec_setup;
  va_list ap;

  va_start(ap,in);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,int);
  va_end(ap);

  memcpy(f,in+x[iq],sizeof(*f));
  /* fill in the lowpass field, even if it's temporary */
  f->n=ci->blocksizes[block]>>1;

  /* books */
  {
    int partitions=f->partitions;
    int maxclass=-1;
    int maxbook=-1;
    for(i=0;i<partitions;i++)
      if(f->partitionclass[i]>maxclass)maxclass=f->partitionclass[i];
    for(i=0;i<=maxclass;i++){
      if(f->class_book[i]>maxbook)maxbook=f->class_book[i];
      f->class_book[i]+=ci->books;
      for(k=0;k<(1<<f->class_subs[i]);k++){
	if(f->class_subbook[i][k]>maxbook)maxbook=f->class_subbook[i][k];
	if(f->class_subbook[i][k]>=0)f->class_subbook[i][k]+=ci->books;
      }
    }

    for(i=0;i<=maxbook;i++)
      ci->book_param[ci->books++]=books[x[iq]][i];
  }

  /* for now, we're only using floor 1 */
  ci->floor_type[ci->floors]=1;
  ci->floor_param[ci->floors]=f;
  ci->floors++;

  return(0);
}

static int vorbis_encode_global_psych_setup(vorbis_info *vi,double q,
					   vorbis_info_psy_global *in, ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *g=&ci->psy_g_param;
  va_list ap;
  
  va_start(ap,in);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,double);
  va_end(ap);

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  memcpy(g,in+(int)x[iq],sizeof(*g));

  dq=x[iq]*(1.-dq)+x[iq+1]*dq;
  iq=(int)dq;
  dq-=iq;
  if(dq==0 && iq>0){
    iq--;
    dq=1.;
  }

  /* interpolate the trigger threshholds */
  for(i=0;i<4;i++){
    g->preecho_thresh[i]=in[iq].preecho_thresh[i]*(1.-dq)+in[iq+1].preecho_thresh[i]*dq;
    g->postecho_thresh[i]=in[iq].postecho_thresh[i]*(1.-dq)+in[iq+1].postecho_thresh[i]*dq;
  }
  g->ampmax_att_per_sec=ci->hi.amplitude_track_dBpersec;
  return(0);
}

static int vorbis_encode_psyset_setup(vorbis_info *vi,int block){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  if(block>=ci->psys)
    ci->psys=block+1;
  if(!p){
    p=_ogg_calloc(1,sizeof(*p));
    ci->psy_param[block]=p;
  }

  memcpy(p,&_psy_info_template,sizeof(*p));

  return 0;
}

static int vorbis_encode_tonemask_setup(vorbis_info *vi,double q,int block,
				       double *att,
				       double *max,
				       int *peaklimit_bands,
				       vp_adjblock *in){
  int i,j,iq;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  iq=q*10;
  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  p->tone_masteratt=att[iq]*(1.-dq)+att[iq+1]*dq;
  p->max_curve_dB=max[iq]*(1.-dq)+max[iq+1]*dq;
  p->curvelimitp=peaklimit_bands[iq];

  iq=q*5.;
  if(iq==5){
    iq=5;
    dq=1.;
  }else{
    dq=q*5.-iq;
  }
  
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->toneatt.block[i][j]=(j<4?4:j)*-10.+
	in[iq].block[i][j]*(1.-dq)+in[iq+1].block[i][j]*dq;
  return(0);
}


static int vorbis_encode_compand_setup(vorbis_info *vi,double q,int block,
				      float in[][NOISE_COMPAND_LEVELS], ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];
  va_list ap;
  
  va_start(ap,in);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,double);
  va_end(ap);

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  dq=x[iq]*(1.-dq)+x[iq+1]*dq;
  iq=(int)dq;
  dq-=iq;
  if(dq==0 && iq>0){
    iq--;
    dq=1.;
  }

  /* interpolate the compander settings */
  for(i=0;i<NOISE_COMPAND_LEVELS;i++)
    p->noisecompand[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
  return(0);
}

static int vorbis_encode_peak_setup(vorbis_info *vi,double q,int block,
				    double *guard,
				    double *suppress,
				    vp_adjblock *in){
  int i,j,iq;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  iq=q*10;
  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  p->peakattp=1;
  p->tone_guard=guard[iq]*(1.-dq)+guard[iq+1]*dq;
  p->tone_abs_limit=suppress[iq]*(1.-dq)+suppress[iq+1]*dq;

  iq=q*5.;
  if(iq==5){
    iq=5;
    dq=1.;
  }else{
    dq=q*5.-iq;
  }

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->peakatt.block[i][j]=(j<4?4:j)*-10.+
	in[iq].block[i][j]*(1.-dq)+in[iq+1].block[i][j]*dq;
  return(0);
}

static int vorbis_encode_noisebias_setup(vorbis_info *vi,double q,int block,
					 double *suppress,
					 int in[][17],int guard[33]){
  int i,iq=q*10;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  p->noisemaxsupp=suppress[iq]*(1.-dq)+suppress[iq+1]*dq;
  p->noisewindowlomin=guard[iq*3];
  p->noisewindowhimin=guard[iq*3+1];
  p->noisewindowfixed=guard[iq*3+2];
  
  for(i=0;i<P_BANDS;i++)
    p->noiseoff[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
  return(0);
}

static int vorbis_encode_ath_setup(vorbis_info *vi,double q,int block,
				   float in[][27], ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];
  va_list ap;
  
  va_start(ap,in);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,double);
  va_end(ap);

  p->ath_adjatt=ci->hi.ath_floating_dB;
  p->ath_maxatt=ci->hi.ath_absolute_dB;

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  dq=x[iq]*(1.-dq)+x[iq+1]*dq;
  iq=(int)dq;
  dq-=iq;
  if(dq==0 && iq>0){
    iq--;
    dq=1.;
  }

  for(i=0;i<27;i++)
    p->ath[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
  return(0);
}


static int book_dup_or_new(codec_setup_info *ci,static_codebook *book){
  int i;
  for(i=0;i<ci->books;i++)
    if(ci->book_param[i]==book)return(i);
  
  return(ci->books++);
}

static int vorbis_encode_residue_setup(vorbis_info *vi,double q,int block,
				       int coupled_p,
				       int stereo_backfill_p,
				       int residue_backfill_p,
				       vorbis_residue_template *in,
				       int point_dB,
				       double point_kHz){

  int i,iq=q*10;
  int n,k;
  int partition_position=0;
  int res_position=0;
  int iterations=1;
  int amplitude_select=0;

  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_residue0 *r;
  vorbis_info_psy *psy=ci->psy_param[block*2];
  
  /* may be re-called due to ctl */
  if(ci->residue_param[block])
    /* free preexisting instance */
    residue_free_info(ci->residue_param[block],ci->residue_type[block]);

  r=ci->residue_param[block]=_ogg_malloc(sizeof(*r));
  memcpy(r,in[iq].res[block],sizeof(*r));
  if(ci->residues<=block)ci->residues=block+1;

  if(block){
    r->grouping=32;
  }else{
    r->grouping=16;
  }

  /* for uncoupled, we use type 1, else type 2 */
  if(coupled_p){
    ci->residue_type[block]=2;
  }else{
    ci->residue_type[block]=1;
  }

  switch(ci->residue_type[block]){
  case 1:
    n=r->end=ci->blocksizes[block?1:0]>>1; /* to be adjusted by lowpass later */
    partition_position=rint(point_kHz*1000./(vi->rate/2)*n/r->grouping);
    res_position=partition_position*r->grouping;
    break;
  case 2:
    n=r->end=(ci->blocksizes[block?1:0]>>1)*vi->channels; /* to be adjusted by lowpass later */
    partition_position=rint(point_kHz*1000./(vi->rate/2)*n/r->grouping);
    res_position=partition_position*r->grouping/vi->channels;
    break;
  }

  for(i=0;i<r->partitions;i++)
    if(r->blimit[i]<0)r->blimit[i]=partition_position;

  for(i=0;i<r->partitions;i++)
    for(k=0;k<3;k++)
      if(in[iq].books_base[point_dB][i][k])
	r->secondstages[i]|=(1<<k);
  
  ci->passlimit[0]=3;
    
  if(coupled_p){
    vorbis_info_mapping0 *map=ci->map_param[block];

    map->coupling_steps=1;
    map->coupling_mag[0]=0;
    map->coupling_ang[0]=1;

    psy->couple_pass[0].granulem=1.;
    psy->couple_pass[0].igranulem=1.;

    psy->couple_pass[0].couple_pass[0].limit=res_position;
    psy->couple_pass[0].couple_pass[0].outofphase_redundant_flip_p=1;
    psy->couple_pass[0].couple_pass[0].outofphase_requant_limit=9e10;
    psy->couple_pass[0].couple_pass[0].amppost_point=0;
    psy->couple_pass[0].couple_pass[1].limit=9999;
    psy->couple_pass[0].couple_pass[1].outofphase_redundant_flip_p=1;
    psy->couple_pass[0].couple_pass[1].outofphase_requant_limit=9e10;
    psy->couple_pass[0].couple_pass[1].amppost_point=
      stereo_threshholds[point_dB];
    amplitude_select=point_dB;

    if(stereo_backfill_p && amplitude_select){
      memcpy(psy->couple_pass+iterations,psy->couple_pass+iterations-1,
	     sizeof(*psy->couple_pass));
      psy->couple_pass[1].couple_pass[1].amppost_point=stereo_threshholds[amplitude_select-1];
      ci->passlimit[1]=4;
      for(i=0;i<r->partitions;i++)
	if(in[iq].books_stereo_backfill[amplitude_select][i])
	  r->secondstages[i]|=8;
      amplitude_select=amplitude_select-1;
      iterations++;
    }
    
    if(residue_backfill_p){
      memcpy(psy->couple_pass+iterations,psy->couple_pass+iterations-1,
	     sizeof(*psy->couple_pass));
      psy->couple_pass[iterations].granulem=.333333333;
      psy->couple_pass[iterations].igranulem=3.;
      psy->couple_pass[iterations].couple_pass[0].outofphase_requant_limit=1.;
      psy->couple_pass[iterations].couple_pass[1].outofphase_requant_limit=1.;
      for(i=0;i<r->partitions;i++)
	if(in[iq].books_residue_backfill[amplitude_select][i][0])
	  r->secondstages[i]|=(1<<(iterations+2));
      ci->passlimit[iterations]=ci->passlimit[iterations-1]+1;
      iterations++;
      
      memcpy(psy->couple_pass+iterations,psy->couple_pass+iterations-1,
	     sizeof(*psy->couple_pass));
      psy->couple_pass[iterations].granulem=.1111111111;
      psy->couple_pass[iterations].igranulem=9.;
      psy->couple_pass[iterations].couple_pass[0].outofphase_requant_limit=.3;
      psy->couple_pass[iterations].couple_pass[1].outofphase_requant_limit=.3;
      for(i=0;i<r->partitions;i++)
	if(in[iq].books_residue_backfill[amplitude_select][i][1])
	  r->secondstages[i]|=(1<<(iterations+2));
      ci->passlimit[iterations]=ci->passlimit[iterations-1]+1;
      iterations++;
    }
    ci->coupling_passes=iterations;

  }else{

    if(residue_backfill_p){
      for(i=0;i<r->partitions;i++){
	if(in[iq].books_residue_backfill[0][i][0])
	  r->secondstages[i]|=8;
	if(in[iq].books_residue_backfill[0][i][1])
	  r->secondstages[i]|=16;
      }
      ci->passlimit[1]=4;
      ci->passlimit[2]=5;
      ci->coupling_passes=3;
    }else
      ci->coupling_passes=1;
  }
  
  memcpy(&ci->psy_param[block*2+1]->couple_pass,
	 &ci->psy_param[block*2]->couple_pass,
	 sizeof(psy->couple_pass));
  
  /* fill in all the books */
  {
    int booklist=0,k;
    r->groupbook=ci->books;
    ci->book_param[ci->books++]=in[iq].book_aux[block];
    for(i=0;i<r->partitions;i++){
      for(k=0;k<3;k++){
	if(in[iq].books_base[point_dB][i][k]){
	  int bookid=book_dup_or_new(ci,in[iq].books_base[point_dB][i][k]);
	  r->booklist[booklist++]=bookid;
	  ci->book_param[bookid]=in[iq].books_base[point_dB][i][k];
	}
      }
      if(coupled_p && stereo_backfill_p && point_dB &&
	 in[iq].books_stereo_backfill[point_dB][i]){
	int bookid=book_dup_or_new(ci,in[iq].books_stereo_backfill[point_dB][i]);
	r->booklist[booklist++]=bookid;
	ci->book_param[bookid]=in[iq].books_stereo_backfill[point_dB][i];
      }
      if(residue_backfill_p){
	for(k=0;k<2;k++){
	  if(in[iq].books_residue_backfill[amplitude_select][i][k]){
	    int bookid=book_dup_or_new(ci,in[iq].books_residue_backfill[amplitude_select][i][k]);
	    r->booklist[booklist++]=bookid;
	    ci->book_param[bookid]=in[iq].books_residue_backfill[amplitude_select][i][k];
	  }
	}
      }
    }
  }

  return(0);
}      

static int vorbis_encode_lowpass_setup(vorbis_info *vi,double q,int block){
  int iq=q*10;
  double dq;
  double freq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_floor1 *f=ci->floor_param[block];
  vorbis_info_residue0 *r=ci->residue_param[block];
  int blocksize=ci->blocksizes[block]>>1;
  double nyq=vi->rate/2.;

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }
  
  freq=ci->hi.lowpass_kHz[block]*1000.;
  if(freq>vi->rate/2)freq=vi->rate/2;
  /* lowpass needs to be set in the floor and the residue. */

  /* in the floor, the granularity can be very fine; it doesn't alter
     the encoding structure, only the samples used to fit the floor
     approximation */
  f->n=freq/nyq*blocksize;

  /* in the residue, we're constrained, physically, by partition
     boundaries.  We still lowpass 'wherever', but we have to round up
     here to next boundary, or the vorbis spec will round it *down* to
     previous boundary in encode/decode */
  if(ci->residue_type[block]==2)
    r->end=(int)((freq/nyq*blocksize*2)/r->grouping+.9)* /* round up only if we're well past */
      r->grouping;
  else
    r->end=(int)((freq/nyq*blocksize)/r->grouping+.9)* /* round up only if we're well past */
      r->grouping;
  return(0);
}

/* encoders will need to use vorbis_info_init beforehand and call
   vorbis_info clear when all done */

/* two interfaces; this, more detailed one, and later a convenience
   layer on top */

/* the final setup call */
int vorbis_encode_setup_init(vorbis_info *vi){
  int ret=0;
  /*long rate=vi->rate;*/
  long channels=vi->channels;
  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;

  ret|=vorbis_encode_floor_setup(vi,hi->base_quality_short,0,
				_floor_44_128_books,_floor_44_128,
				0,1,1,2,2,2,2,2,2,2,2);
  ret|=vorbis_encode_floor_setup(vi,hi->base_quality_long,1,
				_floor_44_1024_books,_floor_44_1024,
				0,0,0,0,0,0,0,0,0,0,0);
  
  ret|=vorbis_encode_global_psych_setup(vi,hi->trigger_quality,_psy_global_44,
				       0., 1., 1.5, 2., 2., 2.5, 3., 3.5, 4., 4., 4.);

  ret|=vorbis_encode_psyset_setup(vi,0);
  ret|=vorbis_encode_psyset_setup(vi,1);
  ret|=vorbis_encode_psyset_setup(vi,2);
  ret|=vorbis_encode_psyset_setup(vi,3);
  
  ret|=vorbis_encode_tonemask_setup(vi,hi->blocktype[0].tone_mask_quality,0,
				    _psy_tone_masteratt,_psy_tone_0dB,_psy_ehmer_bandlimit,
				    _vp_tonemask_adj_otherblock);
  ret|=vorbis_encode_tonemask_setup(vi,hi->blocktype[1].tone_mask_quality,1,
				    _psy_tone_masteratt,_psy_tone_0dB,_psy_ehmer_bandlimit,
				    _vp_tonemask_adj_otherblock);
  ret|=vorbis_encode_tonemask_setup(vi,hi->blocktype[2].tone_mask_quality,2,
				    _psy_tone_masteratt,_psy_tone_0dB,_psy_ehmer_bandlimit,
				    _vp_tonemask_adj_otherblock);
  ret|=vorbis_encode_tonemask_setup(vi,hi->blocktype[3].tone_mask_quality,3,
				    _psy_tone_masteratt,_psy_tone_0dB,_psy_ehmer_bandlimit,
				    _vp_tonemask_adj_longblock);
  
  ret|=vorbis_encode_compand_setup(vi,hi->blocktype[0].noise_compand_quality,
				  0,_psy_compand_44_short,
				  1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
  ret|=vorbis_encode_compand_setup(vi,hi->blocktype[1].noise_compand_quality,
				  1,_psy_compand_44_short,
				  1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
  ret|=vorbis_encode_compand_setup(vi,hi->blocktype[2].noise_compand_quality,
				  2,_psy_compand_44,
				  1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
  ret|=vorbis_encode_compand_setup(vi,hi->blocktype[3].noise_compand_quality,
				  3,_psy_compand_44,
				  1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
  ret|=vorbis_encode_peak_setup(vi,hi->blocktype[0].tone_peaklimit_quality,
				0,_psy_tone_masterguard,_psy_tone_suppress,
				_vp_peakguard);
  ret|=vorbis_encode_peak_setup(vi,hi->blocktype[1].tone_peaklimit_quality,
				1,_psy_tone_masterguard,_psy_tone_suppress,
				_vp_peakguard);
  ret|=vorbis_encode_peak_setup(vi,hi->blocktype[2].tone_peaklimit_quality,
				2,_psy_tone_masterguard,_psy_tone_suppress,
				_vp_peakguard);
  ret|=vorbis_encode_peak_setup(vi,hi->blocktype[3].tone_peaklimit_quality,
				3,_psy_tone_masterguard,_psy_tone_suppress,
				_vp_peakguard);

  if(hi->impulse_block_p){
    ret|=vorbis_encode_noisebias_setup(vi,hi->blocktype[0].noise_bias_quality,
				       0,_psy_noise_suppress,_psy_noisebias_impulse,
				       _psy_noiseguards_short);
  }else{
    ret|=vorbis_encode_noisebias_setup(vi,hi->blocktype[0].noise_bias_quality,
				       0,_psy_noise_suppress,_psy_noisebias_other,
				       _psy_noiseguards_short);
  }

  ret|=vorbis_encode_noisebias_setup(vi,hi->blocktype[1].noise_bias_quality,
				    1,_psy_noise_suppress,_psy_noisebias_other,
				      _psy_noiseguards_short);
  ret|=vorbis_encode_noisebias_setup(vi,hi->blocktype[2].noise_bias_quality,
				      2,_psy_noise_suppress,_psy_noisebias_other,
				    _psy_noiseguards_long);
  ret|=vorbis_encode_noisebias_setup(vi,hi->blocktype[3].noise_bias_quality,
				    3,_psy_noise_suppress,_psy_noisebias_long,
				    _psy_noiseguards_long);

  ret|=vorbis_encode_ath_setup(vi,hi->blocktype[0].ath_quality,0,ATH_Bark_dB,
			      0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);
  ret|=vorbis_encode_ath_setup(vi,hi->blocktype[1].ath_quality,1,ATH_Bark_dB,
			      0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);
  ret|=vorbis_encode_ath_setup(vi,hi->blocktype[2].ath_quality,2,ATH_Bark_dB,
			      0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);
  ret|=vorbis_encode_ath_setup(vi,hi->blocktype[3].ath_quality,3,ATH_Bark_dB,
			      0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);

  if(ret){
    vorbis_info_clear(vi);
    return ret; 
  }

  if(channels==2 && hi->stereo_couple_p){
    /* setup specific to stereo coupling */
    
    ret|=vorbis_encode_residue_setup(vi,hi->base_quality_short,0,
				    1, /* coupled */
				    hi->stereo_backfill_p,
				    hi->residue_backfill_p, 
				    _residue_template_44_stereo,
				    hi->stereo_point_dB,
				    hi->stereo_point_kHz[0]);
      
    ret|=vorbis_encode_residue_setup(vi,hi->base_quality_long,1,
				    1, /* coupled */
				    hi->stereo_backfill_p,
				    hi->residue_backfill_p, 
				    _residue_template_44_stereo,
				    hi->stereo_point_dB,
				    hi->stereo_point_kHz[1]);

  }else{
    /* setup specific to non-stereo (mono or uncoupled polyphonic)
       coupling */
    ret|=vorbis_encode_residue_setup(vi,hi->base_quality_short,0,
				    0, /* uncoupled */
				    0,
				    hi->residue_backfill_p, 
				    _residue_template_44_uncoupled,
				    0,
				    hi->stereo_point_kHz[0]); /* just
				    used as an encoding partitioning
				    point */
      
    ret|=vorbis_encode_residue_setup(vi,hi->base_quality_long,1,
				    0, /* uncoupled */
				    0,
				    hi->residue_backfill_p, 
				    _residue_template_44_uncoupled,
				    0,
				    hi->stereo_point_kHz[1]); /* just
				    used as an encoding partitioning
				    point */
  }
  ret|=vorbis_encode_lowpass_setup(vi,hi->lowpass_kHz[0],0);
  ret|=vorbis_encode_lowpass_setup(vi,hi->lowpass_kHz[1],1);
    
  if(ret)
    vorbis_info_clear(vi);
  return(ret);

}

/* this is only tuned for 44.1kHz right now.  S'ok, for other rates it
   just doesn't guess */
static double ratepch_un44[11]=
    {40000.,50000.,60000.,70000.,75000.,85000.,105000.,
     115000.,135000.,160000.,250000.};
static double ratepch_st44[11]=
    {32000.,40000.,48000.,56000.,64000.,
     80000.,96000.,112000.,128000.,160000.,250000.};

static double vbr_to_approx_bitrate(int ch,int coupled,
				    double q,long srate){
  int iq=q*10.;
  double dq;
  double *r=NULL;

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  if(srate>42000 && srate<46000){
    if(coupled)
      r=ratepch_st44;
    else
      r=ratepch_un44;
  }
  
  if(r==NULL)
    return(-1);
  
  return((r[iq]*(1.-dq)+r[iq+1]*dq)*ch);  
}

static double approx_bitrate_to_vbr(int ch,int coupled,
				    double bitrate,long srate){
  double *r=NULL,del;
  int i;

  if(srate>42000 && srate<46000){
    if(coupled)
      r=ratepch_st44;
    else
      r=ratepch_un44;
  }
  
  if(r==NULL)
    return(-1.);

  bitrate/=ch;

  if(bitrate<=r[0])return(0.);
  for(i=0;i<10;i++)
    if(r[i]<bitrate && r[i+1]>=bitrate)break;
  if(i==10)return(10.);

  del=(bitrate-r[i])/(r[i+1]-r[i]);
  
  return((i+del)*.1);
}

/* only populates the high-level settings so that we can tweak with ctl before final setup */
int vorbis_encode_setup_vbr(vorbis_info *vi,
			    long channels,
			    long rate,
			    
			    float base_quality){
  int ret=0,i,iq;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  highlevel_encode_setup *hi=&ci->hi;
  
  base_quality+=.0001;
  if(base_quality<0.)base_quality=0.;
  if(base_quality>.999)base_quality=.999;

  iq=base_quality*10;
  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=base_quality*10.-iq;
  }

  ret|=vorbis_encode_toplevel_setup(vi,256,2048,channels,rate);
  hi->base_quality=base_quality;
  hi->base_quality_short=base_quality;
  hi->base_quality_long=base_quality;
  hi->trigger_quality=base_quality;

  for(i=0;i<4;i++){
    hi->blocktype[i].tone_mask_quality=base_quality;
    hi->blocktype[i].tone_peaklimit_quality=base_quality;
    hi->blocktype[i].noise_bias_quality=base_quality;
    hi->blocktype[i].noise_compand_quality=base_quality;
    hi->blocktype[i].ath_quality=base_quality;
  }

  hi->short_block_p=1;
  hi->long_block_p=1;
  hi->impulse_block_p=1;
  hi->amplitude_track_dBpersec=-6.;

  hi->stereo_couple_p=1; /* only relevant if a two channel input */
  hi->stereo_backfill_p=0;
  hi->residue_backfill_p=0;

  /* set the ATH floaters */
  hi->ath_floating_dB=_psy_ath_floater[iq]*(1.-dq)+_psy_ath_floater[iq+1]*dq;
  hi->ath_absolute_dB=_psy_ath_abs[iq]*(1.-dq)+_psy_ath_abs[iq+1]*dq;

  /* set stereo dB and Hz */
  hi->stereo_point_dB=_psy_stereo_point_dB_44[iq];
  hi->stereo_point_kHz[0]=_psy_stereo_point_kHz_44[0][iq]*(1.-dq)+
    _psy_stereo_point_kHz_44[0][iq+1]*dq;
  hi->stereo_point_kHz[1]=_psy_stereo_point_kHz_44[1][iq]*(1.-dq)+
    _psy_stereo_point_kHz_44[1][iq+1]*dq;
  
  /* set lowpass */
  hi->lowpass_kHz[0]=
    hi->lowpass_kHz[1]=
    _psy_lowpass_44[iq]*(1.-dq)+_psy_lowpass_44[iq+1]*dq;

  /* set bitrate approximation */
  vi->bitrate_nominal=vbr_to_approx_bitrate(vi->channels,hi->stereo_couple_p,
					    base_quality,vi->rate);
  vi->bitrate_lower=-1;
  vi->bitrate_upper=-1;
  vi->bitrate_window=-1;

  return(ret);
}

int vorbis_encode_init_vbr(vorbis_info *vi,
			   long channels,
			   long rate,
			   
			   float base_quality /* 0. to 1. */
			   ){
  int ret=0;

  ret=vorbis_encode_setup_vbr(vi,channels,rate,base_quality);
  
  if(ret){
    vorbis_info_clear(vi);
    return ret; 
  }
  ret=vorbis_encode_setup_init(vi);
  if(ret)
    vorbis_info_clear(vi);
  return(ret);
}

int vorbis_encode_setup_managed(vorbis_info *vi,
				long channels,
				long rate,
				
				long max_bitrate,
				long nominal_bitrate,
				long min_bitrate){

  double tnominal=nominal_bitrate;
  double approx_vbr;
  int ret=0;

  if(nominal_bitrate<=0.){
    if(max_bitrate>0.){
      nominal_bitrate=max_bitrate*.875;
    }else{
      if(min_bitrate>0.){
	nominal_bitrate=min_bitrate;
      }else{
	return(OV_EINVAL);
      }
    }
  }

  approx_vbr=approx_bitrate_to_vbr(channels,(channels==2), 
				   (float)nominal_bitrate,rate);
  if(approx_vbr<0)return(OV_EIMPL);


  ret=vorbis_encode_setup_vbr(vi,channels,rate,approx_vbr);
  if(ret){
    vorbis_info_clear(vi);
    return ret; 
  }

  /* adjust to make management's life easier.  Use the ctl() interface
     once it's implemented */
  {
    codec_setup_info *ci=vi->codec_setup;
    highlevel_encode_setup *hi=&ci->hi;

    /* backfills */
    hi->stereo_backfill_p=1;
    hi->residue_backfill_p=1;

    /* no impulse blocks */
    hi->impulse_block_p=0;
    /* de-rate stereo */
    if(hi->stereo_point_dB && hi->stereo_couple_p && channels==2){
      hi->stereo_point_dB++;
      if(hi->stereo_point_dB>3)hi->stereo_point_dB=3;
    }      
    /* slug the vbr noise setting*/
    hi->blocktype[0].noise_bias_quality-=.1;
    if(hi->blocktype[0].noise_bias_quality<0.)
      hi->blocktype[0].noise_bias_quality=0.;
    hi->blocktype[1].noise_bias_quality-=.1;
    if(hi->blocktype[1].noise_bias_quality<0.)
      hi->blocktype[1].noise_bias_quality=0.;
    hi->blocktype[2].noise_bias_quality-=.05;
    if(hi->blocktype[2].noise_bias_quality<0.)
      hi->blocktype[2].noise_bias_quality=0.;
    hi->blocktype[3].noise_bias_quality-=.05;
    if(hi->blocktype[3].noise_bias_quality<0.)
      hi->blocktype[3].noise_bias_quality=0.;
   
    /* initialize management.  Currently hardcoded for 44, but so is above. */
    memcpy(&ci->bi,&_bm_44_default,sizeof(ci->bi));
    ci->bi.queue_hardmin=min_bitrate;
    ci->bi.queue_hardmax=max_bitrate;
    
    ci->bi.queue_avgmin=tnominal;
    ci->bi.queue_avgmax=tnominal;

    /* adjust management */
    ci->bi.avgfloat_noise_maxval=_bm_max_noise_offset[(int)approx_vbr];

  }
  vi->bitrate_nominal = nominal_bitrate;
  vi->bitrate_lower = min_bitrate;
  vi->bitrate_upper = max_bitrate;

  return(ret);
}

int vorbis_encode_init(vorbis_info *vi,
		       long channels,
		       long rate,

		       long max_bitrate,
		       long nominal_bitrate,
		       long min_bitrate){

  int ret=vorbis_encode_setup_managed(vi,channels,rate,
				      max_bitrate,
				      nominal_bitrate,
				      min_bitrate);
  if(ret){
    vorbis_info_clear(vi);
    return(ret);
  }

  ret=vorbis_encode_setup_init(vi);
  if(ret)
    vorbis_info_clear(vi);
  return(ret);
}

int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg){
  return(OV_EIMPL);
}
