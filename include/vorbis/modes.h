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
 last mod: $Id: modes.h,v 1.15.2.1 2000/06/23 08:36:35 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp12_0.vqh"
#include "vorbis/book/lsp30_0.vqh"
#include "vorbis/book/res0_128_1024aux.vqh"
#include "vorbis/book/res0_128_128aux.vqh"

#include "vorbis/book/res0_100_1024a_0.vqh"
#include "vorbis/book/res0_100_1024a_1.vqh"
#include "vorbis/book/res0_100_1024a_2.vqh"
#include "vorbis/book/res0_100_1024a_3.vqh"
#include "vorbis/book/res0_100_1024a_4.vqh"

#include "vorbis/book/res0_100_1024b_1.vqh"
#include "vorbis/book/res0_100_1024b_2.vqh"
#include "vorbis/book/res0_100_1024b_3.vqh"
#include "vorbis/book/res0_100_1024b_4.vqh"
#include "vorbis/book/res0_100_1024b_5.vqh"
#include "vorbis/book/res0_100_1024b_6.vqh"
#include "vorbis/book/res0_100_1024b_7.vqh"

#include "vorbis/book/res0_100_1024c_1.vqh"
#include "vorbis/book/res0_100_1024c_2.vqh"
#include "vorbis/book/res0_100_1024c_3.vqh"
#include "vorbis/book/res0_100_1024c_4.vqh"
#include "vorbis/book/res0_100_1024c_5.vqh"
#include "vorbis/book/res0_100_1024c_6.vqh"

/* A farily high quality setting mix */
static vorbis_info_psy _psy_set0={
  1,/*athp*/
  1,/*decayp*/
  1,/*smoothp*/
  0,8,0.,

  -130.,

  1,/* tonemaskp */
  {-80.,-80.,-80.,-80.,-100.}, /* remember that el 0,2 is a 80 dB curve */
  {-40.,-40.,-60.,-80.,-80.}, /* remember that el 4 is an 80 dB curve, not 100 */
  {-40.,-40.,-60.,-80.,-100.},
  {-40.,-40.,-60.,-80.,-100.},
  {-40.,-40.,-60.,-80.,-100.},
  {-40.,-40.,-60.,-80.,-100.},
  {-40.,-40.,-60.,-80.,-100.},  

  1,/* peakattp */
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-12.,-12.,-12.,-16.,-18.},
  {-10.,-10.,-12.,-16.,-18.},
  {-6.,-8.,-10.,-12.,-12.},

  1,/*noisemaskp */
  {-100.,-100.,-100.,-200.,-200.},
  {-100.,-100.,-100.,-200.,-200.},
  {-100.,-100.,-100.,-200.,-200.},
  {-65.,-65.,-65.,-85.,-85.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},

  100.,

  .9998, .9998  /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0={0};
static vorbis_info_floor0 _floor_set0={12, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1={30, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0={0,128, 32,12,2,
					   {1,1,1,1,1,
					    0,1,1,1,1,1,1},
					   {4,5,6,7,8,
					    16,17,18,19,20,21},
					   {9999,9999,99999,99999,99999,
					    0,8,9999,15,9999,9999},
					   {2.5,5,9,18,9999,
					    0,1.5,1.5,2.5,2.5,5},
					   {5,5,5,5,5,
					    5,5,5,5,5,5},
					   {1,1,1,1,1,
					    99,99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1={0,1024, 64,20,3,
					   {1,1,1,1,1,
					    0,1,1,1,1,1,1,1,
					    0,1,1,1,1,1,1},
					   {4,5,6,7,8,
					    9,10,11,12,13,14,15,
					    16,17,18,19,20,21},
					   {9999,9999,99999,99999,99999,
					    0,8,9999,15,9999,9999,9999,9999,
					    0,8,9999,15,9999,9999},
					   {2.5,5,9,18,9999,
					    0,1.5,1.5,2.5,2.5,3.5,5,9999,
					    0,1.5,1.5,2.5,2.5,5},
					   {5,5,5,5,5,
					    5,5,5,5,5,5,5,5,
					    5,5,5,5,5,5},
					   {4,4,4,4,4,
					    12,12,12,12,12,12,12,12,
					    99,99,99,99,99,99}};

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
  2,          2,    1,     2,       2,   22,   1,
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
  {&_vq_book_lsp12_0,      /* 0 */
   &_vq_book_lsp30_0,      /* 1 */

   &_huff_book_res0_128_128aux,
   &_huff_book_res0_128_1024aux, /* 3 */

   &_vq_book_res0_100_1024a_0,
   &_vq_book_res0_100_1024a_1,
   &_vq_book_res0_100_1024a_2,
   &_vq_book_res0_100_1024a_3,
   &_vq_book_res0_100_1024a_4, /* 8 */

   &_vq_book_res0_100_1024b_1,
   &_vq_book_res0_100_1024b_2,
   &_vq_book_res0_100_1024b_3,
   &_vq_book_res0_100_1024b_4,
   &_vq_book_res0_100_1024b_5,
   &_vq_book_res0_100_1024b_6,
   &_vq_book_res0_100_1024b_7, /* 15 */

   &_vq_book_res0_100_1024c_1,
   &_vq_book_res0_100_1024c_2,
   &_vq_book_res0_100_1024c_3,
   &_vq_book_res0_100_1024c_4,
   &_vq_book_res0_100_1024c_5,
   &_vq_book_res0_100_1024c_6, /* 21 */
  },
  /* psy */
  {&_psy_set0},
  /* thresh sample period, preecho clamp trigger threshhold, range */
  64, 15, 2 
};

#define PREDEF_INFO_MAX 0

#endif
