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
 last mod: $Id: modes.h,v 1.9.2.2 2000/04/01 12:51:31 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp20_0.vqh"
#include "vorbis/book/lsp32_0.vqh"

#include "vorbis/book/resshort8aux.vqh"
#include "vorbis/book/reslong8aux.vqh"

#include "vorbis/book/resX_0.vqh"
#include "vorbis/book/resX_1.vqh"

#include "vorbis/book/res128_0a.vqh"
#include "vorbis/book/res1024_0a.vqh"
/*#include "vorbis/book/res128_0b.vqh"
#include "vorbis/book/res128_0c7x4.vqh"
#include "vorbis/book/res1024_1a.vqh"
#include "vorbis/book/res1024_0b.vqh"
#include "vorbis/book/res1024_0c8x4.vqh"*/

/* A farily high quality setting mix */
static vorbis_info_psy _psy_set0={
  3,1,1,

  1,16,4.,

  -130.,

  {-35.,-40.,-60.,-80.,-85.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},
  {-35.,-40.,-60.,-80.,-100.},

  /*{-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},
  {-99,-99,-99,-99,-99},*/
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  {-8.,-12.,-18.,-20.,-24.},
  -20.,-10.,

  110.,

  .9998, .9997  /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0={0};
static vorbis_info_floor0 _floor_set0={20, 44100,  64, 12,140, 1, {0} };
static vorbis_info_floor0 _floor_set1={32, 44100, 256, 12,140, 1, {1} };
static vorbis_info_residue0 _residue_set0={0, 128, 8,4,2,{0,1,1,1},{6,6,6},3.};
static vorbis_info_residue0 _residue_set1={0,1024, 8,4,3,{0,1,1,1},{6,6,6},3.};
static vorbis_info_mapping0 _mapping_set0={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1={1, {0,0}, {0}, {1}, {1}, {0}};
static vorbis_info_mode _mode_set0={0,0,0,0};
static vorbis_info_mode _mode_set1={1,0,0,1};

/* CD quality stereo, no channel coupling */
vorbis_info info_A={
  /* channels, sample rate, upperkbps, nominalkbps, lowerkbps */
  0, 2, 44100, 0,0,0,
  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   7,   1,
  /* modes */
  {&_mode_set0,&_mode_set1},
  /* maps */
  {0,0},{&_mapping_set0,&_mapping_set1},
  /* times */
  {0,0},{&_time_set0},
  /* floors */
  {0,0},{&_floor_set0,&_floor_set1},
  /* residue */
  {0,0},{&_residue_set0,&_residue_set1},
  /* books */
  {&_vq_book_lsp20_0,      /* 0 */
   &_vq_book_lsp32_0,      /* 1 */

   &_huff_book_resshort8aux,/* 2 */
   &_huff_book_reslong8aux, /* 3 */

   &_vq_book_res128_0a,    /* 4 */
   &_vq_book_res1024_0a,   /* 5 */

#if 0
   &_vq_book_res128_0a,    /* 4 */
   &_vq_book_res128_0b,    /* 5 */
   &_vq_book_res128_0c7x4, /* 6 */
   &_vq_book_res1024_1a,   /* 7 */
   &_vq_book_res1024_0b,   /* 8 */
   &_vq_book_res1024_0c8x4,/* 9 */
#endif

   &_vq_book_resX_0,

  },
  /* psy */
  {&_psy_set0},
  /* thresh sample period, preecho clamp trigger threshhold, range */
  64, 10, 2 
};

#define PREDEF_INFO_MAX 0

#endif
