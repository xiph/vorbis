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

 function: window functions
 last mod: $Id: window.c,v 1.13 2001/02/26 03:50:43 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "os.h"
#include "misc.h"

float *_vorbis_window(int type, int window,int left,int right){
  float *ret=_ogg_calloc(window,sizeof(float));

  switch(type){
  case 0:
    /* The 'vorbis window' (window 0) is sin(sin(x)*sin(x)*2pi) */
    {
      int leftbegin=window/4-left/2;
      int rightbegin=window-window/4-right/2;
      int i;
    
      for(i=0;i<left;i++){
	float x=(i+.5f)/left*M_PI/2.;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	ret[i+leftbegin]=x;
      }
      
      for(i=leftbegin+left;i<rightbegin;i++)
	ret[i]=1.f;
      
      for(i=0;i<right;i++){
	float x=(right-i-.5f)/right*M_PI/2.;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	ret[i+rightbegin]=x;
      }
    }
    break;
  default:
    _ogg_free(ret);
    return(NULL);
  }
  return(ret);
}

