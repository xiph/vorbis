#ifndef _OS_TYPES_H
#define _OS_TYPES_H
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
 last mod: $Id: os_types.h,v 1.6 2000/07/07 06:16:24 xiphmont Exp $

 ********************************************************************/

#if defined (_WIN32) 
#if !defined(__GNUC__)

typedef __int64 int64_t;
typedef __int16 int16_t;
#define vorbis_size32_t int
#else
#include <_G_config.h>
pedef _G_int64_t int64_t;
typedef _G_int32_t int32_t;
typedef _G_int16_t int16_t;
#endif
#endif

#ifdef __BEOS__
#include <inttypes.h>
#endif

#endif 

