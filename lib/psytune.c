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

 function: simple utility that runs audio through the psychoacoustics
           without encoding
 last mod: $Id: psytune.c,v 1.1.2.1 2000/03/29 03:49:28 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vorbis/codec.h"
#include "psy.h"
#include "mdct.h"
#include "window.h"
#include "scales.h"
#include "lpc.h"

/*
   0      1      2      3      4     5      6     7     8     9 
   0,   100,  200,   300,   400,   510,   630,  770,  920, 1080,

   10    11    12     13     14     15     16    17    18    19
 1270, 1480, 1720,  2000,  2320,  2700,  3150, 3700, 4400, 5300,

   20    21    22     23     24     25     26 Bark
 6400, 7700, 9500, 12000, 15500, 20500, 27000 Hz    */

static vorbis_info_psy _psy_set0={
  /* ATH */
  { 70,  25,  15,  11,   9,   8, 7.5,   7,   7,   7,
     6,   4,   2,   0,  -3,  -5,  -6,  -6,-4.5, 2.5,
    11,  18, 21, 17, 25, 80,120}, 
  /* master attenuation */
  120.,

  /* mask1 attenuation proportion */
  { .7, .75, .80, .83, .84, .85, .85, .85, .85, .85,
    .00, .00, .00, .00, .00, .00, .00, .00, .00, .00,
    /*  .85, .85, .85, .85, .00, .00, .00, .00, .00, .00,*/
    .00, .00, .00, .00, .00, .00, .00},
  /* mask1 slope */
  {-40, -24, -12, -8., -4., -4, -4, -4, -4, -4,
   -99, -99, -99, -99, -99,-99,-99,-99,-99,-99,
   -99,-99,-99,-99,-99,-99,-99},

  /* mask2 attenuation proportion */
  { .60, .60, .60, .60, .55, .55, .50, .50, .50, .50,
    .50, .50, .50, .50, .50, .50, .50, .50, .50, .55,
    .60, .65, .66, .67, .68, .7, .7},      
  /* mask2 slope */
  {-40.,-24.,-11., -8., -6., -5.,-3.,-1.8, -1.8,-1.8,
   -1.8,-1.8,-1.9,-1.9,-2, -2, -2, -2,-2,-1.8,
   -1.7,-1.7,-1.6,-1.6,-1.6,-1.6,-1.6},

  -33., /* backward masking rolloff */

  .9998, .9997  /* attack/decay control */
};

static int noisy=0;
void analysis(char *base,int i,double *v,int n,int bark,int dB){
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
	  fprintf(of,"%g ",toBARK(22050.*j/n));
	else
	  fprintf(of,"%g ",(double)j);
      
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

long frameno=0;

int main(int argc,char *argv[]){
  int eos=0;
  double nonz=0.;
  double acc=0.;
  double tot=0.;

  int framesize=2048;
  int order=32;

  double *pcm[2],*out[2],*window,*mask,*maskwindow,*decay[2],*lpc,*floor;
  signed char *buffer,*buffer2;
  mdct_lookup m_look;
  vorbis_look_psy p_look;
  long i,j,k;

  drft_lookup fft_look;
  lpc_lookup lpc_look;

  int ath=0;
  int mask1p=0;
  int mask2p=0;
  int mask3p=0;
  int backp=0;
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
      if(argv[0][1]=='1'){
	mask1p=0;
      }
      if(argv[0][1]=='2'){
	mask2p=0;
      }
      if(argv[0][1]=='3'){
	mask3p=0;
      }
      if(argv[0][1]=='B'){
	backp=0;
      }
      if(argv[0][1]=='D'){
	decayp=0;
      }
      if(argv[0][1]=='X'){
	ath=0;
	mask1p=0;
	mask2p=0;
	mask3p=0;
	decayp=0;
	backp=0;
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
	if(argv[0][1]=='1'){
	  mask1p=1;
	}
	if(argv[0][1]=='2'){
	  mask2p=1;
	}
	if(argv[0][1]=='3'){
	  mask3p=1;
	}
	if(argv[0][1]=='B'){
	  backp=1;
	}
	if(argv[0][1]=='D'){
	  decayp=1;
	}
	if(argv[0][1]=='X'){
	  ath=1;
	  mask1p=1;
	  mask2p=1;
	  mask3p=1;
	  decayp=1;
	  backp=1;
	}
      }else
	framesize=atoi(argv[0]);
    argv++;
  }
  
  pcm[0]=malloc(framesize*sizeof(double));
  pcm[1]=malloc(framesize*sizeof(double));
  out[0]=calloc(framesize/2,sizeof(double));
  out[1]=calloc(framesize/2,sizeof(double));
  decay[0]=calloc(framesize/2,sizeof(double));
  decay[1]=calloc(framesize/2,sizeof(double));
  mask=malloc(framesize*sizeof(double));
  floor=malloc(framesize*sizeof(double));
  lpc=malloc(order*sizeof(double));
  buffer=malloc(framesize*4);
  buffer2=buffer+framesize*2;
  window=_vorbis_window(0,framesize,framesize/2,framesize/2);
  maskwindow=_vorbis_window(0,framesize,framesize/2,framesize/2);
  mdct_init(&m_look,framesize);
  _vp_psy_init(&p_look,&_psy_set0,framesize/2,44100);
  lpc_init(&lpc_look,framesize/2,256,44100,order);

  for(j=0;j<framesize;j++)
    maskwindow[j]*=maskwindow[j];

  /* we cheat on the WAV header; we just bypass 44 bytes and never
     verify that it matches 16bit/stereo/44.1kHz. */
  
  fread(buffer,1,44,stdin);
  fwrite(buffer,1,44,stdout);
  memset(buffer,0,framesize*2);

  analysis("window",0,window,framesize,0,0);

  fprintf(stderr,"Processing for frame size %d...\n",framesize);

  while(!eos){
    long s;
    long bytes=fread(buffer2,1,framesize*2,stdin); 
    if(bytes<framesize*2)
      memset(buffer2+bytes,0,framesize*2-bytes);
    
    if(bytes!=0){

      /* uninterleave samples */
      for(i=0;i<framesize;i++){
        pcm[0][i]=((buffer[i*4+1]<<8)|
                      (0x00ff&(int)buffer[i*4]))/32768.;
        pcm[1][i]=((buffer[i*4+3]<<8)|
		   (0x00ff&(int)buffer[i*4+2]))/32768.;
      }
      
      for(i=0;i<2;i++){

	analysis("pre",frameno,pcm[i],framesize,0,0);
	
	/* do the psychacoustics */
	memset(mask,0,sizeof(double)*framesize/2);
	for(j=0;j<framesize;j++)
	  mask[j]=pcm[i][j]*maskwindow[j];

	mdct_forward(&m_look,mask,mask);

	analysis("maskmdct",frameno,mask,framesize/2,1,1);
	_vp_tone_tone_mask(&p_look,mask,mask,
			   1,1,decayp,decay[i]);
	analysis("mask",frameno,mask,framesize/2,1,0);
	analysis("lmask",frameno,mask,framesize/2,0,0);
	analysis("decay",frameno,decay[i],framesize/2,1,1);
	analysis("ldecay",frameno,decay[i],framesize/2,0,1);
	
	for(j=0;j<framesize;j++)
	  pcm[i][j]=pcm[i][j]*window[j];
	analysis("pcm",frameno,pcm[i],framesize,0,0);

	mdct_forward(&m_look,pcm[i],pcm[i]);
	analysis("mdct",frameno,pcm[i],framesize/2,1,1);
	analysis("lmdct",frameno,pcm[i],framesize/2,0,1);

	/* floor */
	{
	  double amp;

	  for(j=0;j<framesize/2;j++)floor[j]=mask[j]+DYNAMIC_RANGE_dB;
	  amp=sqrt(vorbis_curve_to_lpc(floor,lpc,&lpc_look));
	  fprintf(stderr,"amp=%g\n",amp);
	  vorbis_lpc_to_curve(floor,lpc,amp,&lpc_look);
	  for(j=0;j<framesize/2;j++)floor[j]-=DYNAMIC_RANGE_dB;
	  analysis("floor",frameno,floor,framesize/2,1,0);

	}


	/* quantize according to masking */
	for(j=0;j<framesize/2;j++){
	  double val;
	  if(mask[j]==0)
	    val=0;
	  else{
	    val=todB(pcm[i][j])-floor[j]-3.;
	    if(val<-6.){
	      val=0;
	    }else{
	      nonz+=1;
	      if(val<1.5){
		val=1;
	      }else{
		val=rint(val/3.)+1;
	      }
	    }
	    if(pcm[i][j]<0)val= -val;
	  }
  
	  acc+=log(fabs(val)*2.+1.)/log(2);
	  tot++;
	  if(val==0)
	    pcm[i][j]=0.;
	  else{
	    if(val>0)
	      pcm[i][j]=fromdB(val*3.+floor[j]-3.);
	    else
	      pcm[i][j]=-fromdB(floor[j]-val*3.-3.);
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
	char *ptr=buffer+i*2;
	double  *mono=out[i];
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
