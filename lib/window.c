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

 function: window functions
 last mod: $Id: window.c,v 1.20 2003/03/04 21:22:11 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "os.h"
#include "misc.h"

float *_vorbis_window_create(int type, int left){
  float *ret=_ogg_calloc(left,sizeof(*ret));
  int i;

  switch(type){
  case 0:
    /* The 'vorbis window' (window 0) is sin(sin(x)*sin(x)*pi*.5) */
    {
    
      for(i=0;i<left;i++){
	float x=(i+.5f)/left*M_PI/2.;
	x=sin(x);
	x*=x;
	x*=M_PI/2.f;
	x=sin(x);
	ret[i]=x;
      }
    }
    break;
  default:
    _ogg_free(ret);
    return(NULL);
  }
  return(ret);
}

void _vorbis_apply_window(float *d,float *window[2],long *blocksizes,
			  int lW,int W,int nW){
  lW=(W?lW:0);
  nW=(W?nW:0);

  {
    long n=blocksizes[W];
    long ln=blocksizes[lW];
    long rn=blocksizes[nW];
    
    long leftbegin=n/4-ln/4;
    long leftend=leftbegin+ln/2;
    
    long rightbegin=n/2+n/4-rn/4;
    long rightend=rightbegin+rn/2;
    
    int i,p;
    
    for(i=0;i<leftbegin;i++)
      d[i]=0.f;
    
    for(p=0;i<leftend;i++,p++)
      d[i]*=window[lW][p];
    
    for(i=rightbegin,p=rn/2-1;i<rightend;i++,p--)
      d[i]*=window[nW][p];
    
    for(;i<n;i++)
      d[i]=0.f;
  }
}

