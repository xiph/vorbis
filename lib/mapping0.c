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

 function: channel mapping 0 implementation
 last mod: $Id: mapping0.c,v 1.49.2.1 2002/05/07 23:47:13 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "codebook.h"
#include "window.h"
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

extern int analysis_noisy;

typedef struct {
  drft_lookup fft_look;
  vorbis_info_mode *mode;
  vorbis_info_mapping0 *map;

  vorbis_look_floor **floor_look;

  vorbis_look_residue **residue_look;
  vorbis_look_psy *psy_look[2];

  vorbis_func_floor **floor_func;
  vorbis_func_residue **residue_func;

  int ch;
  long lastframe; /* if a different mode is called, we need to 
		     invalidate decay */
} vorbis_look_mapping0;

static vorbis_info_mapping *mapping0_copy_info(vorbis_info_mapping *vm){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;
  vorbis_info_mapping0 *ret=_ogg_malloc(sizeof(*ret));
  memcpy(ret,info,sizeof(*ret));
  return(ret);
}

static void mapping0_free_info(vorbis_info_mapping *i){
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)i;
  if(info){
    memset(info,0,sizeof(*info));
    _ogg_free(info);
  }
}

static void mapping0_free_look(vorbis_look_mapping *look){
  int i;
  vorbis_look_mapping0 *l=(vorbis_look_mapping0 *)look;
  if(l){
    drft_clear(&l->fft_look);

    for(i=0;i<l->map->submaps;i++){
      l->floor_func[i]->free_look(l->floor_look[i]);
      l->residue_func[i]->free_look(l->residue_look[i]);
    }
    if(l->psy_look[1] && l->psy_look[1]!=l->psy_look[0]){
      _vp_psy_clear(l->psy_look[1]);
      _ogg_free(l->psy_look[1]);
    }
    if(l->psy_look[0]){
      _vp_psy_clear(l->psy_look[0]);
      _ogg_free(l->psy_look[0]);
    }
    _ogg_free(l->floor_func);
    _ogg_free(l->residue_func);
    _ogg_free(l->floor_look);
    _ogg_free(l->residue_look);
    memset(l,0,sizeof(*l));
    _ogg_free(l);
  }
}

static vorbis_look_mapping *mapping0_look(vorbis_dsp_state *vd,vorbis_info_mode *vm,
			  vorbis_info_mapping *m){
  int i;
  vorbis_info          *vi=vd->vi;
  codec_setup_info     *ci=vi->codec_setup;
  vorbis_look_mapping0 *look=_ogg_calloc(1,sizeof(*look));
  vorbis_info_mapping0 *info=look->map=(vorbis_info_mapping0 *)m;
  look->mode=vm;
  
  look->floor_look=_ogg_calloc(info->submaps,sizeof(*look->floor_look));

  look->residue_look=_ogg_calloc(info->submaps,sizeof(*look->residue_look));

  look->floor_func=_ogg_calloc(info->submaps,sizeof(*look->floor_func));
  look->residue_func=_ogg_calloc(info->submaps,sizeof(*look->residue_func));
  
  for(i=0;i<info->submaps;i++){
    int floornum=info->floorsubmap[i];
    int resnum=info->residuesubmap[i];

    look->floor_func[i]=_floor_P[ci->floor_type[floornum]];
    look->floor_look[i]=look->floor_func[i]->
      look(vd,vm,ci->floor_param[floornum]);
    look->residue_func[i]=_residue_P[ci->residue_type[resnum]];
    look->residue_look[i]=look->residue_func[i]->
      look(vd,vm,ci->residue_param[resnum]);
    
  }
  if(ci->psys && vd->analysisp){
    if(info->psy[0] != info->psy[1]){

      int psynum=info->psy[0];
      look->psy_look[0]=_ogg_calloc(1,sizeof(*look->psy_look[0]));      
      _vp_psy_init(look->psy_look[0],ci->psy_param[psynum],
		   &ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);

      psynum=info->psy[1];
      look->psy_look[1]=_ogg_calloc(1,sizeof(*look->psy_look[1]));      
      _vp_psy_init(look->psy_look[1],ci->psy_param[psynum],
		   &ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);
    }else{

      int psynum=info->psy[0];
      look->psy_look[0]=_ogg_calloc(1,sizeof(*look->psy_look[0]));      
      look->psy_look[1]=look->psy_look[0];
      _vp_psy_init(look->psy_look[0],ci->psy_param[psynum],
		   &ci->psy_g_param,
		   ci->blocksizes[vm->blockflag]/2,vi->rate);

    }
  }

  look->ch=vi->channels;

  if(vd->analysisp)drft_init(&look->fft_look,ci->blocksizes[vm->blockflag]);
  return(look);
}

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

static void mapping0_pack(vorbis_info *vi,vorbis_info_mapping *vm,
			  oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=(vorbis_info_mapping0 *)vm;

  /* another 'we meant to do it this way' hack...  up to beta 4, we
     packed 4 binary zeros here to signify one submapping in use.  We
     now redefine that to mean four bitflags that indicate use of
     deeper features; bit0:submappings, bit1:coupling,
     bit2,3:reserved. This is backward compatable with all actual uses
     of the beta code. */

  if(info->submaps>1){
    oggpack_write(opb,1,1);
    oggpack_write(opb,info->submaps-1,4);
  }else
    oggpack_write(opb,0,1);

  if(info->coupling_steps>0){
    oggpack_write(opb,1,1);
    oggpack_write(opb,info->coupling_steps-1,8);
    
    for(i=0;i<info->coupling_steps;i++){
      oggpack_write(opb,info->coupling_mag[i],ilog2(vi->channels));
      oggpack_write(opb,info->coupling_ang[i],ilog2(vi->channels));
    }
  }else
    oggpack_write(opb,0,1);
  
  oggpack_write(opb,0,2); /* 2,3:reserved */

  /* we don't write the channel submappings if we only have one... */
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++)
      oggpack_write(opb,info->chmuxlist[i],4);
  }
  for(i=0;i<info->submaps;i++){
    oggpack_write(opb,0,8); /* time submap unused */
    oggpack_write(opb,info->floorsubmap[i],8);
    oggpack_write(opb,info->residuesubmap[i],8);
  }
}

/* also responsible for range checking */
static vorbis_info_mapping *mapping0_unpack(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  vorbis_info_mapping0 *info=_ogg_calloc(1,sizeof(*info));
  codec_setup_info     *ci=vi->codec_setup;
  memset(info,0,sizeof(*info));

  if(oggpack_read(opb,1))
    info->submaps=oggpack_read(opb,4)+1;
  else
    info->submaps=1;

  if(oggpack_read(opb,1)){
    info->coupling_steps=oggpack_read(opb,8)+1;

    for(i=0;i<info->coupling_steps;i++){
      int testM=info->coupling_mag[i]=oggpack_read(opb,ilog2(vi->channels));
      int testA=info->coupling_ang[i]=oggpack_read(opb,ilog2(vi->channels));

      if(testM<0 || 
	 testA<0 || 
	 testM==testA || 
	 testM>=vi->channels ||
	 testA>=vi->channels) goto err_out;
    }

  }

  if(oggpack_read(opb,2)>0)goto err_out; /* 2,3:reserved */
    
  if(info->submaps>1){
    for(i=0;i<vi->channels;i++){
      info->chmuxlist[i]=oggpack_read(opb,4);
      if(info->chmuxlist[i]>=info->submaps)goto err_out;
    }
  }
  for(i=0;i<info->submaps;i++){
    oggpack_read(opb,8); /* time submap unused */
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
static ogg_int64_t total=0;
extern void _analysis_output_always(char *base,int i,float *v,int n,int bark,int dB,ogg_int64_t off);

extern int *floor1_fit(vorbis_block *vb,vorbis_look_floor *look,
		       const float *logmdct,   /* in */
		       const float *logmask);
extern int *floor1_interpolate_fit(vorbis_block *vb,vorbis_look_floor *look,
				   int *A,int *B,
				   int del);
extern int floor1_encode(vorbis_block *vb,vorbis_look_floor *look,
			 int *post,int *ilogmask);


static int mapping0_forward(vorbis_block *vb,vorbis_look_mapping *l){
  vorbis_dsp_state      *vd=vb->vd;
  vorbis_info           *vi=vd->vi;
  codec_setup_info      *ci=vi->codec_setup;
  backend_lookup_state  *b=vb->vd->backend_state;
  vorbis_look_mapping0  *look=(vorbis_look_mapping0 *)l;
  vorbis_info_mapping0  *info=look->map;
  /*vorbis_info_mode      *mode=look->mode;*/
  vorbis_block_internal *vbi=(vorbis_block_internal *)vb->internal;
  int                    n=vb->pcmend;
  int i,j,k;

  int    *nonzero    = alloca(sizeof(*nonzero)*vi->channels);
  float  **gmdct     = _vorbis_block_alloc(vb,vi->channels*sizeof(*gmdct));
  int    **ilogmaskch= _vorbis_block_alloc(vb,vi->channels*sizeof(*gmdct));
  int ***floor_posts = _vorbis_block_alloc(vb,vi->channels*sizeof(*floor_posts));
  
  float global_ampmax=vbi->ampmax;
  float *local_ampmax=alloca(sizeof(*local_ampmax)*vi->channels);
  int blocktype=vbi->blocktype;
  long setup_bits=0;


  /* we differentiate between short and long block types to help the
     masking engine; the window shapes also matter.
     impulse block (a short block in which an impulse occurs)
     padding block (a short block that pads between a transitional 
          long block and an impulse block, or vice versa)
     transition block (the weird one; a long block with the transition 
          window; affects bass/midrange response and that must be 
	  accounted for in masking) 
     long block (run of the mill long block)
  */

  for(i=0;i<vi->channels;i++){
    float scale=4.f/n;
    float scale_dB;

    /* the following makes things clearer to *me* anyway */
    float *pcm     =vb->pcm[i]; 
    float *logfft  =pcm;

    gmdct[i]=_vorbis_block_alloc(vb,n/2*sizeof(**gmdct));

    scale_dB=todB(&scale);

#if 0
    if(vi->channels==2)
      if(i==0)
	_analysis_output_always("pcmL",seq,pcm,n,0,0,total-n/2);
      else
	_analysis_output_always("pcmR",seq,pcm,n,0,0,total-n/2);
#endif
  
    /* window the PCM data */
    _vorbis_apply_window(pcm,b->window,ci->blocksizes,vb->lW,vb->W,vb->nW);

#if 0
    if(vi->channels==2)
      if(i==0)
	_analysis_output_always("windowedL",seq,pcm,n,0,0,total-n/2);
      else
	_analysis_output_always("windowedR",seq,pcm,n,0,0,total-n/2);
#endif

    /* transform the PCM data */
    /* only MDCT right now.... */
    mdct_forward(b->transform[vb->W][0],pcm,gmdct[i]);
    
    /* FFT yields more accurate tonal estimation (not phase sensitive) */
    drft_forward(&look->fft_look,pcm);
    logfft[0]=scale_dB+todB(pcm);
    local_ampmax[i]=logfft[0];
    for(j=1;j<n-1;j+=2){
      float temp=pcm[j]*pcm[j]+pcm[j+1]*pcm[j+1];
      temp=logfft[(j+1)>>1]=scale_dB+.5f*todB(&temp);
      if(temp>local_ampmax[i])local_ampmax[i]=temp;
    }

    if(local_ampmax[i]>0.f)local_ampmax[i]=0.f;
    if(local_ampmax[i]>global_ampmax)global_ampmax=local_ampmax[i];

#if 0
    if(vi->channels==2)
      if(i==0)
	_analysis_output_always("fftL",seq,logfft,n/2,1,0,0);
      else
	_analysis_output_always("fftR",seq,logfft,n/2,1,0,0);
#endif

  }

  {
    float   *noise        = _vorbis_block_alloc(vb,n/2*sizeof(*noise));
    float   *tone         = _vorbis_block_alloc(vb,n/2*sizeof(*tone));

    for(i=0;i<vi->channels;i++){
      int submap=info->chmuxlist[i];
      
      /* the following makes things clearer to *me* anyway */
      float *mdct    =gmdct[i];
      float *logfft  =vb->pcm[i];
      
      float *logmdct =logfft+n/2;
      float *logmask =logfft;

      floor_posts[i]=_vorbis_block_alloc(vb,PACKETBLOBS*sizeof(**floor_posts));
      memset(floor_posts[i],0,sizeof(**floor_posts)*PACKETBLOBS);
      
      for(j=0;j<n/2;j++)
	logmdct[j]=todB(mdct+j);

#if 0
      if(vi->channels==2)
	if(i==0)
	  _analysis_output_always("mdctL",seq,logmdct,n/2,1,0,0);
	else
	  _analysis_output_always("mdctR",seq,logmdct,n/2,1,0,0);
#endif 

      /* first step; noise masking.  Not only does 'noise masking'
         give us curves from which we can decide how much resolution
         to give noise parts of the spectrum, it also implicitly hands
         us a tonality estimate (the larger the value in the
         'noise_depth' vector, the more tonal that area is) */

      _vp_noisemask(look->psy_look[blocktype],
		    logmdct,
		    noise); /* noise does not have by-frequency offset
                               bias applied yet */
#if 0
      if(vi->channels==2)
	if(i==0)
	  _analysis_output_always("noiseL",seq,noise,n/2,1,0,0);
	else
	  _analysis_output_always("noiseR",seq,noise,n/2,1,0,0);
#endif

      /* second step: 'all the other crap'; all the stuff that isn't
         computed/fit for bitrate management goes in the second psy
         vector.  This includes tone masking, peak limiting and ATH */

      _vp_tonemask(look->psy_look[blocktype],
		   logfft,
		   tone,
		   global_ampmax,
		   local_ampmax[i]);

#if 0
      if(vi->channels==2)
	if(i==0)
	  _analysis_output_always("toneL",seq,tone,n/2,1,0,0);
	else
	  _analysis_output_always("toneR",seq,tone,n/2,1,0,0);
#endif

      /* third step; we offset the noise vectors, overlay tone
	 masking.  We then do a floor1-specific line fit.  If we're
	 performing bitrate management, the line fit is performed
	 multiple times for up/down tweakage on demand. */
      
      _vp_offset_and_mix(look->psy_look[blocktype],
			 noise,
			 tone,
			 1,
			 logmask);

#if 0
      if(vi->channels==2)
	if(i==0)
	  _analysis_output_always("mask1L",seq,logmask,n/2,1,0,0);
	else
	  _analysis_output_always("mask1R",seq,logmask,n/2,1,0,0);
#endif

      /* this algorithm is hardwired to floor 1 for now; abort out if
         we're *not* floor1.  This won't happen unless someone has
         broken the encode setup lib.  Guard it anyway. */
      if(ci->floor_type[info->floorsubmap[submap]]!=1)return(-1);

      floor_posts[i][PACKETBLOBS/2]=
	floor1_fit(vb,look->floor_look[submap],
		   logmdct,
		   logmask);
      
      /* are we managing bitrate?  If so, perform two more fits for
         later rate tweaking (fits represent hi/lo) */
      if(vorbis_bitrate_managed(vb) && floor_posts[i][PACKETBLOBS/2]){
	/* higher rate by way of lower noise curve */
	_vp_offset_and_mix(look->psy_look[blocktype],
			   noise,
			   tone,
			   2,
			   logmask);

#if 0
	if(vi->channels==2)
	  if(i==0)
	    _analysis_output_always("mask2L",seq,logmask,n/2,1,0,0);
	  else
	    _analysis_output_always("mask2R",seq,logmask,n/2,1,0,0);
#endif

	floor_posts[i][PACKETBLOBS-1]=
	  floor1_fit(vb,look->floor_look[submap],
		     logmdct,
		     logmask);
      
	/* lower rate by way of higher noise curve */
	_vp_offset_and_mix(look->psy_look[blocktype],
			   noise,
			   tone,
			   0,
			   logmask);

#if 0
	if(vi->channels==2)
	  if(i==0)
	    _analysis_output_always("mask0L",seq,logmask,n/2,1,0,0);
	  else
	    _analysis_output_always("mask0R",seq,logmask,n/2,1,0,0);
#endif

	floor_posts[i][0]=
	  floor1_fit(vb,look->floor_look[submap],
		     logmdct,
		     logmask);
	
	/* we also interpolate a range of intermediate curves for
           intermediate rates */
	for(k=1;k<PACKETBLOBS/2;k++)
	  floor_posts[i][k]=
	    floor1_interpolate_fit(vb,look->floor_look[submap],
				   floor_posts[i][0],
				   floor_posts[i][PACKETBLOBS/2],
				   k*65536/(PACKETBLOBS/2));
	for(k=PACKETBLOBS/2+1;k<PACKETBLOBS-1;k++)
	  floor_posts[i][k]=
	    floor1_interpolate_fit(vb,look->floor_look[submap],
				   floor_posts[i][PACKETBLOBS/2],
				   floor_posts[i][PACKETBLOBS-1],
				   (k-PACKETBLOBS/2)*65536/(PACKETBLOBS/2));
      }
    }
  }
  vbi->ampmax=global_ampmax;

  /* now save the bit cursor in the write buffer */
  setup_bits=oggpack_bits(&vb->opb);

  /*
    the next phases are performed once for vbr-only and PACKETBLOB
    times for bitrate managed modes.
    
    1) reset write buffer
    2) encode the floor for each channel, compute coded mask curve/res
    3) normalize and couple.
    4) encode residue
    5) save packet bytes to the packetblob vector
    
  */

  /* iterate over the many masking curve fits we've created */

  {
    float **res_bundle=alloca(sizeof(*res_bundle)*vi->channels);
    float **couple_bundle=alloca(sizeof(*couple_bundle)*vi->channels);
    int *zerobundle=alloca(sizeof(*zerobundle)*vi->channels);

    float **mag_memo=
      _vp_quantize_couple_memo(vb,
			       look->psy_look[blocktype],
			       info,
			       gmdct);    
    
    for(k=(vorbis_bitrate_managed(vb)?0:PACKETBLOBS/2);
	k<=(vorbis_bitrate_managed(vb)?PACKETBLOBS-1:PACKETBLOBS/2);
	k++){

      /* Abuse the bitpacker abstraction slightly by storing multiple
         packets in one packbuffer. */      
      if(vorbis_bitrate_managed(vb) && k){
	oggpack_writecopy(&vb->opb,
			  oggpack_get_buffer(&vb->opb),
			  setup_bits);
      }

      /* encode floor, compute masking curve, sep out residue */
      for(i=0;i<vi->channels;i++){
	int submap=info->chmuxlist[i];
	float *mdct    =gmdct[i];
	float *res     =vb->pcm[i];
	int   *ilogmask=ilogmaskch[i]=
	  _vorbis_block_alloc(vb,n/2*sizeof(**gmdct));
      
	nonzero[i]=floor1_encode(vb,look->floor_look[submap],
				 floor_posts[i][k],
				 ilogmask);
#if 0
	{
	  char buf[80];
	  sprintf(buf,"maskI%d",k);
	  _analysis_output_always(buf,seq+i,mask,n/2,1,1,0);
	}
#endif
	_vp_remove_floor(look->psy_look[blocktype],
			 mdct,
			 ilogmask,
			 res);

#if 0
	{
	  char buf[80];
	  sprintf(buf,"resI%d",k);
	  _analysis_output_always(buf,seq+i,res,n/2,1,1,0);
	}
#endif
      }
      
      /* our iteration is now based on masking curve, not prequant and
	 coupling.  Only one prequant/coupling step */
      
      /* quantize/couple */
      _vp_quantize_couple(look->psy_look[blocktype],
			  info,
			  vb->pcm,
			  mag_memo,
			  ilogmaskch,
			  nonzero
			  );
      
      /* classify and encode by submap */
      for(i=0;i<info->submaps;i++){
	int ch_in_bundle=0;
	long **classifications;

	for(j=0;j<vi->channels;j++){
	  if(info->chmuxlist[j]==i){
	    zerobundle[ch_in_bundle]=0;
	    if(nonzero[j])zerobundle[ch_in_bundle]=1;
	    res_bundle[ch_in_bundle]=vb->pcm[j];
	    couple_bundle[ch_in_bundle++]=vb->pcm[j]+n/2;
	  }
	}
	
	classifications=look->residue_func[i]->
	  class(vb,look->residue_look[i],
		res_bundle,zerobundle,ch_in_bundle);
	
	look->residue_func[i]->
	  forward(vb,look->residue_look[i],
		  couple_bundle,NULL,zerobundle,ch_in_bundle,classifications);
      }
      
      /* ok, done encoding.  Mark this protopacket and prepare next. */
      oggpack_writealign(&vb->opb);
      vbi->packetblob_markers[k]=oggpack_bytes(&vb->opb);
      
    }
    
    seq++;
  } 

  total+=ci->blocksizes[vb->W]/4+ci->blocksizes[vb->nW]/4;
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
  /*vorbis_info_mode     *mode=look->mode;*/
  int                   i,j;
  long                  n=vb->pcmend=ci->blocksizes[vb->W];

  float **pcmbundle=alloca(sizeof(*pcmbundle)*vi->channels);
  int    *zerobundle=alloca(sizeof(*zerobundle)*vi->channels);

  int   *nonzero  =alloca(sizeof(*nonzero)*vi->channels);
  void **floormemo=alloca(sizeof(*floormemo)*vi->channels);
  
  /* recover the spectral envelope; store it in the PCM vector for now */
  for(i=0;i<vi->channels;i++){
    int submap=info->chmuxlist[i];
    floormemo[i]=look->floor_func[submap]->
      inverse1(vb,look->floor_look[submap]);
    if(floormemo[i])
      nonzero[i]=1;
    else
      nonzero[i]=0;      
    memset(vb->pcm[i],0,sizeof(*vb->pcm[i])*n/2);
  }

  /* channel coupling can 'dirty' the nonzero listing */
  for(i=0;i<info->coupling_steps;i++){
    if(nonzero[info->coupling_mag[i]] ||
       nonzero[info->coupling_ang[i]]){
      nonzero[info->coupling_mag[i]]=1; 
      nonzero[info->coupling_ang[i]]=1; 
    }
  }

  /* recover the residue into our working vectors */
  for(i=0;i<info->submaps;i++){
    int ch_in_bundle=0;
    for(j=0;j<vi->channels;j++){
      if(info->chmuxlist[j]==i){
	if(nonzero[j])
	  zerobundle[ch_in_bundle]=1;
	else
	  zerobundle[ch_in_bundle]=0;
	pcmbundle[ch_in_bundle++]=vb->pcm[j];
      }
    }
    
    look->residue_func[i]->inverse(vb,look->residue_look[i],
				   pcmbundle,zerobundle,ch_in_bundle);
  }

  /* channel coupling */
  for(i=info->coupling_steps-1;i>=0;i--){
    float *pcmM=vb->pcm[info->coupling_mag[i]];
    float *pcmA=vb->pcm[info->coupling_ang[i]];

    for(j=0;j<n/2;j++){
      float mag=pcmM[j];
      float ang=pcmA[j];

      if(mag>0)
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag-ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag+ang;
	}
      else
	if(ang>0){
	  pcmM[j]=mag;
	  pcmA[j]=mag+ang;
	}else{
	  pcmA[j]=mag;
	  pcmM[j]=mag-ang;
	}
    }
  }

  /* compute and apply spectral envelope */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    int submap=info->chmuxlist[i];
    look->floor_func[submap]->
      inverse2(vb,look->floor_look[submap],floormemo[i],pcm);
    //_analysis_output_always("out",seq++,pcm,n/2,1,1,0);
  }

  /* transform the PCM data; takes PCM vector, vb; modifies PCM vector */
  /* only MDCT right now.... */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    mdct_backward(b->transform[vb->W][0],pcm,pcm);
  }

  /* window the data */
  for(i=0;i<vi->channels;i++){
    float *pcm=vb->pcm[i];
    if(nonzero[i])
      _vorbis_apply_window(pcm,b->window,ci->blocksizes,vb->lW,vb->W,vb->nW);
    else
      for(j=0;j<n;j++)
	pcm[j]=0.f;

  }

  /* all done! */
  return(0);
}

/* export hooks */
vorbis_func_mapping mapping0_exportbundle={
  &mapping0_pack,
  &mapping0_unpack,
  &mapping0_look,
  &mapping0_copy_info,
  &mapping0_free_info,
  &mapping0_free_look,
  &mapping0_forward,
  &mapping0_inverse
};

