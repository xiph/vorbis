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
 last mod: $Id: psytune.c,v 1.1.2.2 2000/03/29 20:08:49 xiphmont Exp $

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

static vorbis_info_psy _psy_set0={
  -130.,

  {-35.,-40.,-60.,-80.,-85.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},

  /*{-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},*/
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  -25.,-12.,

  110.,

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

  lpc_lookup lpc_look;

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

  for(i=0;i<11;i++)
    for(j=0;j<9;j++)
      analysis("Pcurve",i*10+j,p_look.curves[i][j],EHMER_MAX,0,1);

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

	{
	  double *iter=malloc(sizeof(double)*framesize/2);

	  _vp_tone_tone_mask(&p_look,mask,floor,
			     ath,0,decay[i]);
	
	  memcpy(iter,mask,sizeof(double)*framesize/2);
	  for(j=0;j<framesize/2;j++)
	    if(fabs(iter[j])<fromdB(floor[j]))iter[j]=0.;
	  _vp_tone_tone_mask(&p_look,iter,iter,
			     ath,0,decay[i]);
	  for(j=0;j<framesize/2;j++)
	    floor[j]=(floor[j]+iter[j])/2.;

	  memcpy(iter,mask,sizeof(double)*framesize/2);
	  for(j=0;j<framesize/2;j++)
	    if(fabs(iter[j])<fromdB(floor[j]))iter[j]=0.;
	  _vp_tone_tone_mask(&p_look,iter,iter,
			     ath,0,decay[i]);
	  for(j=0;j<framesize/2;j++)
	    floor[j]=(floor[j]+iter[j])/2.;
	  
	  memcpy(iter,mask,sizeof(double)*framesize/2);
	  for(j=0;j<framesize/2;j++)
	    if(fabs(iter[j])<fromdB(floor[j]))iter[j]=0.;
	  _vp_tone_tone_mask(&p_look,iter,mask,
			     ath,0,decay[i]);
	}	  


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
	    val=rint((todB(pcm[i][j])-floor[j]));
	    if(val<0.){
	      val=0.;
	    }else{
	      nonz+=1;
	      val+=1;
	      if(pcm[i][j]<0)val= -val;
	    }
	  }
	  
	  acc+=log(fabs(val)*2.+1.)/log(2);
	  tot++;
	  if(val==0)
	    pcm[i][j]=0.;
	  else{
	    if(val>0)
	      pcm[i][j]=fromdB((val-1.)+floor[j]);
	    else
	      pcm[i][j]=-fromdB(floor[j]-(val+1.));
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
