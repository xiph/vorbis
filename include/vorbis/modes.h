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

 function: predefined encoding modes
 last mod: $Id: modes.h,v 1.1 2000/01/05 03:10:47 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "vorbis/codec.h"

/*
   0      1      2      3      4     5      6     7     8     9 
   0,   100,  200,   300,   400,   510,   630,  770,  920, 1080,

   10    11    12     13     14     15     16    17    18    19
 1270, 1480, 1720,  2000,  2320,  2700,  3150, 3700, 4400, 5300,

   20    21    22     23     24     25     26 Bark
 6400, 7700, 9500, 12000, 15500, 20500, 27000 Hz    */

vorbis_info predef_modes[]={
  /* CD quality stereo, no channel coupling */

    /* channels, sample rate, upperkbps, nominalkbps, lowerkbps */
  { 2, 44100, 0,0,0,
    /* dummy, dummy, dummy, dummy */
    0, NULL, 0, NULL, 
    /* smallblock, largeblock, LPC order (small, large) */
    {256, 2048}, {20,32}, 
    /* {bark mapping size}, spectral channels */
    {64,256}, 2,
    /* thresh sample period, preecho clamp trigger threshhold, range, dummy */
    64, 10, 2, 
    /* tone masking curve dB attenuation levels [27] */
    { -10, -10, -10, -10, -10, -10, -10, -10, -10, -10,
      -12, -14, -16, -16, -16, -16, -18, -18, -16, -16,
      -12, -10, -8, -6, -6, -6, -4},
    /* tone masking rolloff settings (dB per octave), octave bias */
    24,10,
    NULL,NULL,NULL},
  
};

#define predef_mode_max 0

#endif
