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

 function: window functions
 last mod: $Id: window.c,v 1.10 2000/11/14 00:05:32 xiphmont Exp $

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
	float x=(i+.5)/left*M_PI/2.;
	x=sin(x);
	x*=x;
	x*=M_PI/2.;
	x=sin(x);
	ret[i+leftbegin]=x;
      }
      
      for(i=leftbegin+left;i<rightbegin;i++)
	ret[i]=1.;
      
      for(i=0;i<right;i++){
	float x=(right-i-.5)/right*M_PI/2.;
	x=sin(x);
	x*=x;
	x*=M_PI/2.;
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

