#ifndef _OS_H
#define _OS_H
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
 last mod: $Id: os.h,v 1.9 2000/08/23 06:38:49 xiphmont Exp $

 ********************************************************************/

#include <math.h>
#ifndef _V_IFDEFJAIL_H_
#define _V_IFDEFJAIL_H_

#ifndef M_PI
#define M_PI (3.1415926539)
#endif

#ifndef __GNUC__
#ifdef _WIN32
#  include <malloc.h>
#  define rint(x)   (floor((x)+0.5)) 
#endif
#endif

#ifdef _WIN32
#  define FAST_HYPOT(a, b) sqrt((a)*(a) + (b)*(b))
#else /* if not _WIN32 */
#  define FAST_HYPOT hypot
#endif

#endif

#ifdef USE_ALLOCA_H
#include <alloca.h>
#endif

#ifdef USE_MEMORY_H
#include <memory.h>
#endif

#ifndef min
#  define min(x,y)  ((x)>(y)?(y):(x))
#endif

#ifndef max
#  define max(x,y)  ((x)<(y)?(y):(x))
#endif

#include "../include/vorbis/os_types.h"

#endif /* _OS_H */



