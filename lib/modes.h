/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: predefined encoding modes
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 22 1999

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "codec.h"

double threshhold_points[THRESH_POINTS]=
/* 0Hz                                                             24kHz
 0   1   2   3   4   5   6   7  8   9  10 11  12 13  14 15 16 17 18 19 */ 
{0.,.01,.02,.03,.04,.06,.08,.1,.15,.2,.25,.3,.34,.4,.45,.5,.6,.7,.8,1.};

vorbis_info predef_modes[]={
  /* CD quality stereo, no channel coupling */

    /* channels, sample rate,  dummy, dummy, dummy, dummy */
  { 2, 44100,     0, NULL, 0, NULL, 
    /* smallblock, largeblock, LPC order (small, large) */
    {512, 2048}, {16,16}, 
    /* spectral octaves (small, large), spectral channels */
    {5,5}, 2,
    /* thresh sample period, preecho clamp trigger threshhold, range, dummy */
    128, 4, 2, NULL,
    /* noise masking curve dB attenuation levels [20] */
    {-12,-12,-18,-18,-18,-18,-18,-18,-18,-12,
      -8,-4,0,0,1,2,3,3,4,5},
    /*{-100,-100,-100,-100,-100,-100,-100,-24,-24,-24,
      -24,-24,-24,-24,-24,-24,-24,-24,-24,-24}*/
    /* noise masking scale biases */
    .95,1.01,.01,
    /* tone masking curve dB attenuation levels [20] */
    {-20,-20,-20,-20,-20,-20,-20,-20,-20,-20,
     -20,-20,-20,-20,-20,-20,-20,-20,-20,-20},
    /* tone masking rolloff settings (dB per octave), octave bias */
    90,60,.001,
    NULL,NULL,NULL},
  
};

#define predef_mode_max 0

#endif
