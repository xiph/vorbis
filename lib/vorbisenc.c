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

 function: simple programmatic interface for encoder mode setup
 last mod: $Id: vorbisenc.c,v 1.17.2.1 2001/12/04 11:16:20 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>

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
  vorbis_info_residue0 *res;
  static_codebook *book_aux[2];
  static_codebook *books_base[6][10][3];
  static_codebook *books_stereo_backfill[6][10];
  static_codebook *books_residue_backfill[6][10][2];
} vorbis_residue_template;

#include "modes/residue_44.h"
#include "modes/psych_44.h"
#include "modes/floor_44.h"

/* a few static coder conventions */
static vorbis_info_time0 _time_dummy={0};
static vorbis_info_mode _mode_set_short={0,0,0,0};
static vorbis_info_mode _mode_set_long={1,0,0,1};

/* mapping conventions:
   only one submap (this would change for efficient 5.1 support for example)
/* three psychoacoustic profiles are used: One for
   short blocks, and two for long blocks (transition and regular) */
static vorbis_info_mapping0 _mapping_set_short={
  1, {0,0}, {0}, {0}, {0}, {0,0}, 0,{0},{0}};
static vorbis_info_mapping0 _mapping_set_long={
  1, {0,0}, {0}, {1}, {1}, {1,2}, 0,{0},{0}};

static int vorbis_encode_toplevel_init(vorbis_info *vi,int small,int large,int ch,long rate){
  if(vi && vi->codec_setup){
    codec_setup_info *ci=vi->ci;

    vi->version=0;
    vi->channels=ch;
    vi->rate=rate;
    
    ci->blocksizes[0]=small;
    ci->blocksizes[1]=large;

    /* time mapping hooks are unused in vorbis I */
    ci->times=1;
    ci->time_type[0]=0;
    ci->time_param[0]=&time_dummy;

    /* by convention, two modes: one for short, one for long blocks.
       short block mode uses mapping sero, long block uses mapping 1 */
    ci->modes=2;
    ci->mode_param[0]=&_mode_set_short;
    ci->mode_param[1]=&_mode_set_long;

    /* by convention two mappings, both mapping type zero (polyphonic
       PCM), first for short, second for long blocks */
    ci->maps=2;
    ci->map_type[0]=0;
    ci->map_param[0]=&_mapping_set_short;
    ci->map_type[1]=0;
    ci->map_param[1]=&_mapping_set_short;

    return(0);
  }
  return(OV_EINVAL);
}

static int vorbis_encode_floor_init(vorbis_info *vi,double q,int block,
				    static_codebook    **books, 
				    vorbis_info_floor1 **in, 
				    ...){
  int x[11],i,k,iq=rint(q*10);
  vorbis_info_floor1 *f=calloc(1,sizeof(*f));
  codec_setup_info *ci=vi->ci;
  va_list ap;

  va_start(ap,in);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,int);
  va_end(ap);

  memcpy(f,in[x[iq]],sizoef(*f));
  /* fill in the lowpass field, even if it's temporary */
  f->n=ci->blocksizes[block]>>1;

  /* books */
  {
    int partitions=f->partitions;
    int maxclass=-1;
    int maxbook=-1;
    for(i=0;i<partitions;i++)
      if(f->partitionclass[i]>maxclass)maxclass=f->partitionclass[i];
    for(i=0;i<maxclass;i++){
      if(f->class_book[i]>maxbook)maxbook=f->class_book[i];
      f->class_book[i]+=ci->books;
      for(k=0;k<(1<<info->class_subs[i]);k++){
	if(f->class_subbook[i][k]>maxbook)maxbook=f->class_subbook[i][k];
	f->class_subbook[i][k]+=ci->books;
      }
    }

    for(i=0;i<=maxbook;i++)
      ci->book_param[ci->books++]=books[x[iq]][i];
  }

  /* for now, we're only using floor 1 */
  vi->floor_type[vi->floors]=1;
  vi->floor_param[vi->floors]=f;
  vi->floors++;

  return(0);
}

static int vorbis_encode_global_psych_init(vorbis_info *vi,double q,
					   vorbis_info_psy_global **in, ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy_global *g=&ci->g;
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

  memcpy(g,in[(int)x[iq]],sizeof(*g));

  /* interpolate the trigger threshholds */
  for(i=0;i<4;i++){
    g->preecho_thresh[i]=in[iq]->preecho_thresh[i]*(1.-dq)+in[iq+1]->preecho_thresh[i]*dq;
    g->preecho_thresh[i]=in[iq]->postecho_thresh[i]*(1.-dq)+in[iq+1]->postecho_thresh[i]*dq;
  }
  g->ampmax_att_per_sec=in[iq]->ampmax_att_per_sec*(1.-dq)+in[iq+1]->ampmax_att_per_sec*dq;
  return(0);
}

static int vorbis_encode_psyset_init(vorbis_info *vi,double q,int block,
					   vorbis_info_psy *in, ...){
  int i,j,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy *p=&ci->psy_param[block];
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

  if(block>=ci->psys)
    ci->psys=block+1;
  if(!p){
    p=calloc(1,sizeof(*p));
    ci->psy_params[block]=p;
  }

  memcpy(p,in[(int)(q*10.)],sizeof(*p));
  
  p->ath_adjatt=in[iq]->ath_adjatt*(1.-dq)+in[iq+1]->ath_adjatt*dq;
  p->ath_maxatt=in[iq]->ath_maxatt*(1.-dq)+in[iq+1]->ath_maxatt*dq;

  p->tone_masteratt=in[iq]->tone_masteratt*(1.-dq)+in[iq+1]->tone_masteratt*dq;
  p->tone_guard=in[iq]->tone_guard*(1.-dq)+in[iq+1]->tone_guard*dq;
  p->tone_abs_limit=in[iq]->tone_abs_limit*(1.-dq)+in[iq+1]->tone_abs_limit*dq;

  p->noisemaxsupp=in[iq]->noisemaxsupp*(1.-dq)+in[iq+1]->noisemaxsupp*dq;

}

static int vorbis_encode_compand_init(vorbis_info *vi,double q,int block,
					   float **in, ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy *p=&ci->psy_param[block];
  va_list ap;
  
  va_start(ap,in);
  for(i=0;i<11;i++)
    x[i]=va_arg(ap,double);
  va_end(ap);

  if(iq==10){
    iq=9;
    dq=1.;
  }else{
    dq=q+10.-iq;
  }

  /* interpolate the compander settings */
  for(i=0;i<NOISE_COMPAND_LEVELS;i++)
    p->noisecompand[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
}

static int vorbis_encode_tonemask_init(vorbis_info *vi,double q,int block,
				       int ***in, ...){
  int i,j,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy *p=&ci->psy_param[block];
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

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->toneatt[i][j]=(j<4?4:j)*-10.+
	in[iq][i][j]*(1.-dq)+in[iq+1][i][j]*dq;
}

static int vorbis_encode_peak_init(vorbis_info *vi,double q,int block,
				   int ***in, ...){
  int i,j,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy *p=&ci->psy_param[block];
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

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->peakatt[i][j]=(j<4?4:j)*-10.+
	in[iq][i][j]*(1.-dq)+in[iq+1][i][j]*dq;
}

static int vorbis_encode_noisebias_init(vorbis_info *vi,double q,int block,
					int **in, ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy *p=&ci->psy_param[block];
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

  for(i=0;i<P_BANDS;i++)
    p->noiseoff[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
}

static int vorbis_encode_ath_init(vorbis_info *vi,double q,int block,
				  float **in, ...){
  int i,iq=q*10;
  double x[11],dq;
  codec_setup_info *ci=vi->ci;
  vorbis_info_psy *p=&ci->psy_param[block];
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

  for(i=0;i<P_BANDS;i++)
    p->ath[i]=in[iq][i]*(1.-dq)+in[iq+1][i]*dq;
}

static double stereo_threshholds[]={0.0, 2.5, 4.5, 7.5, 12.5, 22.5};
static int vorbis_encode_residue_init(vorbis_info *vi,double q,int block,
				      vorbis_residue_template *in, ...){

  int i,iq=q*10;
  int t[11];
  int a[11];
  double c[11];
  double dq;
  int coupled_p;
  int stereo_backfill_p;
  int residue_backfill_p;
  int type;
  int n;
  int partition_position;
  int res_position;
  int iterations=1;
  double amplitude;
  int amplitude_select;

  codec_setup_info *ci=vi->ci;
  vorbis_info_residue *r;
  vorbis_info_psy *psy=ci->psys+block;
  va_list ap;
  
  va_start(ap,in);
  for(i=0;i<11;i++)
    t[i]=va_arg(ap,int);
  coupled_p=va_arg(ap,int);
  stereo_backfill_p=va_arg(ap,int);
  residue_backfill_p=va_arg(ap,int);
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
  memcpy(r,in[t[iq]],sizeof(*r));

  n=r->end=ci->blocksizes[block?1:0]>>1; /* to be adjusted by lowpass later */

  if(block){
    r->grouping=32;
  }else{
    r->grouping=16;
  }

  res_position=rint((double)c[iq]*1000/vi->rate*n);
  partition_position=res_position/r->grouping;
  for(i=0;i<r->partitions;i++)
    if(r->blimit[i]<0)r->blimit[i]=partition_position;

  /* for uncoupled, we use type 1, else type 2 */
  if(coupled_p){
    ci->map_param[block].coupling_steps=1;
    ci->map_param[block].coupling_mag[0]=0;
    ci->map_param[block].coupling_ang[0]=1;

    ci->residue_type[block]=2;

    psy->couple_pass[0]={1.,1., {{-1,  1,9e10,  0},{9999, 1,9e10, -1}}};
    psy->couple_pass[0].couple_pass[0].limit=res_position;
    ci->passlimit[0]=3;
    amplitude_select=a[qi];
    amplitude=psy->couple_pass[0].couple_pass[1].limit=stereo_threshholds[a[qi]];
    
    if(stereo_backfill_p && a[qi]){
      psy->couple_pass[1]={1.,1., {{-1,  1,9e10,  0},{9999, 1,9e10, -1}}};
      psy->couple_pass[1].couple_pass[0].limit=res_position;
      amplitude_select=a[qi]-1;
      amplitude=psy->couple_pass[1].couple_pass[1].limit=stereo_threshholds[a[qi]-1];
      psy->passlimit[1]=4;
      for(i=0;i<r->partitions;i++)
	if(in[qi].books_stereo_backfill[a[qi]-1][i])
	  r->secondstages[i]|=8;
      iterations++;
    }

    if(residue_backfill_p){
      psy->couple_pass[iterations]={.33333333,3., {{-1,  1,1.,  0},{9999, 1,9e10, -1}}};
      psy->couple_pass[iterations].couple_pass[0].limit=res_position;
      psy->couple_pass[iterations].couple_pass[1].limit=amplitude;      
      ci->passlimit[iterations]=ci->passlimit[iterations-1]+1;
      for(i=0;i<r->partitions;i++)
	if(in[qi].books_residue_backfill[amplitude_select][i][0])
	  r->secondstages[i]|=(1<<(iterations+2));
      iterations++;
      psy->couple_pass[iterations]={.11111111,9., {{-1,  1,.3,  0},{9999, 1,9e10, -1}}};
      psy->couple_pass[iterations].couple_pass[0].limit=res_position;
      psy->couple_pass[iterations].couple_pass[1].limit=amplitude;      
      ci->passlimit[iterations]=r->passlimit[iterations-1]+1;
      for(i=0;i<r->partitions;i++)
	if(in[qi].books_residue_backfill[amplitude_select][i][1])
	  r->secondstages[i]|=(1<<(iterations+2));
      iterations++;
    }
    ci->coupling_passes=iterations;

    if(block)
      memcpy(ci->psys+block+1,psy,sizeof(*psy));

  }else{
    ci->residue_type[block]=1;

    ci->passlimit[0]=3;

    if(residue_backfill_p){
      for(i=0;i<r->partitions;i++){
	if(in[qi].books_residue_backfill[amplitude_select][i][0])
	  r->secondstages[i]|=8;
	if(in[qi].books_residue_backfill[amplitude_select][i][1])
	  r->secondstages[i]|=16;
      }
      ci->passlimit[1]=4;
      ci->passlimit[2]=5;
      ci->coupling_passes=3;
    }else
      ci->coupling_passes=1;
  }
  
  /* fill in all the books */
  {
    int booklist=0;
    r->groupbook=ci->books;
    ci->book_param[ci->books++]=in[qi].book_aux[block];
    for(i=0;i<r->partitions;i++){
      for(k=0;k<3;k++){
	if(in[qi].books_base[a[qi]][i][k]){
	  r->booklist[booklist++]=ci->books;
	  ci->book_param[ci->books++]=in[qi].books_base[a[qi]][i][k]);
      }
      if(coupled_p && stereo_backfill_p && a[qi] &&
	 in[qi].books_stereo_backfill[a[qi]][i]){
	  r->booklist[booklist++]=ci->books;
	  ci->book_param[ci->books++]=in[qi].books_stereo_backfill[a[qi]][i];
      }
      if(residue_backfill_p)
	for(k=0;k<2;k++){
	  if(in[qi].books_residue_backfill[amplitude_select][i][k]){
	    r->booklist[booklist++]=ci->books;
	    ci->book_param[ci->books++]=in[qi].books_residue_backfill[amplitude_select][i][k];
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
  codec_setup_info *ci=vi->ci;
  vorbis_info_floor1 *f=ci->floor_param+block;
  vorbis_info_residue0 *r=ci->residue_param+block;
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
  
  freq=x[iq]*(1.-dq)+x[iq+1]*dq;

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

  if(rate>40000){
    ret|=vorbis_encode_toplevel_init(vi,256,2048,channels,rate);
    ret|=vorbis_encode_floor_init(vi,base_quality,0,_floor_44_128_books,_floor_44_128,
				  0,0,0,1,1,1,1,1,1,1,1);
    ret|=vorbis_encode_floor_init(vi,base_quality,1,_floor_44_1024_books,_floor_44_1024,
				  0,0,0,0,0,0,0,0,0,0,0);
    
    ret|=vorbis_encode_global_psych_init(vi,base_quality,_psy_global_44,
					 0., 0., .5, 1., 1., 1., 1., 1., 1., 1., 1.);
    
    ret|=vorbis_encode_psyset_init(vi,base_quality,0,_psy_settings,
				   0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.);
    ret|=vorbis_encode_psyset_init(vi,base_quality,1,_psy_settings,
				   0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.);
    ret|=vorbis_encode_psyset_init(vi,base_quality,2,_psy_settings,
				   0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.);

    ret|=vorbis_encode_tonemask_init(vi,base_quality,0,_vp_tonemask_adj_otherblock,
				     0., .5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.);
    ret|=vorbis_encode_tonemask_init(vi,base_quality,1,_vp_tonemask_adj_otherblock,
				     0., .5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.);
    ret|=vorbis_encode_tonemask_init(vi,base_quality,2,_vp_tonemask_adj_longblock,
				     0., .5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.);

    ret|=vorbis_encode_compand_init(vi,base_quality,0,_psy_compand_44,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    ret|=vorbis_encode_compand_init(vi,base_quality,1,_psy_compand_44,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    ret|=vorbis_encode_compand_init(vi,base_quality,2,_psy_compand_44,
				    1., 1., 1.3, 1.6, 2., 2., 2., 2., 2., 2., 2.);
    
    ret|=vorbis_encode_peak_init(vi,base_quality,0,_vp_peakguard,
				 0., .5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.);
    ret|=vorbis_encode_peak_init(vi,base_quality,1,_vp_peakguard,
				 0., .5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.);
    ret|=vorbis_encode_peak_init(vi,base_quality,2,_vp_peakguard,
				 0., .5, 1., 1.5, 2., 2.5, 3., 3.5, 4., 4.5, 5.);
    
    ret|=vorbis_encode_noisebias_init(vi,base_quality,0,_vp_noisebias_other,
				      0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.);
    ret|=vorbis_encode_noisebias_init(vi,base_quality,1,_vp_noisebias_other,
				      0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.);
    ret|=vorbis_encode_noisebias_init(vi,base_quality,2,_vp_noisebias_long,
				      0., 1., 2., 3., 4., 5., 6., 7., 8., 9., 10.);

    ret|=vorbis_encode_ath_init(vi,base_quality,0,ATH_Bark_dB,
				0., 0., 0., 0., 0., .5, 1., 1., 1.5, 2., 2.);
    ret|=vorbis_encode_ath_init(vi,base_quality,1,ATH_Bark_dB,
				0., 0., 0., 0., 0., .5, 1., 1., 1.5, 2., 2.);
    ret|=vorbis_encode_ath_init(vi,base_quality,2,ATH_Bark_dB,
				0., 0., 0., 0., 0., .5, 1., 1., 1.5, 2., 2.);
    
    if(ret){
      vorbis_info_clear(vi);
      return ret; 
    }

    switch(channels){
    case 2:
      /* setup specific to stereo coupling */

      /* unmanaged, one iteration residue setup */
      ret|=vorbis_encode_residue_init(vi,base_quality,0,_residue_template_44_stereo_temp,
				      1, /* coupled */
				      0, /* no mid stereo backfill */
				      0, /* no residue backfill */
				      4,  3,  3,  2,   1,  0,  0,  0,  0,  0,  0,
				      4., 4., 6., 6., 10., 4., 4., 4., 4., 4., 4.);
      
      ret|=vorbis_encode_residue_init(vi,base_quality,1,_residue_template_44_stereo_temp,
				      1, /* coupled */
				      0, /* no mid stereo backfill */
				      0, /* no residue backfill */
				      4,  3,  3,   2,   1,  0,  0,  0,  0,  0,  0,
				      4., 6., 6., 10., 10., 4., 4., 4., 4., 4., 4.);      

      ret|=vorbis_encode_lowpass_init(vi,base_quality,0,
				      15.1,15.9,16.9,17.9.,19.9.,
				      999.,999.,999.,999.,999.,999.);
      ret|=vorbis_encode_lowpass_init(vi,base_quality,1,
				      15.1,15.9,16.9,17.9.,19.9.,
				      999.,999.,999.,999.,999.,999.);
      
      return(ret);

      break;
    default:
      return(OV_EIMPL);
      
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
  return(OV_EIMPL);
}

int vorbis_encode_ctl(vorbis_info *vi,int number,void *arg){
  return(OV_EIMPL);
}
		       
