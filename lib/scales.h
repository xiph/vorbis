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

 function: linear scale -> dB, Bark and Mel scales
 last mod: $Id: scales.h,v 1.9 2000/12/21 21:04:41 xiphmont Exp $

 ********************************************************************/

#ifndef _V_SCALE_H_
#define _V_SCALES_H_

#include <math.h>

/* 20log10(x) */
#define DYNAMIC_RANGE_dB 200.f
#define todB(x)   ((x)==0?-9e20f:log(fabs(x))*8.6858896f)
#define todB_nn(x)   ((x)==0.f?-400.f:log(x)*8.6858896f)
#define fromdB(x) (exp((x)*.11512925f))


/* The bark scale equations are approximations, since the original
   table was somewhat hand rolled.  The below are chosen to have the
   best possible fit to the rolled tables, thus their somewhat odd
   appearance (these are more accurate and over a longer range than
   the oft-quoted bark equations found in the texts I have).  The
   approximations are valid from 0 - 30kHz (nyquist) or so.

   all f in Hz, z in Bark */

#define toBARK(f)   (13.1f*atan(.00074f*(f))+2.24f*atan((f)*(f)*1.85e-8f)+1e-4f*(f))
#define fromBARK(z) (102.f*(z)-2.f*pow(z,2.f)+.4f*pow(z,3.f)+pow(1.46f,z)-1.f)
#define toMEL(f)    (log(1.f+(f)*.001f)*1442.695f)
#define fromMEL(m)  (1000.f*exp((m)/1442.695f)-1000.f)

/* Frequency to octave.  We arbitrarily declare 125.0 Hz to be octave
   0.0 */

#define toOC(f)     (log(f)*1.442695f-6.965784f)
#define fromOC(o)   (exp(((o)+6.965784f)*.693147f))

#endif

