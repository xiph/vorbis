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

 function: simple utility that runs audio through the psychoacoustics
           without encoding
 last mod: $Id: psytune.c,v 1.11 2000/12/21 21:04:40 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vorbis/codec.h"
#include "os.h"
#include "psy.h"
#include "mdct.h"
#include "window.h"
#include "scales.h"
#include "lpc.h"
#include "lsp.h"

static vorbis_info_psy _psy_set0={
  1,/*athp*/
  0,/*decayp*/
  1,/*smoothp*/
  0,.2,

  -100.,
  -140.,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

   0,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {{-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*63*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*88*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*125*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*175*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*250*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*350*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*500*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*700*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*1000*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*1400*/
   {-40.,-40.,-40.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*2000*/
   {-40.,-40.,-40.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*2800*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*4000*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*5600*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*8000*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*11500*/
   {-30.,-35.,-35.,-40.,-40.,-50.,-60.,-70.,-80.,-90.,-100.}, /*16000*/
  },

  1,/* peakattp */
  {{-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-22.,-22.}, /*63*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-22.,-22.}, /*88*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-22.,-22.}, /*125*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-18.,-20.}, /*8000*/
   { -7., -8., -9.,-10.,-10.,-11.,-12.,-13.,-15.,-16.,-17.}, /*8000*/
   { -6., -7., -9., -9., -9., -9.,-10.,-11.,-12.,-13.,-14.}, /*11500*/
   { -6., -6., -9., -9., -9., -9., -9., -9.,-10.,-11.,-12.}, /*16000*/
  },


  0,/*noisemaskp */
  /*  0   10   20   30   40   50   60    70    80    90   100 */
  {{-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*63*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*88*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*125*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*175*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*250*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*350*/
   {-99.,-99.,-99.,-99.,-99.,-99.,-99.,-199.,-199.,-199.,-199.}, /*500*/
   {  6.,  6.,  6.,  2.,  2.,  2.,  2.,   2.,   2.,   1.,   0.}, /*700*/

   {  6.,  6.,  6.,  5.,  5.,  5.,  5.,   5.,   5.,   4.,   3.}, /*1000*/
   {  6.,  6.,  6.,  5.,  5.,  5.,  5.,   5.,   5.,   4.,   3.}, /*1400*/
   {  6.,  6.,  6.,  5.,  5.,  5.,  5.,   5.,   5.,   4.,   3.}, /*2000*/
   {  6.,  6.,  6.,  5.,  5.,  5.,  5.,   5.,   5.,   4.,   3.}, /*2800*/
   {  6.,  6.,  6.,  5.,  5.,  5.,  5.,   5.,   5.,   4.,   3.}, /*4000*/
   { 10., 10., 10., 10., 10., 10.,  8.,   8.,   6.,   6.,   6.}, /*5600*/
   { 10., 10., 10., 10., 10., 10.,  8.,   8.,   8.,   8.,   8.}, /*8000*/
   { 10., 10., 10., 10., 10., 10., 10.,  10.,  10.,  10.,  10.}, /*11500*/
   { 10., 10., 10., 10., 10., 10., 10.,  10.,  10.,  10.,  10.}, /*16000*/
  },
 
  100.,

  -0., -.004   /* attack/decay control */
};

static int noisy=0;
void analysis(char *base,int i,float *v,int n,int bark,int dB){
  if(noisy){
    int j;
    FILE *of;
    char buffer[80];
    sprintf(buffer,"%s_%d.m",base,i);
    of=fopen(buffer,"w");

    for(j=0;j<n;j++){
      if(dB && v[j]==0)
	  fprintf(of,"\n\n");
      else{
	if(bark)
	  fprintf(of,"%g ",toBARK(22050.f*j/n));
	else
	  fprintf(of,"%g ",(float)j);
      
	if(dB){
	  fprintf(of,"%g\n",todB(fabs(v[j])));
	}else{
	  fprintf(of,"%g\n",v[j]);
	}
      }
    }
    fclose(of);
  }
}

typedef struct {
  long n;
  int ln;
  int  m;
  int *linearmap;

  lpc_lookup lpclook;
} vorbis_look_floor0;

long frameno=0;

/* hacked from floor0.c */
static void floorinit(vorbis_look_floor0 *look,int n,int m,int ln){
  int j;
  float scale;
  look->m=m;
  look->n=n;
  look->ln=ln;
  lpc_init(&look->lpclook,look->ln,look->m);

  scale=look->ln/toBARK(22050.f);

  look->linearmap=_ogg_malloc(look->n*sizeof(int));
  for(j=0;j<look->n;j++){
    int val=floor( toBARK(22050.f/n*j) *scale);
    if(val>look->ln)val=look->ln;
    look->linearmap[j]=val;
  }
}

int main(int argc,char *argv[]){
  int eos=0;
  float nonz=0.f;
  float acc=0.f;
  float tot=0.f;

  int framesize=2048;
  int order=32;
  int map=256;

  float *pcm[2],*out[2],*window,*decay[2],*lpc,*floor,*mask;
  signed char *buffer,*buffer2;
  mdct_lookup m_look;
  vorbis_look_psy p_look;
  long i,j,k;

  vorbis_look_floor0 floorlook;

  int ath=0;
  int decayp=0;

  argv++;
  while(*argv){
    if(*argv[0]=='-'){
      /* option */
      if(argv[0][1]=='v'){
	noisy=0;
      }
      if(argv[0][1]=='A'){
	ath=0;
      }
      if(argv[0][1]=='D'){
	decayp=0;
      }
      if(argv[0][1]=='X'){
	ath=0;
	decayp=0;
      }
    }else
      if(*argv[0]=='+'){
	/* option */
	if(argv[0][1]=='v'){
	  noisy=1;
	}
	if(argv[0][1]=='A'){
	  ath=1;
	}
	if(argv[0][1]=='D'){
	  decayp=1;
	}
	if(argv[0][1]=='X'){
	  ath=1;
	  decayp=1;
	}
      }else
	framesize=atoi(argv[0]);
    argv++;
  }
  
  mask=_ogg_malloc(framesize*sizeof(float));
  pcm[0]=_ogg_malloc(framesize*sizeof(float));
  pcm[1]=_ogg_malloc(framesize*sizeof(float));
  out[0]=_ogg_calloc(framesize/2,sizeof(float));
  out[1]=_ogg_calloc(framesize/2,sizeof(float));
  decay[0]=_ogg_calloc(framesize/2,sizeof(float));
  decay[1]=_ogg_calloc(framesize/2,sizeof(float));
  floor=_ogg_malloc(framesize*sizeof(float));
  lpc=_ogg_malloc(order*sizeof(float));
  buffer=_ogg_malloc(framesize*4);
  buffer2=buffer+framesize*2;
  window=_vorbis_window(0,framesize,framesize/2,framesize/2);
  mdct_init(&m_look,framesize);
  _vp_psy_init(&p_look,&_psy_set0,framesize/2,44100);
  floorinit(&floorlook,framesize/2,order,map);

  for(i=0;i<P_BANDS;i++)
    for(j=0;j<P_LEVELS;j++)
      analysis("Ptonecurve",i*100+j,p_look.tonecurves[i][j],EHMER_MAX,0,1);

  /* we cheat on the WAV header; we just bypass 44 bytes and never
     verify that it matches 16bit/stereo/44.1kHz. */
  
  fread(buffer,1,44,stdin);
  fwrite(buffer,1,44,stdout);
  memset(buffer,0,framesize*2);

  analysis("window",0,window,framesize,0,0);

  fprintf(stderr,"Processing for frame size %d...\n",framesize);

  while(!eos){
    long bytes=fread(buffer2,1,framesize*2,stdin); 
    if(bytes<framesize*2)
      memset(buffer2+bytes,0,framesize*2-bytes);
    
    if(bytes!=0){

      /* uninterleave samples */
      for(i=0;i<framesize;i++){
        pcm[0][i]=((buffer[i*4+1]<<8)|
                      (0x00ff&(int)buffer[i*4]))/32768.f;
        pcm[1][i]=((buffer[i*4+3]<<8)|
		   (0x00ff&(int)buffer[i*4+2]))/32768.f;
      }
      
      for(i=0;i<2;i++){
	float amp;

	analysis("pre",frameno,pcm[i],framesize,0,0);
	
	/* do the psychacoustics */
	for(j=0;j<framesize;j++)
	  pcm[i][j]*=window[j];

	memcpy(mask,pcm[i],sizeof(float)*framesize);

	mdct_forward(&m_look,pcm[i],pcm[i]);

	analysis("mdct",frameno,pcm[i],framesize/2,1,1);

	_vp_compute_mask(&p_look,pcm[i],floor,decay[i]);
	
	analysis("floor",frameno,floor,framesize/2,1,1);
	analysis("decay",frameno,decay[i],framesize/2,1,1);

	_vp_apply_floor(&p_look,pcm[i],floor);


	analysis("quant",frameno,pcm[i],framesize/2,1,1);

	/* re-add floor */
	for(j=0;j<framesize/2;j++){
	  float val=rint(pcm[i][j]);
	  tot++;
	  if(val){
	    nonz++;
	    acc+=log(fabs(val)*2.f+1.f)/log(2);
	    pcm[i][j]=val*floor[j];
	  }else{
	    pcm[i][j]=0.f;
	  }
	}
	
	analysis("final",frameno,pcm[i],framesize/2,1,1);

	/* take it back to time */
	mdct_backward(&m_look,pcm[i],pcm[i]);
	for(j=0;j<framesize/2;j++)
	  out[i][j]+=pcm[i][j]*window[j];

	frameno++;
      }
           
      /* write data.  Use the part of buffer we're about to shift out */
      for(i=0;i<2;i++){
	char  *ptr=buffer+i*2;
	float *mono=out[i];
	for(j=0;j<framesize/2;j++){
	  int val=mono[j]*32767.;
	  /* might as well guard against clipping */
	  if(val>32767)val=32767;
	  if(val<-32768)val=-32768;
	  ptr[0]=val&0xff;
	  ptr[1]=(val>>8)&0xff;
	  ptr+=4;
	}
      }
 
      fprintf(stderr,"*");
      fwrite(buffer,1,framesize*2,stdout);
      memmove(buffer,buffer2,framesize*2);

      for(i=0;i<2;i++){
	for(j=0,k=framesize/2;j<framesize/2;j++,k++)
	  out[i][j]=pcm[i][k]*window[k];
      }
    }else
      eos=1;
  }
  fprintf(stderr,"average raw bits of entropy: %.03g/sample\n",acc/tot);
  fprintf(stderr,"average nonzero samples: %.03g/%d\n",nonz/tot*framesize/2,
	  framesize/2);
  fprintf(stderr,"Done\n\n");
  return 0;
}
