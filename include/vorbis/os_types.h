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
 last mod: $Id: os_types.h,v 1.3 2000/06/14 08:19:22 xiphmont Exp $

 ********************************************************************/

#if defined (_WIN32) && !defined(__GNUC__)
typedef __int64 int64_t;
typedef unsigned int u_int32_t;
#endif

#ifdef __BEOS__
#include <inttypes.h>
#endif

#endif 

