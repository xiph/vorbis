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

 function: dup/clear the psy setup
 last mod: $Id: infopsy.c,v 1.1 2000/01/19 08:57:56 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"

static vorbis_info_psy *vorbis_psy_dup(vorbis_info_psy *source){
  if(source){
    vorbis_info_psy *d=malloc(sizeof(vorbis_info_psy));
    memcpy(d,source,sizeof(vorbis_info_psy));
    return(d);
  }else
    return(NULL);
}

static void vorbis_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    free(i);
  }
}

