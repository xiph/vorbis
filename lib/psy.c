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

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.67.2.1 2002/05/07 23:47:14 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "vorbis/codec.h"
#include "codec_internal.h"

#include "masking.h"
#include "psy.h"
#include "os.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

#define NEGINF -9999.f

/* Why Bark scale for encoding but not masking computation? Because
   masking has a strong harmonic dependency */

vorbis_look_psy_global *_vp_global_look(vorbis_info *vi){
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;
  vorbis_look_psy_global *look=_ogg_calloc(1,sizeof(*look));

  look->channels=vi->channels;

  look->ampmax=-9999.;
  look->gi=gi;
  return(look);
}

void _vp_global_free(vorbis_look_psy_global *look){
  if(look){
    memset(look,0,sizeof(*look));
    _ogg_free(look);
  }
}

void _vi_gpsy_free(vorbis_info_psy_global *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(*i));
    _ogg_free(i);
  }
}

vorbis_info_psy *_vi_psy_copy(vorbis_info_psy *i){
  vorbis_info_psy *ret=_ogg_malloc(sizeof(*ret));
  memcpy(ret,i,sizeof(*ret));
  return(ret);
}

/* Set up decibel threshold slopes on a Bark frequency scale */
/* ATH is the only bit left on a Bark scale.  No reason to change it
   right now */
static void set_curve(float *ref,float *c,int n, float crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK((float)(i+1))*2*n/crate);
    float base=ref[i];
    if(j<endpos){
      float delta=(ref[i+1]-base)/(endpos-j);
      for(;j<endpos && j<n;j++){
	c[j]=base;
	base+=delta;
      }
    }
  }
}

static void min_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]<c[i])c[i]=c2[i];
}
static void max_curve(float *c,
		       float *c2){
  int i;  
  for(i=0;i<EHMER_MAX;i++)if(c2[i]>c[i])c[i]=c2[i];
}

static void attenuate_curve(float *c,float att){
  int i;
  for(i=0;i<EHMER_MAX;i++)
    c[i]+=att;
}

extern int analysis_noisy;

static void odd_decade_level_interpolate(float **c){
  int i,j;

  for(i=1;i<P_LEVELS;i+=2)
    for(j=0;j<EHMER_MAX;j++)
      if(c[i-1][j+2]>-200 || c[i+1][j+2]>-200){
	c[i][j+2]=(c[i-1][j+2]+c[i+1][j+2])/2;
      }else{
	c[i][j+2]=-900;
      }
}

static void setup_curve(float **c,
			int band,
			float *curveatt_dB){
  int i,j;
  float ath[EHMER_MAX];
  float tempc[P_LEVELS][EHMER_MAX];
  float *ATH=ATH_Bark_dB_lspconservative; /* just for limiting here */

  /* we add back in the ATH to avoid low level curves falling off to
     -infinity and unnecessarily cutting off high level curves in the
     curve limiting (last step).  But again, remember... a half-band's
     settings must be valid over the whole band, and it's better to
     mask too little than too much, so be pessimistical. */

  for(i=0;i<EHMER_MAX;i++){
    float oc_min=band*.5+(i-EHMER_OFFSET)*.125;
    float oc_max=band*.5+(i-EHMER_OFFSET+1)*.125;
    float bark=toBARK(fromOC(oc_min));
    int ibark=floor(bark);
    float del=bark-ibark;
    float ath_min,ath_max;

    if(ibark<26)
      ath_min=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_min=ATH[25];

    bark=toBARK(fromOC(oc_max));
    ibark=floor(bark);
    del=bark-ibark;

    if(ibark<26)
      ath_max=ATH[ibark]*(1.f-del)+ATH[ibark+1]*del;
    else
      ath_max=ATH[25];

    ath[i]=min(ath_min,ath_max);
  }

  /* normalize curves so the driving amplitude is 0dB */
  /* make temp curves with the ATH overlayed */
  for(i=0;i<P_LEVELS;i++){
    attenuate_curve(c[i]+2,curveatt_dB[i]);
    memcpy(tempc[i],ath,EHMER_MAX*sizeof(*tempc[i]));
    attenuate_curve(tempc[i],-i*10.f);
    max_curve(tempc[i],c[i]+2);
  }
  
  /* Now limit the louder curves.

     the idea is this: We don't know what the playback attenuation
     will be; 0dB SL moves every time the user twiddles the volume
     knob. So that means we have to use a single 'most pessimal' curve
     for all masking amplitudes, right?  Wrong.  The *loudest* sound
     can be in (we assume) a range of ...+100dB] SL.  However, sounds
     20dB down will be in a range ...+80], 40dB down is from ...+60],
     etc... */

  for(j=1;j<P_LEVELS;j++){
    min_curve(tempc[j],tempc[j-1]);
    min_curve(c[j]+2,tempc[j]);
  }

  /* add fenceposts */
  for(j=0;j<P_LEVELS;j++){

    for(i=0;i<EHMER_OFFSET;i++)
      if(c[j][i+2]>-200.f)break;  
    c[j][0]=i;

    for(i=EHMER_MAX-1;i>EHMER_OFFSET+1;i--)
      if(c[j][i+2]>-200.f)
	break;
    c[j][1]=i;

  }

}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,
		  vorbis_info_psy_global *gi,int n,long rate){
  long i,j,k,lo=-99,hi=0;
  long maxoc;
  memset(p,0,sizeof(*p));


  p->eighth_octave_lines=gi->eighth_octave_lines;
  p->shiftoc=rint(log(gi->eighth_octave_lines*8.f)/log(2.f))-1;

  p->firstoc=toOC(.25f*rate/n)*(1<<(p->shiftoc+1))-gi->eighth_octave_lines;
  maxoc=toOC((n*.5f-.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;
  p->total_octave_lines=maxoc-p->firstoc+1;

  if(vi->ath)
    p->ath=_ogg_malloc(n*sizeof(*p->ath));
  p->octave=_ogg_malloc(n*sizeof(*p->octave));
  p->bark=_ogg_malloc(n*sizeof(*p->bark));
  p->vi=vi;
  p->n=n;
  p->rate=rate;

  /* set up the lookups for a given blocksize and sample rate */
  if(vi->ath)
    set_curve(vi->ath, p->ath,n,(float)rate);
  for(i=0;i<n;i++){
    float bark=toBARK(rate/(2*n)*i); 

    for(;lo+vi->noisewindowlomin<i && 
	  toBARK(rate/(2*n)*lo)<(bark-vi->noisewindowlo);lo++);
    
    for(;hi<n && (hi<i+vi->noisewindowhimin ||
	  toBARK(rate/(2*n)*hi)<(bark+vi->noisewindowhi));hi++);
    
    p->bark[i]=(lo<<16)+hi;

  }

  for(i=0;i<n;i++)
    p->octave[i]=toOC((i*.5f+.25f)*rate/n)*(1<<(p->shiftoc+1))+.5f;

  p->tonecurves=_ogg_malloc(P_BANDS*sizeof(*p->tonecurves));
  for(i=0;i<P_BANDS;i++)
    p->tonecurves[i]=_ogg_malloc(P_LEVELS*sizeof(*p->tonecurves[i]));
  
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      p->tonecurves[i][j]=_ogg_malloc((EHMER_MAX+2)*sizeof(*p->tonecurves[i][j]));
  

  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[0][i]+2,tone_125[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[2][i]+2,tone_125[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[4][i]+2,tone_250[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[6][i]+2,tone_500[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[8][i]+2,tone_1000[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[10][i]+2,tone_2000[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[12][i]+2,tone_4000[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[14][i]+2,tone_8000[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);
  for(i=0;i<P_LEVELS;i+=2)
    memcpy(p->tonecurves[16][i]+2,tone_16000[i<4?0:i/2-2],sizeof(***p->tonecurves)*EHMER_MAX);

  for(i=0;i<P_BANDS;i+=2)
    for(j=0;j<P_LEVELS;j+=2)
      for(k=2;k<EHMER_MAX+2;k++)
	p->tonecurves[i][j][k]+=vi->tone_masteratt;

  for(i=0;i<P_BANDS;i+=2)
    odd_decade_level_interpolate(p->tonecurves[i]);

  /* interpolate curves between */
  for(i=1;i<P_BANDS;i+=2)
    for(j=0;j<P_LEVELS;j++){
      memcpy(p->tonecurves[i][j]+2,p->tonecurves[i-1][j]+2,EHMER_MAX*sizeof(***p->tonecurves));
      /*interp_curve(p->tonecurves[i][j],
		   p->tonecurves[i-1][j],
		   p->tonecurves[i+1][j],.5);*/
      min_curve(p->tonecurves[i][j]+2,p->tonecurves[i+1][j]+2);
    }

  analysis_noisy=0;
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_1.4kHz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_2.8kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("precurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  /* set up the final curves */
  for(i=0;i<P_BANDS;i++)
    setup_curve(p->tonecurves[i],i,vi->toneatt.block[i]);

  analysis_noisy=0;
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
     _analysis_output("curve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("curve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  if(vi->curvelimitp){
    /* value limit the tonal masking curves; the peakatt not only
       optionally specifies maximum dynamic depth, but also
       limits the masking curves to a minimum depth  */
    for(i=0;i<P_BANDS;i++)
      for(j=0;j<P_LEVELS;j++){
	for(k=2;k<EHMER_OFFSET+2+vi->curvelimitp;k++)
	  if(p->tonecurves[i][j][k]> vi->peakatt.block[i][j])
	    p->tonecurves[i][j][k]=  vi->peakatt.block[i][j];
	  else
	    break;
      }
  }

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("licurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  if(vi->peakattp) /* we limit maximum depth only optionally */
    for(i=0;i<P_BANDS;i++)
      for(j=0;j<P_LEVELS;j++)
	if(p->tonecurves[i][j][EHMER_OFFSET+2]< vi->peakatt.block[i][j])
	  p->tonecurves[i][j][EHMER_OFFSET+2]=  vi->peakatt.block[i][j];

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("pcurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);

  /* but guarding is mandatory */
  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      if(p->tonecurves[i][j][EHMER_OFFSET+2]< vi->tone_guard)
	  p->tonecurves[i][j][EHMER_OFFSET+2]=  vi->tone_guard;

  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_63Hz",i,p->tonecurves[0][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_88Hz",i,p->tonecurves[1][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_125Hz",i,p->tonecurves[2][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_170Hz",i,p->tonecurves[3][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_250Hz",i,p->tonecurves[4][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_350Hz",i,p->tonecurves[5][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_500Hz",i,p->tonecurves[6][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_700Hz",i,p->tonecurves[7][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_1kHz",i,p->tonecurves[8][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_1.4Hz",i,p->tonecurves[9][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_2kHz",i,p->tonecurves[10][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_2.4kHz",i,p->tonecurves[11][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_4kHz",i,p->tonecurves[12][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_5.6kHz",i,p->tonecurves[13][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_8kHz",i,p->tonecurves[14][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_11.5kHz",i,p->tonecurves[15][i]+2,EHMER_MAX,0,0);
  for(i=0;i<P_LEVELS;i++)
    _analysis_output("fcurve_16kHz",i,p->tonecurves[16][i]+2,EHMER_MAX,0,0);
  analysis_noisy=0;

  /* set up rolling noise median */
  p->noiseoffset=_ogg_malloc(P_NOISECURVES*sizeof(*p->noiseoffset));
  for(i=0;i<P_NOISECURVES;i++)
    p->noiseoffset[i]=_ogg_malloc(n*sizeof(**p->noiseoffset));
  
  for(i=0;i<n;i++){
    float halfoc=toOC((i+.5)*rate/(2.*n))*2.;
    int inthalfoc;
    float del;
    
    if(halfoc<0)halfoc=0;
    if(halfoc>=P_BANDS-1)halfoc=P_BANDS-1;
    inthalfoc=(int)halfoc;
    del=halfoc-inthalfoc;
    
    for(j=0;j<P_NOISECURVES;j++)
      p->noiseoffset[j][i]=
	p->vi->noiseoff[j][inthalfoc]*(1.-del) + 
	p->vi->noiseoff[j][inthalfoc+1]*del;
    
  }

  analysis_noisy=0;
  //_analysis_output_always("noiseoff0",n,p->noiseoffset[0],n,1,0,0);
  //_analysis_output_always("noiseoff1",n,p->noiseoffset[1],n,1,0,0);
  //_analysis_output_always("noiseoff2",n,p->noiseoffset[2],n,1,0,0);
  analysis_noisy=1;

}

void _vp_psy_clear(vorbis_look_psy *p){
  int i,j;
  if(p){
    if(p->ath)_ogg_free(p->ath);
    if(p->octave)_ogg_free(p->octave);
    if(p->bark)_ogg_free(p->bark);
    if(p->tonecurves){
      for(i=0;i<P_BANDS;i++){
	for(j=0;j<P_LEVELS;j++){
	  _ogg_free(p->tonecurves[i][j]);
	}
	_ogg_free(p->tonecurves[i]);
      }
      _ogg_free(p->tonecurves);
    }
    _ogg_free(p->noiseoffset);
    memset(p,0,sizeof(*p));
  }
}

/* octave/(8*eighth_octave_lines) x scale and dB y scale */
static void seed_curve(float *seed,
		       const float **curves,
		       float amp,
		       int oc, int n,
		       int linesper,float dBoffset){
  int i,post1;
  int seedptr;
  const float *posts,*curve;

  int choice=(int)((amp+dBoffset)*.1f);
  choice=max(choice,0);
  choice=min(choice,P_LEVELS-1);
  posts=curves[choice];
  curve=posts+2;
  post1=(int)posts[1];
  seedptr=oc+(posts[0]-16)*linesper-(linesper>>1);

  for(i=posts[0];i<post1;i++){
    if(seedptr>0){
      float lin=amp+curve[i];
      if(seed[seedptr]<lin)seed[seedptr]=lin;
    }
    seedptr+=linesper;
    if(seedptr>=n)break;
  }
}

static void seed_loop(vorbis_look_psy *p,
		      const float ***curves,
		      const float *f, 
		      const float *flr,
		      float *seed,
		      float specmax){
  vorbis_info_psy *vi=p->vi;
  long n=p->n,i;
  float dBoffset=vi->max_curve_dB-specmax;

  /* prime the working vector with peak values */

  for(i=0;i<n;i++){
    float max=f[i];
    long oc=p->octave[i];
    while(i+1<n && p->octave[i+1]==oc){
      i++;
      if(f[i]>max)max=f[i];
    }
    
    if(max+6.f>flr[i]){
      oc=oc>>p->shiftoc;
      if(oc>=P_BANDS)oc=P_BANDS-1;
      if(oc<0)oc=0;
      seed_curve(seed,
		 curves[oc],
		 max,
		 p->octave[i]-p->firstoc,
		 p->total_octave_lines,
		 p->eighth_octave_lines,
		 dBoffset);
    }
  }
}

static void seed_chase(float *seeds, int linesper, long n){
  long  *posstack=alloca(n*sizeof(*posstack));
  float *ampstack=alloca(n*sizeof(*ampstack));
  long   stack=0;
  long   pos=0;
  long   i;

  for(i=0;i<n;i++){
    if(stack<2){
      posstack[stack]=i;
      ampstack[stack++]=seeds[i];
    }else{
      while(1){
	if(seeds[i]<ampstack[stack-1]){
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;
	}else{
	  if(i<posstack[stack-1]+linesper){
	    if(stack>1 && ampstack[stack-1]<=ampstack[stack-2] &&
	       i<posstack[stack-2]+linesper){
	      /* we completely overlap, making stack-1 irrelevant.  pop it */
	      stack--;
	      continue;
	    }
	  }
	  posstack[stack]=i;
	  ampstack[stack++]=seeds[i];
	  break;

	}
      }
    }
  }

  /* the stack now contains only the positions that are relevant. Scan
     'em straight through */

  for(i=0;i<stack;i++){
    long endpos;
    if(i<stack-1 && ampstack[i+1]>ampstack[i]){
      endpos=posstack[i+1];
    }else{
      endpos=posstack[i]+linesper+1; /* +1 is important, else bin 0 is
					discarded in short frames */
    }
    if(endpos>n)endpos=n;
    for(;pos<endpos;pos++)
      seeds[pos]=ampstack[i];
  }
  
  /* there.  Linear time.  I now remember this was on a problem set I
     had in Grad Skool... I didn't solve it at the time ;-) */

}

/* bleaugh, this is more complicated than it needs to be */
static void max_seeds(vorbis_look_psy *p,
		      float *seed,
		      float *flr){
  long   n=p->total_octave_lines;
  int    linesper=p->eighth_octave_lines;
  long   linpos=0;
  long   pos;

  seed_chase(seed,linesper,n); /* for masking */
 
  pos=p->octave[0]-p->firstoc-(linesper>>1);
  while(linpos+1<p->n){
    float minV=seed[pos];
    long end=((p->octave[linpos]+p->octave[linpos+1])>>1)-p->firstoc;
    if(minV>p->vi->tone_abs_limit)minV=p->vi->tone_abs_limit;
    while(pos+1<=end){
      pos++;
      if((seed[pos]>NEGINF && seed[pos]<minV) || minV==NEGINF)
	minV=seed[pos];
    }
    
    /* seed scale is log.  Floor is linear.  Map back to it */
    end=pos+p->firstoc;
    for(;linpos<p->n && p->octave[linpos]<=end;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
  {
    float minV=seed[p->total_octave_lines-1];
    for(;linpos<p->n;linpos++)
      if(flr[linpos]<minV)flr[linpos]=minV;
  }
  
}

static void bark_noise_hybridmp(int n,const long *b,
                                const float *f,
                                float *noise,
                                const float offset,
                                const int fixed){
  
  float *N=alloca((n+1)*sizeof(*N));
  float *X=alloca((n+1)*sizeof(*N));
  float *XX=alloca((n+1)*sizeof(*N));
  float *Y=alloca((n+1)*sizeof(*N));
  float *XY=alloca((n+1)*sizeof(*N));

  float tN, tX, tXX, tY, tXY;
  float fi;
  int i;
  
  tN = tX = tXX = tY = tXY = 0.f;
  for (i = 0, fi = 0.f; i < n; i++, fi += 1.f) {
    float w, x, y;
    
    x = fi;
    y = f[i] + offset;
    if (y < 1.f) y = 1.f;
    w = y * y;
    N[i] = tN;
    X[i] = tX;
    XX[i] = tXX;
    Y[i] = tY;
    XY[i] = tXY;
    tN += w;
    tX += w * x;
    tXX += w * x * x;
    tY += w * y;
    tXY += w * x * y;
  }
  N[i] = tN;
  X[i] = tX;
  XX[i] = tXX;
  Y[i] = tY;
  XY[i] = tXY;
  
  for (i = 0, fi = 0.f;; i++, fi += 1.f) {
    int lo, hi;
    float R, A, B, D;
    
    lo = b[i] >> 16;
    if( lo>=0 ) break;
    hi = b[i] & 0xffff;
    
    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];    
    tXY = XY[hi] - XY[-lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;
    if (R < 0.f)
      R = 0.f;
    
    noise[i] = R - offset;
  }

  for ( ; i < n; i++, fi += 1.f) {
    int lo, hi;
    float R, A, B, D;
    
    lo = b[i] >> 16;
    hi = b[i] & 0xffff;
    
    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;
    if (R < 0.f) R = 0.f;
    
    noise[i] = R - offset;
  }
  
  if (fixed <= 0) return;
  
  for (i = 0, fi = 0.f; i < (fixed + 1) / 2; i++, fi += 1.f) {
    int lo, hi;
    float R, A, B, D;
    
    hi = i + fixed / 2;
    lo = hi - fixed;
    
    tN = N[hi] + N[-lo];
    tX = X[hi] - X[-lo];
    tXX = XX[hi] + XX[-lo];
    tY = Y[hi] + Y[-lo];
    tXY = XY[hi] - XY[-lo];
    
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;

    if (R > 0.f && R - offset < noise[i]) noise[i] = R - offset;
  }
  for ( ; i < n; i++, fi += 1.f) {
    int lo, hi;
    float R, A, B, D;
    
    hi = i + fixed / 2;
    lo = hi - fixed;
    
    tN = N[hi] - N[lo];
    tX = X[hi] - X[lo];
    tXX = XX[hi] - XX[lo];
    tY = Y[hi] - Y[lo];
    tXY = XY[hi] - XY[lo];
    
    A = tY * tXX - tX * tXY;
    B = tN * tXY - tX * tY;
    D = tN * tXX - tX * tX;
    R = (A + fi * B) / D;
    
    if (R > 0.f && R - offset < noise[i]) noise[i] = R - offset;
  }
}

static float FLOOR1_fromdB_INV_LOOKUP[256]={
  0.F, 8.81683e+06F, 8.27882e+06F, 7.77365e+06F, 
  7.29930e+06F, 6.85389e+06F, 6.43567e+06F, 6.04296e+06F, 
  5.67422e+06F, 5.32798e+06F, 5.00286e+06F, 4.69759e+06F, 
  4.41094e+06F, 4.14178e+06F, 3.88905e+06F, 3.65174e+06F, 
  3.42891e+06F, 3.21968e+06F, 3.02321e+06F, 2.83873e+06F, 
  2.66551e+06F, 2.50286e+06F, 2.35014e+06F, 2.20673e+06F, 
  2.07208e+06F, 1.94564e+06F, 1.82692e+06F, 1.71544e+06F, 
  1.61076e+06F, 1.51247e+06F, 1.42018e+06F, 1.33352e+06F, 
  1.25215e+06F, 1.17574e+06F, 1.10400e+06F, 1.03663e+06F, 
  973377.F, 913981.F, 858210.F, 805842.F, 
  756669.F, 710497.F, 667142.F, 626433.F, 
  588208.F, 552316.F, 518613.F, 486967.F, 
  457252.F, 429351.F, 403152.F, 378551.F, 
  355452.F, 333762.F, 313396.F, 294273.F, 
  276316.F, 259455.F, 243623.F, 228757.F, 
  214798.F, 201691.F, 189384.F, 177828.F, 
  166977.F, 156788.F, 147221.F, 138237.F, 
  129802.F, 121881.F, 114444.F, 107461.F, 
  100903.F, 94746.3F, 88964.9F, 83536.2F, 
  78438.8F, 73652.5F, 69158.2F, 64938.1F, 
  60975.6F, 57254.9F, 53761.2F, 50480.6F, 
  47400.3F, 44507.9F, 41792.0F, 39241.9F, 
  36847.3F, 34598.9F, 32487.7F, 30505.3F, 
  28643.8F, 26896.0F, 25254.8F, 23713.7F, 
  22266.7F, 20908.0F, 19632.2F, 18434.2F, 
  17309.4F, 16253.1F, 15261.4F, 14330.1F, 
  13455.7F, 12634.6F, 11863.7F, 11139.7F, 
  10460.0F, 9821.72F, 9222.39F, 8659.64F, 
  8131.23F, 7635.06F, 7169.17F, 6731.70F, 
  6320.93F, 5935.23F, 5573.06F, 5232.99F, 
  4913.67F, 4613.84F, 4332.30F, 4067.94F, 
  3819.72F, 3586.64F, 3367.78F, 3162.28F, 
  2969.31F, 2788.13F, 2617.99F, 2458.24F, 
  2308.24F, 2167.39F, 2035.14F, 1910.95F, 
  1794.35F, 1684.85F, 1582.04F, 1485.51F, 
  1394.86F, 1309.75F, 1229.83F, 1154.78F, 
  1084.32F, 1018.15F, 956.024F, 897.687F, 
  842.910F, 791.475F, 743.179F, 697.830F, 
  655.249F, 615.265F, 577.722F, 542.469F, 
  509.367F, 478.286F, 449.101F, 421.696F, 
  395.964F, 371.803F, 349.115F, 327.812F, 
  307.809F, 289.026F, 271.390F, 254.830F, 
  239.280F, 224.679F, 210.969F, 198.096F, 
  186.008F, 174.658F, 164.000F, 153.993F, 
  144.596F, 135.773F, 127.488F, 119.708F, 
  112.404F, 105.545F, 99.1046F, 93.0572F, 
  87.3788F, 82.0469F, 77.0404F, 72.3394F, 
  67.9252F, 63.7804F, 59.8885F, 56.2341F, 
  52.8027F, 49.5807F, 46.5553F, 43.7144F, 
  41.0470F, 38.5423F, 36.1904F, 33.9821F, 
  31.9085F, 29.9614F, 28.1332F, 26.4165F, 
  24.8045F, 23.2910F, 21.8697F, 20.5352F, 
  19.2822F, 18.1056F, 17.0008F, 15.9634F, 
  14.9893F, 14.0746F, 13.2158F, 12.4094F, 
  11.6522F, 10.9411F, 10.2735F, 9.64662F, 
  9.05798F, 8.50526F, 7.98626F, 7.49894F, 
  7.04135F, 6.61169F, 6.20824F, 5.82941F, 
  5.47370F, 5.13970F, 4.82607F, 4.53158F, 
  4.25507F, 3.99542F, 3.75162F, 3.52269F, 
  3.30774F, 3.10590F, 2.91638F, 2.73842F, 
  2.57132F, 2.41442F, 2.26709F, 2.12875F, 
  1.99885F, 1.87688F, 1.76236F, 1.65482F, 
  1.55384F, 1.45902F, 1.36999F, 1.28640F, 
  1.20790F, 1.13419F, 1.06499F, 1.F
};

void _vp_remove_floor(vorbis_look_psy *p,
		      float *mdct,
		      int *codedflr,
		      float *residue){ 
  int i,n=p->n;
  
  for(i=0;i<n;i++)
      residue[i]=mdct[i]*FLOOR1_fromdB_INV_LOOKUP[codedflr[i]];
}

void _vp_noisemask(vorbis_look_psy *p,
		   float *logmdct, 
		   float *logmask){

  int i,n=p->n;
  float *work=alloca(n*sizeof(*work));
  
  bark_noise_hybridmp(n,p->bark,logmdct,logmask,
		      140.,-1);

  for(i=0;i<n;i++)work[i]=logmdct[i]-logmask[i];

  bark_noise_hybridmp(n,p->bark,work,logmask,0.,
		      p->vi->noisewindowfixed);

  for(i=0;i<n;i++)work[i]=logmdct[i]-work[i];

  /* work[i] holds the median line (.5), logmask holds the upper
     envelope line (1.) */
  
  for(i=0;i<n;i++){
    int dB=logmask[i]+.5;
    if(dB>=NOISE_COMPAND_LEVELS)dB=NOISE_COMPAND_LEVELS-1;
    logmask[i]= work[i]+p->vi->noisecompand[dB];
  }
}

void _vp_tonemask(vorbis_look_psy *p,
		  float *logfft,
		  float *logmask,
		  float global_specmax,
		  float local_specmax){

  int i,n=p->n;

  float *seed=alloca(sizeof(*seed)*p->total_octave_lines);
  float att=local_specmax+p->vi->ath_adjatt;
  for(i=0;i<p->total_octave_lines;i++)seed[i]=NEGINF;
  
  /* set the ATH (floating below localmax, not global max by a
     specified att) */
  if(att<p->vi->ath_maxatt)att=p->vi->ath_maxatt;
  
  for(i=0;i<n;i++)
    logmask[i]=p->ath[i]+att;

  /* tone masking */
  seed_loop(p,(const float ***)p->tonecurves,logfft,logmask,seed,global_specmax);
  max_seeds(p,seed,logmask);

}


void _vp_offset_and_mix(vorbis_look_psy *p,
			float *noise,
			float *tone,
			int offset_select,
			float *logmask){
  int i,n=p->n;

  for(i=0;i<n;i++){
    logmask[i]= noise[i]+p->noiseoffset[offset_select][i];
    if(logmask[i]>p->vi->noisemaxsupp)logmask[i]=p->vi->noisemaxsupp;
    logmask[i]=max(logmask[i],tone[i]);
  }
}

float _vp_ampmax_decay(float amp,vorbis_dsp_state *vd){
  vorbis_info *vi=vd->vi;
  codec_setup_info *ci=vi->codec_setup;
  vorbis_info_psy_global *gi=&ci->psy_g_param;

  int n=ci->blocksizes[vd->W]/2;
  float secs=(float)n/vi->rate;

  amp+=secs*gi->ampmax_att_per_sec;
  if(amp<-9999)amp=-9999;
  return(amp);
}

static void couple_lossless(float A, float B, 
			    float *mag, float *ang){
  
  if(fabs(A)>fabs(B)){
    A=rint(A);B=rint(B);
    *mag=A; *ang=(A>0.f?A-B:B-A);
  }else{
    A=rint(A);B=rint(B);  
    *mag=B; *ang=(B>0.f?A-B:B-A);
  }

  if(*ang>fabs(*mag)*1.9999f){
    *ang= -fabs(*mag)*2.f;
    *mag= -*mag;
  }
}

static float hypot_lookup[32]={
  -0.009935, -0.011245, -0.012726, -0.014397, 
  -0.016282, -0.018407, -0.020800, -0.023494, 
  -0.026522, -0.029923, -0.033737, -0.038010, 
  -0.042787, -0.048121, -0.054064, -0.060671, 
  -0.068000, -0.076109, -0.085054, -0.094892, 
  -0.105675, -0.117451, -0.130260, -0.144134, 
  -0.159093, -0.175146, -0.192286, -0.210490, 
  -0.229718, -0.249913, -0.271001, -0.292893};

/* floorA and B are the pre-lookup logscale integers from floor1 decode */
static void precomputed_couple_point(float premag,
				     float A, float B,
				     int floorA,int floorB,
				     float *mag, float *ang){
  
  int test=(floorA>floorB)-1;
  int offset=31-abs(floorA-floorB);
  float floormag=hypot_lookup[((offset<0)-1)&offset]+1.f;
  
  floormag*=FLOOR1_fromdB_INV_LOOKUP[(floorB&test)|(floorA&(~test))];
  if(fabs(A)>fabs(B)){
    premag*=unitnorm(A);
  }else{
    premag*=unitnorm(B);
  }
  *mag=premag*floormag;
  *ang=0.f;
}

#if 0
static void couple_point(float A, float B, float fA, float fB, 
			 float granule,float igranule,
			 float fmag, float *mag, float *ang){

  if(fmag!=0.f){
    float corr=FAST_HYPOT(A*fA,B*fB)/FAST_HYPOT(fA,fB);
    
    if(fabs(A)>fabs(B)){
      *mag=A;
    }else{
      *mag=B;
    }

    *mag=unitnorm(*mag)*floor(corr*igranule+.5f)*granule; 
    *ang=0.f;

  }else{
    *mag=0.f;
    *ang=0.f;
  }    
}
#endif

/* just like below, this is currently set up to only do
   single-step-depth coupling.  Otherwise, we'd have to do more
   copying (which will be inevitable later) */
float **_vp_quantize_couple_memo(vorbis_block *vb,
				 vorbis_look_psy *p,
				 vorbis_info_mapping0 *vi,
				 float **mdct){
  
  int i,j,n=p->n;
  vorbis_info_psy *info=p->vi;
  float **ret=_vorbis_block_alloc(vb,vi->coupling_steps*sizeof(*ret));
  
  for(i=0;i<vi->coupling_steps;i++){
    float point=info->couple_pass.amppost_point;
    int limit=info->couple_pass.limit;
    ret[i]=0;
    if(point>0){
      float *mdctM=mdct[vi->coupling_mag[i]];
      float *mdctA=mdct[vi->coupling_ang[i]];
      ret[i]=_vorbis_block_alloc(vb,(n-limit)*sizeof(**ret));
      for(j=limit;j<n;j++)
	ret[i][j-limit]=FAST_HYPOT(mdctM[j],mdctA[j]);
    }
  }
  return(ret);
}

void _vp_quantize_couple(vorbis_look_psy *p,
			 vorbis_info_mapping0 *vi,
			 float **res,
			 float **mag_memo,
			 int   **ifloor,
			 int   *nonzero){

  int i,j,n=p->n;
  vorbis_info_psy *info=p->vi;

  /* perform any requested channel coupling */
  /* point stereo can only be used in a first stage (in this encoder)
     because of the dependency on floor lookups */
  for(i=0;i<vi->coupling_steps;i++){

    /* once we're doing multistage coupling in which a channel goes
       through more than one coupling step, the floor vector
       magnitudes will also have to be recalculated an propogated
       along with PCM.  Right now, we're not (that will wait until 5.1
       most likely), so the code isn't here yet. The memory management
       here is all assuming single depth couplings anyway. */

    /* make sure coupling a zero and a nonzero channel results in two
       nonzero channels. */
    if(nonzero[vi->coupling_mag[i]] ||
       nonzero[vi->coupling_ang[i]]){
     

      float *resM=res[vi->coupling_mag[i]];
      float *resA=res[vi->coupling_ang[i]];
      int *floorM=ifloor[vi->coupling_mag[i]];
      int *floorA=ifloor[vi->coupling_ang[i]];
      float *outM=res[vi->coupling_mag[i]]+n;
      float *outA=res[vi->coupling_ang[i]]+n;
      int limit=info->couple_pass.limit;
      float point=info->couple_pass.amppost_point;
 
      nonzero[vi->coupling_mag[i]]=1; 
      nonzero[vi->coupling_ang[i]]=1; 

      for(j=0;j<limit;j++){

	couple_lossless(resM[j],resA[j],outM+j,outA+j);
      }

      for(;j<p->n;j++){

	if(fabs(resM[j])<point && fabs(resA[j])<point){
	  precomputed_couple_point(mag_memo[i][j-limit],resM[j],resA[j],
				   floorM[j],floorA[j],
				   outM+j,outA+j);
	  
	}else{
	  couple_lossless(resM[j],resA[j],outM+j,outA+j);
	}
      }
    }
  }
}
