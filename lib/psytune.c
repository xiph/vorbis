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
 last mod: $Id: psytune.c,v 1.1.2.2.2.9 2000/05/08 08:25:43 xiphmont Exp $

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

static vorbis_info_psy _psy_set0={
  1,/*athp*/
  1,/*decayp*/
  1,/*smoothp*/
  1,8,0.,

  -130.,

  1,/* tonemaskp*/
  {-35.,-40.,-60.,-80.,-80.}, /* remember that el 4 is an 80 dB curve, not 100 */
  {-35.,-40.,-60.,-80.,-95.},
  {-35.,-40.,-60.,-80.,-95.},
  {-35.,-40.,-60.,-80.,-95.},
  {-35.,-40.,-60.,-80.,-95.},
  {-65.,-60.,-60.,-80.,-90.},  /* remember that el 1 is a 60 dB curve, not 40 */

  1,/*noisemaskp*/
  {-100.,-100.,-100.,-200.,-200.}, /* this is the 500 Hz curve, which
                                      is too wrong to work */
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-50.,-55.,-60.,-80.,-80.},

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

typedef struct {
  long n;
  int ln;
  int  m;
  int *linearmap;

  vorbis_info_floor0 *vi;
  lpc_lookup lpclook;
} vorbis_look_floor0;

extern double _curve_to_lpc(double *curve,double *lpc,vorbis_look_floor0 *l,
			    long frameno);
extern void _lpc_to_curve(double *curve,double *lpc,double amp,
			  vorbis_look_floor0 *l,char *name,long frameno);

long frameno=0;

/* hacked from floor0.c */
static void floorinit(vorbis_look_floor0 *look,int n,int m,int ln){
  int j;
  double scale;
  look->m=m;
  look->n=n;
  look->ln=ln;
  lpc_init(&look->lpclook,look->ln,look->m);

  scale=look->ln/toBARK(22050.);

  look->linearmap=malloc(look->n*sizeof(int));
  for(j=0;j<look->n;j++){
    int val=floor( toBARK(22050./n*j) *scale);
    if(val>look->ln)val=look->ln;
    look->linearmap[j]=val;
  }
}

int main(int argc,char *argv[]){
  int eos=0;
  double nonz=0.;
  double acc=0.;
  double tot=0.;

  int framesize=2048;
  int order=32;

  double *pcm[2],*out[2],*window,*decay[2],*lpc,*floor,*mask;
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
  
  pcm[0]=malloc(framesize*sizeof(double));
  pcm[1]=malloc(framesize*sizeof(double));
  out[0]=calloc(framesize/2,sizeof(double));
  out[1]=calloc(framesize/2,sizeof(double));
  decay[0]=calloc(framesize/2,sizeof(double));
  decay[1]=calloc(framesize/2,sizeof(double));
  floor=malloc(framesize*sizeof(double));
  mask=malloc(framesize*sizeof(double));
  lpc=malloc(order*sizeof(double));
  buffer=malloc(framesize*4);
  buffer2=buffer+framesize*2;
  window=_vorbis_window(0,framesize,framesize/2,framesize/2);
  mdct_init(&m_look,framesize);
  _vp_psy_init(&p_look,&_psy_set0,framesize/2,44100);
  floorinit(&floorlook,framesize/2,order,framesize/8);

  for(i=0;i<11;i++)
    for(j=0;j<9;j++)
      analysis("Ptonecurve",i*10+j,p_look.tonecurves[i][j],EHMER_MAX,0,1);
  for(i=0;i<11;i++)
    for(j=0;j<9;j++)
      analysis("Pnoisecurve",i*10+j,p_look.noisecurves[i][j],EHMER_MAX,0,1);

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
	double amp;

	analysis("pre",frameno,pcm[i],framesize,0,0);
	
	/* do the psychacoustics */
	for(j=0;j<framesize;j++)
	  pcm[i][j]*=window[j];

	mdct_forward(&m_look,pcm[i],pcm[i]);

	analysis("mdct",frameno,pcm[i],framesize/2,1,1);

	_vp_compute_mask(&p_look,pcm[i],floor,mask,decay[i]);
	
	analysis("prefloor",frameno,floor,framesize/2,1,1);
	analysis("mask",frameno,mask,framesize/2,1,1);
	analysis("decay",frameno,decay[i],framesize/2,1,1);
	
	amp=_curve_to_lpc(floor,lpc,&floorlook,frameno);
	_lpc_to_curve(floor,lpc,sqrt(amp),&floorlook,"Ffloor",frameno);
	analysis("floor",frameno,floor,framesize/2,1,1);

	_vp_apply_floor(&p_look,pcm[i],floor,mask);
	analysis("quant",frameno,pcm[i],framesize/2,1,1);

	/* re-add floor */
	for(j=0;j<framesize/2;j++){
	  double val=rint(pcm[i][j]);
	  tot++;
	  if(val){
	    nonz++;
	    acc+=log(fabs(val)*2.+1.)/log(2);
	    pcm[i][j]=val*floor[j];
	  }else{
	    pcm[i][j]=0;
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
