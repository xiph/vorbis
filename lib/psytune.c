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
 last mod: $Id: psytune.c,v 1.1 2000/02/25 11:05:32 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "vorbis/codec.h"
#include "psy.h"
#include "mdct.h"
#include "window.h"

static vorbis_info_psy _psy_set0={
  {-20, -20, -14, -14, -14, -14, -14, -14, -14, -14,
   -14, -14, -16, -16, -16, -16, -18, -18, -16, -16,
   -12, -10, -6, -3, -1, -1, -0}, 0., (.6/1024), 10,4
};

int main(int argc,char *argv[]){
  int eos=0;
  double acc=0.;
  double tot=0.;
  int framesize=argv[1]?atoi(argv[1]):2048;
  double *pcm[2],*out[2],*window,*mask,*decay[2];
  signed char *buffer,*buffer2;
  mdct_lookup m_look;
  vorbis_look_psy p_look;

  pcm[0]=malloc(framesize*sizeof(double));
  pcm[1]=malloc(framesize*sizeof(double));
  out[0]=calloc(framesize/2,sizeof(double));
  out[1]=calloc(framesize/2,sizeof(double));
  decay[0]=calloc(framesize/2,sizeof(double));
  decay[1]=calloc(framesize/2,sizeof(double));
  mask=malloc(framesize/2*sizeof(double));
  buffer=malloc(framesize*4);
  buffer2=buffer+framesize*2;
  window=_vorbis_window(0,framesize,framesize/2,framesize/2);
  mdct_init(&m_look,framesize);
  _vp_psy_init(&p_look,&_psy_set0,framesize/2,44100);

  /* we cheat on the WAV header; we just bypass 44 bytes and never
     verify that it matches 16bit/stereo/44.1kHz. */
  
  fread(buffer,1,44,stdin);
  fwrite(buffer,1,44,stdout);
  memset(buffer,0,framesize*2);

  fprintf(stderr,"Processing for frame size %d...\n",framesize);

  while(!eos){
    long i,j,k;
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
	for(j=0;j<framesize;j++)
	  pcm[i][j]*=window[j];

	mdct_forward(&m_look,pcm[i],pcm[i]);

	/* do the psychacoustics */

	memset(mask,0,sizeof(double)*framesize/2);
	_vp_mask_floor(&p_look,pcm[i],mask,decay[i],1);

	/* quantize according to masking */
	for(j=0;j<framesize/2;j++){
	  double val;
	  if(mask[j]==0)
	    val=0;
	  else
	    val=rint(pcm[i][j]/mask[j]);
	  acc+=log(fabs(val)*2.+1.)/log(2);
	  tot++;
	  pcm[i][j]=val*mask[j];
	}

	/* take it back to time */
	mdct_backward(&m_look,pcm[i],pcm[i]);
	for(j=0;j<framesize/2;j++)
	  out[i][j]+=pcm[i][j]*window[j];
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
  fprintf(stderr,"Done\n\n");
  return 0;
}
