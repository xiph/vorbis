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

 function: bark scale utility
 last mod: $Id: barkmel.c,v 1.9.6.1 2002/06/26 00:37:37 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include "scales.h"
int main(){
  int i;
  double rate;
  for(i=64;i<32000;i*=2){
    rate=48000.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=44100.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=32000.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=22050.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=16000.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=11025.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));

    rate=8000.f;
    fprintf(stderr,"rate=%gHz, block=%d, f(1)=%.2gHz bark(1)=%.2g (of %.2g)\n\n",
	    rate,i,rate/2 / (i/2),toBARK(rate/2 /(i/2)),toBARK(rate/2));


  }
  {
    float i;
    int j;
    for(i=0.,j=0;i<28;i+=1,j++){
      fprintf(stderr,"(%d) bark=%f %gHz (%d of 1024)\n",
	      j,i,fromBARK(i),(int)rint(fromBARK(i)/22050.*1024.));
    }
  }
  {
    int foo[]={0,6,12,20,28, 36,44,52,62, 72,82,94,108,  122,138,156,176,  
	       198,224,252,286, 324,370,422,486, 564,658,774,920, 
	       1106,1344,1650,2048};
    for(i=0;i<33;i++){
      float Hz=foo[i]/2048.*22050.;
      fprintf(stderr,"%d: %fHz, %f Bark\n",foo[i],Hz,toBARK(Hz));
    }
  } 


  return(0);
}

