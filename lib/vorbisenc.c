/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: simple programmatic interface for encoder mode setup
 last mod: $Id: vorbisenc.c,v 1.23 2001/12/16 04:15:47 xiphmont Exp $

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

static int vorbis_encode_toplevel_init(vorbis_info *vi,int small,int large,int ch,long rate){
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
    ci->time_param[0]=calloc(1,sizeof(_time_dummy));
    memcpy(ci->time_param[0],&_time_dummy,sizeof(_time_dummy));

    /* by convention, two modes: one for short, one for long blocks.
       short block mode uses mapping sero, long block uses mapping 1 */
    ci->modes=2;
    ci->mode_param[0]=calloc(1,sizeof(_mode_set_short));
    memcpy(ci->mode_param[0],&_mode_set_short,sizeof(_mode_set_short));
    ci->mode_param[1]=calloc(1,sizeof(_mode_set_long));
    memcpy(ci->mode_param[1],&_mode_set_long,sizeof(_mode_set_long));

    /* by convention two mappings, both mapping type zero (polyphonic
       PCM), first for short, second for long blocks */
    ci->maps=2;
    ci->map_type[0]=0;
    ci->map_param[0]=calloc(1,sizeof(_mapping_set_short));
    memcpy(ci->map_param[0],&_mapping_set_short,sizeof(_mapping_set_short));
    ci->map_type[1]=0;
    ci->map_param[1]=calloc(1,sizeof(_mapping_set_long));
    memcpy(ci->map_param[1],&_mapping_set_long,sizeof(_mapping_set_long));

    return(0);
  }
  return(OV_EINVAL);
}

static int vorbis_encode_floor_init(vorbis_info *vi,double q,int block,
				    static_codebook    ***books, 
				    vorbis_info_floor1 *in, 
				    ...){
  int x[11],i,k,iq=rint(q*10);
  vorbis_info_floor1 *f=calloc(1,sizeof(*f));
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

static int vorbis_encode_global_psych_init(vorbis_info *vi,double q,
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
  g->ampmax_att_per_sec=in[iq].ampmax_att_per_sec*(1.-dq)+in[iq+1].ampmax_att_per_sec*dq;
  return(0);
}

static int vorbis_encode_psyset_init(vorbis_info *vi,double q,int block,
					   vorbis_info_psy *in){
  int iq=q*10;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }

  if(block>=ci->psys)
    ci->psys=block+1;
  if(!p){
    p=calloc(1,sizeof(*p));
    ci->psy_param[block]=p;
  }

  memcpy(p,in+(int)(q*10.),sizeof(*p));
  
  p->ath_adjatt=in[iq].ath_adjatt*(1.-dq)+in[iq+1].ath_adjatt*dq;
  p->ath_maxatt=in[iq].ath_maxatt*(1.-dq)+in[iq+1].ath_maxatt*dq;

  p->tone_masteratt=in[iq].tone_masteratt*(1.-dq)+in[iq+1].tone_masteratt*dq;
  p->tone_guard=in[iq].tone_guard*(1.-dq)+in[iq+1].tone_guard*dq;
  p->tone_abs_limit=in[iq].tone_abs_limit*(1.-dq)+in[iq+1].tone_abs_limit*dq;

  p->noisemaxsupp=in[iq].noisemaxsupp*(1.-dq)+in[iq+1].noisemaxsupp*dq;

  return(0);
}

static int vorbis_encode_compand_init(vorbis_info *vi,double q,int block,
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

static int vorbis_encode_tonemask_init(vorbis_info *vi,double q,int block,
				       vp_adjblock *in){
  int i,j,iq=q*5.;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

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

static int vorbis_encode_peak_init(vorbis_info *vi,double q,int block,
				   vp_adjblock *in){
  int i,j,iq=q*5.;
  double dq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy *p=ci->psy_param[block];

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

static int vorbis_encode_noisebias_init(vorbis_info *vi,double q,int block,
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

  p->noisewindowlomin=guard[iq*3];
  p->noisewindowhimin=guard[iq*3+1];
  p->noisewindowfixed=guard[iq*3+2];

  for(i=0;i<P_BANDS;i++)
    p->noiseoff[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
  return(0);
}

static int vorbis_encode_ath_init(vorbis_info *vi,double q,int block,
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

static int vorbis_encode_residue_init(vorbis_info *vi,double q,int block,
				      int coupled_p,
				      int stereo_backfill_p,
				      int residue_backfill_p,
				      vorbis_residue_template *in, ...){

  int i,iq=q*10;
  int a[11];
  double c[11];
  int n,k;
  int partition_position;
  int res_position;
  int iterations=1;
  int amplitude_select=0;

  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_residue0 *r;
  vorbis_info_psy *psy=ci->psy_param[block*2];
  va_list ap;
  
  va_start(ap,in);
  for(i=0;i<11;i++)
    a[i]=va_arg(ap,int);
  for(i=0;i<11;i++)
    c[i]=va_arg(ap,double);
  va_end(ap);
  
  /* may be re-called due to ctl */
  if(ci->residue_param[block])
    /* free preexisting instance */
    residue_free_info(ci->residue_param[block],ci->residue_type[block]);

  r=ci->residue_param[block]=malloc(sizeof(*r));
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
    partition_position=rint((double)c[iq]*1000/(vi->rate/2)*n/r->grouping);
    res_position=partition_position*r->grouping;
    break;
  case 2:
    n=r->end=(ci->blocksizes[block?1:0]>>1)*vi->channels; /* to be adjusted by lowpass later */
    partition_position=rint((double)c[iq]*1000/(vi->rate/2)*n/r->grouping);
    res_position=partition_position*r->grouping/vi->channels;
    break;
  }

  for(i=0;i<r->partitions;i++)
    if(r->blimit[i]<0)r->blimit[i]=partition_position;

  for(i=0;i<r->partitions;i++)
    for(k=0;k<3;k++)
      if(in[iq].books_base[a[iq]][i][k])
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
    psy->couple_pass[0].couple_pass[1].amppost_point=stereo_threshholds[a[iq]];
    amplitude_select=a[iq];

    if(stereo_backfill_p && a[iq]){
      memcpy(psy->couple_pass+iterations,psy->couple_pass+iterations-1,
	     sizeof(*psy->couple_pass));
      amplitude_select=a[iq]-1;
      psy->couple_pass[1].couple_pass[1].amppost_point=stereo_threshholds[a[iq]-1];
      ci->passlimit[1]=4;
      for(i=0;i<r->partitions;i++)
	if(in[iq].books_stereo_backfill[a[iq]-1][i])
	  r->secondstages[i]|=8;
      iterations++;
    }

    if(residue_backfill_p){
      memcpy(psy->couple_pass+iterations,psy->couple_pass+iterations-1,
	     sizeof(*psy->couple_pass));
      psy->couple_pass[iterations].granulem=.333333333;
      psy->couple_pass[iterations].igranulem=3.;
      for(i=0;i<r->partitions;i++)
	if(in[iq].books_residue_backfill[amplitude_select][i][0])
	  r->secondstages[i]|=(1<<(iterations+2));
      ci->passlimit[iterations]=ci->passlimit[iterations-1]+1;
      iterations++;

      memcpy(psy->couple_pass+iterations,psy->couple_pass+iterations-1,
	     sizeof(*psy->couple_pass));
      psy->couple_pass[iterations].granulem=.1111111111;
      psy->couple_pass[iterations].igranulem=9.;
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
	 sizeof(psy->couple_pass[0]));
  
  /* fill in all the books */
  {
    int booklist=0,k;
    r->groupbook=ci->books;
    ci->book_param[ci->books++]=in[iq].book_aux[block];
    for(i=0;i<r->partitions;i++){
      for(k=0;k<3;k++){
	if(in[iq].books_base[a[iq]][i][k]){
	  int bookid=book_dup_or_new(ci,in[iq].books_base[a[iq]][i][k]);
	  r->booklist[booklist++]=bookid;
	  ci->book_param[bookid]=in[iq].books_base[a[iq]][i][k];
	}
      }
      if(coupled_p && stereo_backfill_p && a[iq] &&
	 in[iq].books_stereo_backfill[a[iq]][i]){
	int bookid=book_dup_or_new(ci,in[iq].books_stereo_backfill[a[iq]][i]);
	r->booklist[booklist++]=bookid;
	ci->book_param[bookid]=in[iq].books_stereo_backfill[a[iq]][i];
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

static int vorbis_encode_lowpass_init(vorbis_info *vi,double q,int block,...){
  int i,iq=q*10;
  double x[11],dq;
  double freq;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_floor1 *f=ci->floor_param[block];
  vorbis_info_residue0 *r=ci->residue_param[block];
  int blocksize=ci->blocksizes[block]>>1;
  double nyq=vi->rate/2.;
  va_list ap;
  
  va_start(ap,block);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,double);
  va_end(ap);

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q*10.-iq;
  }
  
  freq=(x[iq]*(1.-dq)+x[iq+1]*dq)*1000.;
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
    r->end=((freq/nyq*blocksize*2)/r->grouping+.9)* /* round up only if we're well past */
      r->grouping;
  else
    r->end=((freq/nyq*blocksize)/r->grouping+.9)* /* round up only if we're well past */
      r->grouping;
  return(0);
}

/* encoders will need to use vorbis_info_init beforehand and call
   vorbis_info clear when all done */

int vorbis_encode_init_vbr(vorbis_info *vi,
			   long channels,
			   long rate,
			   
			   float base_quality /* 0. to 1. */
			   ){
  int ret=0;

  base_quality+=.001;
  if(base_quality<0.)base_quality=0.;
  if(base_quality>.999)base_quality=.999;

  if(rate>40000){
    ret|=vorbis_encode_toplevel_init(vi,256,2048,channels,rate);
    ret|=vorbis_encode_floor_init(vi,base_quality,0,_floor_44_128_books,_floor_44_128,
				  0,1,1,2,2,2,2,2,2,2,2);
    ret|=vorbis_encode_floor_init(vi,base_quality,1,_floor_44_1024_books,_floor_44_1024,
				  0,0,0,0,0,0,0,0,0,0,0);
    
    ret|=vorbis_encode_global_psych_init(vi,base_quality,_psy_global_44,
					 0., 1., 1.5, 2., 2., 2., 2., 2., 2., 2., 2.);
    
    ret|=vorbis_encode_psyset_init(vi,base_quality,0,_psy_settings);
    ret|=vorbis_encode_psyset_init(vi,base_quality,1,_psy_settings);
    ret|=vorbis_encode_psyset_init(vi,base_quality,2,_psy_settings);
    ret|=vorbis_encode_psyset_init(vi,base_quality,3,_psy_settings);

    ret|=vorbis_encode_tonemask_init(vi,base_quality,0,_vp_tonemask_adj_otherblock);
    ret|=vorbis_encode_tonemask_init(vi,base_quality,1,_vp_tonemask_adj_otherblock);
    ret|=vorbis_encode_tonemask_init(vi,base_quality,2,_vp_tonemask_adj_otherblock);
    ret|=vorbis_encode_tonemask_init(vi,base_quality,3,_vp_tonemask_adj_longblock);

    ret|=vorbis_encode_compand_init(vi,base_quality,0,_psy_compand_44_short,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    ret|=vorbis_encode_compand_init(vi,base_quality,1,_psy_compand_44_short,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    ret|=vorbis_encode_compand_init(vi,base_quality,2,_psy_compand_44,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    ret|=vorbis_encode_compand_init(vi,base_quality,3,_psy_compand_44,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    
    ret|=vorbis_encode_peak_init(vi,base_quality,0,_vp_peakguard);
    ret|=vorbis_encode_peak_init(vi,base_quality,1,_vp_peakguard);
    ret|=vorbis_encode_peak_init(vi,base_quality,2,_vp_peakguard);
    ret|=vorbis_encode_peak_init(vi,base_quality,3,_vp_peakguard);
    
    ret|=vorbis_encode_noisebias_init(vi,base_quality,0,_psy_noisebias_impulse,
				      _psy_noiseguards_short);
    ret|=vorbis_encode_noisebias_init(vi,base_quality,1,_psy_noisebias_other,
				      _psy_noiseguards_short);
    ret|=vorbis_encode_noisebias_init(vi,base_quality,2,_psy_noisebias_other,
				      _psy_noiseguards_long);
    ret|=vorbis_encode_noisebias_init(vi,base_quality,3,_psy_noisebias_long,
				      _psy_noiseguards_long);

    ret|=vorbis_encode_ath_init(vi,base_quality,0,ATH_Bark_dB,
				0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);
    ret|=vorbis_encode_ath_init(vi,base_quality,1,ATH_Bark_dB,
				0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);
    ret|=vorbis_encode_ath_init(vi,base_quality,2,ATH_Bark_dB,
				0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);
    ret|=vorbis_encode_ath_init(vi,base_quality,3,ATH_Bark_dB,
				0., 0., 0., 0., .2, .5, 1., 1., 1.5, 2., 2.);

    if(ret){
      vorbis_info_clear(vi);
      return ret; 
    }

    switch(channels){
    case 2:
      /* setup specific to stereo coupling */

      /* unmanaged, one iteration residue setup */
      ret|=vorbis_encode_residue_init(vi,base_quality,0,
				      1, /* coupled */
				      0, /* no mid stereo backfill */
				      0, /* no residue backfill */
				      _residue_template_44_stereo,
				      4,  3,  2,  2,   1,  0,  0,  0,  0,  0,  0,
				      4., 6., 6., 6., 10., 6., 6., 4., 4., 4., 4.);
      
      ret|=vorbis_encode_residue_init(vi,base_quality,1,
				      1, /* coupled */
				      0, /* no mid stereo backfill */
				      0, /* no residue backfill */
				      _residue_template_44_stereo,
				      4,  3,  2,   2,   1,  0,  0,  0,  0,  0,  0,
				      6., 6., 6., 10., 10., 6., 6., 4., 4., 4., 4.);      

      ret|=vorbis_encode_lowpass_init(vi,base_quality,0,
				      15.1,15.8,16.5,17.9,20.5,
				      999.,999.,999.,999.,999.,999.);
      ret|=vorbis_encode_lowpass_init(vi,base_quality,1,
				      15.1,15.8,16.5,17.9,20.5,
				      999.,999.,999.,999.,999.,999.);
      
      return(ret);

      break;
    default:
      /* setup specific to non-stereo (mono or uncoupled polyphonic)
         coupling */
      
      /* unmanaged, one iteration residue setup */
      ret|=vorbis_encode_residue_init(vi,base_quality,0,
				      0, /* uncoupled */
				      0, /* no mid stereo backfill */
				      0, /* residue backfill */
				      _residue_template_44_uncoupled,
				      0,0,0,0,0,0,0,0,0,0,0,
				      4.,4.,4.,6.,6.,6.,6.,4.,4.,4.,4.);
      
      ret|=vorbis_encode_residue_init(vi,base_quality,1,
				      0, /* uncoupled */
				      0, /* no mid stereo backfill */
				      0, /* residue backfill */
				      _residue_template_44_uncoupled,
				      0,0,0,0,0,0,0,0,0,0,0,
				      4.,4.,4.,6.,6.,6.,6.,4.,4.,4.,4.);      

      ret|=vorbis_encode_lowpass_init(vi,base_quality,0,
				      15.1,15.8,16.5,17.9,20.5,
				      999.,999.,999.,999.,999.,999.);
      ret|=vorbis_encode_lowpass_init(vi,base_quality,1,
				      15.1,15.8,16.5,17.9,20.5,
				      999.,999.,999.,999.,999.,999.);
      
      return(ret);
      break;
    }
    return(0);
  }else
    return(OV_EIMPL);

  if(ret)
    vorbis_info_clear(vi);
  return(ret);
}


int vorbis_encode_init(vorbis_info *vi,
		       long channels,
		       long rate,

		       long max_bitrate,
		       long nominal_bitrate,
		       long min_bitrate){

  /* it's temporary while I do the merge; relax */
  if(rate>40000){
    if(nominal_bitrate>360000){
      return(vorbis_encode_init_vbr(vi,channels,rate, 1.));
    }else if(nominal_bitrate>270000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .9));
    }else if(nominal_bitrate>230000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .8));
    }else if(nominal_bitrate>200000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .7));
    }else if(nominal_bitrate>180000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .6));
    }else if(nominal_bitrate>140000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .5));
    }else if(nominal_bitrate>120000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .4));
    }else if(nominal_bitrate>100000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .3));
    }else if(nominal_bitrate>90000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .2));
    }else if(nominal_bitrate>75000){
      return(vorbis_encode_init_vbr(vi,channels,rate, .1));
    }else{
      return(vorbis_encode_init_vbr(vi,channels,rate, .0));
    }
  }

  return(OV_EIMPL);
}

int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg){
  return(OV_EIMPL);
}
		       







