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

 function: window functions
 last mod: $Id: window.c,v 1.3 1999/12/30 07:26:55 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include "os.h"

/* The 'vorbis window' is sin(sin(x)*sin(x)*2pi) */

double *_vorbis_window(int window,int left,int right){
  double *ret=calloc(window,sizeof(double));
  int leftbegin=window/4-left/2;
  int rightbegin=window-window/4-right/2;
  int i;
  
  for(i=0;i<left;i++){
    double x=(i+.5)/left*M_PI/2.;
    x=sin(x);
    x*=x;
    x*=M_PI/2.;
    x=sin(x);
    ret[i+leftbegin]=x;
  }

  for(i=leftbegin+left;i<rightbegin;i++)
    ret[i]=1.;

  for(i=0;i<right;i++){
    double x=(right-i-.5)/right*M_PI/2.;
    x=sin(x);
    x*=x;
    x*=M_PI/2.;
    x=sin(x);
    ret[i+rightbegin]=x;
  }

  return(ret);
}

