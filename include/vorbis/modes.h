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
 last mod: $Id: modes.h,v 1.15 2000/06/19 10:05:57 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_H_
#define _V_MODES_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp12_0.vqh"
#include "vorbis/book/lsp30_0.vqh"
#include "vorbis/book/resaux0_short.vqh"
#include "vorbis/book/resaux0_long.vqh"
#include "vorbis/book/resaux0b_long.vqh"

#include "vorbis/book/res0b_1.vqh"
#include "vorbis/book/res0b_2.vqh"
#include "vorbis/book/res0b_3.vqh"
#include "vorbis/book/res0b_4.vqh"
#include "vorbis/book/res0b_5.vqh"
#include "vorbis/book/res0b_6.vqh"
#include "vorbis/book/res0b_7.vqh"
#include "vorbis/book/res0b_8.vqh"
#include "vorbis/book/res0b_9.vqh"
#include "vorbis/book/res0b_10.vqh"
#include "vorbis/book/res0b_11.vqh"
#include "vorbis/book/res0b_12.vqh"
#include "vorbis/book/res0b_13.vqh"

#include "vorbis/book/res0a_1.vqh"
#include "vorbis/book/res0a_2.vqh"
#include "vorbis/book/res0a_3.vqh"
#include "vorbis/book/res0a_4.vqh"
#include "vorbis/book/res0a_5.vqh"
#include "vorbis/book/res0a_6.vqh"
#include "vorbis/book/res0a_7.vqh"
#include "vorbis/book/res0a_8.vqh"
#include "vorbis/book/res0a_9.vqh"
#include "vorbis/book/res0a_10.vqh"
#include "vorbis/book/res0a_11.vqh"
#include "vorbis/book/res0a_12.vqh"
#include "vorbis/book/res0a_13.vqh"

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
  {-12.,-12.,-12.,-16.,-16.},
  {-12.,-12.,-12.,-16.,-16.},
  {-12.,-12.,-12.,-16.,-16.},
  {-12.,-12.,-12.,-16.,-16.},
  {-12.,-12.,-12.,-16.,-16.},
  {-10.,-10.,-12.,-16.,-16.},
  {-6.,-8.,-10.,-12.,-12.},

  1,/*noisemaskp */
  {-100.,-100.,-100.,-200.,-200.},
  {-100.,-100.,-100.,-200.,-200.},
  {-100.,-100.,-100.,-200.,-200.},
  {-65.,-65.,-65.,-85.,-85.},
  {-60.,-60.,-60.,-80.,-80.},
  {-60.,-60.,-60.,-80.,-80.},
  {-52.,-55.,-60.,-80.,-80.},

  100.,

  .9998, .9999  /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0={0};
static vorbis_info_floor0 _floor_set0={12, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1={30, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0={0,128, 64,14,2,
					   {0,1,1,1,1,1,1,1,1,1,1,1,1,1,
					    0,1,1,1,1,1,1,1,1,1,1,1,1,1},
					   {4,5,6,7,8,9,10,11,12,13,14,15,16},

					   {0,16,9999,30,9999,41,9999,47,9999,60,9999,128,9999},
					   {1.5,1.5,1.5,2.5,2.5,3.5,3.5,5,5,9,9,18,18},
					   {6,6,6,6,6,6,6,6,6,6,6,6,6},
					   {99,99,99,99,99,99,99,
					    99,99,99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1={0,768, 64,28,3,
					   {0,1,1,1,1,1,1,1,1,1,1,1,1,1,
					    0,1,1,1,1,1,1,1,1,1,1,1,1,1},
					   {4,5,6,7,8,9,10,11,12,13,14,15,16,
					    17,18,19,20,21,22,23,24,25,26,27,28,29},
					   
					   {0, 16,9999, 30,9999, 41,9999,
					    47,9999,60,9999,128,9999,9999,
					    0, 16,9999, 30,9999, 41,9999,
					    47,9999,60,9999,128,9999},

					   {1.5, 1.5,1.5, 2.5,2.5, 3.5,3.5,
					    5,5, 9,9, 18,18, 9999,
					   1.5, 1.5,1.5, 2.5,2.5, 3.5,3.5,
					    5,5, 9,9, 18,18},

					   {6,6,6,6,6,6,6,6,6,6,6,6,6,6,
					   6,6,6,6,6,6,6,6,6,6,6,6,6},/*6==64*/

					   {2,2,2,2,2,2,2,
					    2,2,2,2,2,2,2,
					    99,99,99,99,99,99,99,
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
  2,          2,    1,     2,       2,   30,   1,
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

   &_huff_book_resaux0_short,
   &_huff_book_resaux0b_long,

   &_vq_book_res0a_1,
   &_vq_book_res0a_2,
   &_vq_book_res0a_3,
   &_vq_book_res0a_4,
   &_vq_book_res0a_5,
   &_vq_book_res0a_6,
   &_vq_book_res0a_7,
   &_vq_book_res0a_8,
   &_vq_book_res0a_9,
   &_vq_book_res0a_10,
   &_vq_book_res0a_11,
   &_vq_book_res0a_12,
   &_vq_book_res0a_13,
   &_vq_book_res0b_1,
   &_vq_book_res0b_2,
   &_vq_book_res0b_3,
   &_vq_book_res0b_4,
   &_vq_book_res0b_5,
   &_vq_book_res0b_6,
   &_vq_book_res0b_7,
   &_vq_book_res0b_8,
   &_vq_book_res0b_9,
   &_vq_book_res0b_10,
   &_vq_book_res0b_11,
   &_vq_book_res0b_12,
   &_vq_book_res0b_13,
  },
  /* psy */
  {&_psy_set0},
  /* thresh sample period, preecho clamp trigger threshhold, range */
  64, 15, 2 
};

#define PREDEF_INFO_MAX 0

#endif
