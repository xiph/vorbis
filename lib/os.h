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

 function: #ifdef jail to whip a few platforms into the UNIX ideal.
 last mod: $Id: os.h,v 1.2 1999/12/30 07:26:46 xiphmont Exp $

 ********************************************************************/

#ifndef _V_IFDEFJAIL_H_
#define _V_IFDEFJAIL_H_

#ifndef M_PI
#define M_PI (3.1415926539)
#endif

#ifndef rint
/* not strictly correct, but Vorbis doesn't care */
#define rint(x)   (floor((x)+0.5)) 
#endif

#ifndef alloca
#ifdef _WIN32
#define alloca(x) (_alloca(x))
#endif
#endif

#endif


