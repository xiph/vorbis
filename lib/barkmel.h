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

 function: linear scale -> bark and mel scales
 last mod: $Id: barkmel.h,v 1.2 2000/01/01 02:52:58 xiphmont Exp $

 ********************************************************************/

#ifndef _V_BARKMEL_H_
#define _V_BARKMEL_H_

#include <math.h>

/* The bark scale equations are approximations, since the original
   table was somewhat hand rolled.  They're chosen to have the best
   possible fit to the rolled tables, thus their somewhat odd
   appearence (these are more accurate and over a longer range than
   the oft-quoted bark equations found in the texts I have).  The
   approximations are valid from 0 - 30kHz (nyquist) or so.

   all f in Hz, z in Bark */

#define fBARK(f) (13.1*atan(.00074*(f))+2.24*atan((f)*(f)*1.85e-8)+1e-4*(f))
#define iBARK(z) (102.*(z)-2.*pow(z,2.)+.4*pow(z,3)+pow(1.46,z)-1.)
#define fMEL(f)  (log(1.+(f)*.001)*1442.695)
#define iMEL(m)  (1000.*exp((m)/1442.695)-1000.)

#endif
