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

 function: linear scale -> dB, Bark and Mel scales
 last mod: $Id: scales.h,v 1.1.2.1 2000/03/29 03:49:29 xiphmont Exp $

 ********************************************************************/

#ifndef _V_SCALE_H_
#define _V_SCALES_H_

#include <math.h>

#define min(x,y)  ((x)>(y)?(y):(x))
#define max(x,y)  ((x)<(y)?(y):(x))

/* 20log10(x) */
#define DYNAMIC_RANGE_dB 200.
#define todB(x)   ((x)==0?-9.e40:log(fabs(x))*8.6858896)
#define fromdB(x) (exp((x)*.11512925))


/* The bark scale equations are approximations, since the original
   table was somewhat hand rolled.  The below are chosen to have the
   best possible fit to the rolled tables, thus their somewhat odd
   appearance (these are more accurate and over a longer range than
   the oft-quoted bark equations found in the texts I have).  The
   approximations are valid from 0 - 30kHz (nyquist) or so.

   all f in Hz, z in Bark */

#define toBARK(f)   (13.1*atan(.00074*(f))+2.24*atan((f)*(f)*1.85e-8)+1e-4*(f))
#define fromBARK(z) (102.*(z)-2.*pow(z,2.)+.4*pow(z,3)+pow(1.46,z)-1.)
#define toMEL(f)    (log(1.+(f)*.001)*1442.695)
#define fromMEL(m)  (1000.*exp((m)/1442.695)-1000.)

/* Frequency to octave.  We arbitrarily declare 250.0 Hz to be octave
   0.0 */

#define toOC(f)     (log(f)*1.442695-7.965784)
#define fromOC(o)   (exp(((o)+7.965784)*.693147))

#endif

