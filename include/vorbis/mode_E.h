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
 last mod: $Id: mode_E.h,v 1.1 2000/08/15 09:45:47 xiphmont Exp $

 ********************************************************************/

/* this is really a freeform VBR mode.  It roughly centers on 350 kbps stereo */

#ifndef _V_MODES_E_H_
#define _V_MODES_E_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"

#include "vorbis/book/lsp12_0.vqh"
#include "vorbis/book/lsp30_0.vqh"

#include "vorbis/book/resaux0_128a_350.vqh"
#include "vorbis/book/resaux0_1024a_350.vqh"

#include "vorbis/book/res0_128a_350_1.vqh"
#include "vorbis/book/res0_128a_350_2.vqh"
#include "vorbis/book/res0_128a_350_3.vqh"
#include "vorbis/book/res0_128a_350_4.vqh"
#include "vorbis/book/res0_128a_350_5.vqh"
#include "vorbis/book/res0_1024a_350_1.vqh"
#include "vorbis/book/res0_1024a_350_2.vqh"
#include "vorbis/book/res0_1024a_350_3.vqh"
#include "vorbis/book/res0_1024a_350_4.vqh"
#include "vorbis/book/res0_1024a_350_5.vqh"

static vorbis_info_psy _psy_set_E ={
  1,/*athp*/
  0,/*decayp*/
  1,/*smoothp*/
  0,.1,

  -140.,
  -180.,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  0,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {0},

  1,/* peakattp */
  {{-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*63*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*88*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*125*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*175*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*250*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*350*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*500*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*700*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*1000*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*1400*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*2000*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*2800*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*4000*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*5600*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*8000*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*11500*/
   {-14.,-16.,-18.,-19.,-20.,-21.,-22.,-22.,-22.,-24.,-24.}, /*16000*/
  },

  0,/*noisemaskp */
  /*  0   10   20   30   40   50   60    70    80    90   100 */
  {0},
 
  110.,

  -0., -.004   /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0_E={0};
static vorbis_info_floor0 _floor_set0_E={12, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1_E={30, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0_E={0,128, 32,6,2,
					   {0,1,1,1,1,1},
					    {4,5,6,7,8},

					   {0,9999,9999,9999,9999},
					   {99,2.5,7,13.5,27.5},
					   {5,5,5,5,5},
					   {99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1_E={0,1024, 32,6,3,
					   {0,1,1,1,1,1},
					    {9,10,11,12,13},
					   
					   {0,9999,9999,9999,9999},
					   {99,2.5,7,13.5,27.5},
					   {5,5,5,5,5},
					   {99,99,99,99,99}};

static vorbis_info_mapping0 _mapping_set0_E={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1_E={1, {0,0}, {0}, {1}, {1}, {0}};
static vorbis_info_mode _mode_set0_E={0,0,0,0};
static vorbis_info_mode _mode_set1_E={1,0,0,1};

/* CD quality stereo, no channel coupling */
vorbis_info info_E={
  /* channels, sample rate, upperkbps, nominalkbps, lowerkbps */
  0, 2, 44100, 0,0,0,
  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   14,   1,
  /* modes */
  {&_mode_set0_E,&_mode_set1_E},
  /* maps */
  {0,0},{&_mapping_set0_E,&_mapping_set1_E},
  /* times */
  {0,0},{&_time_set0_E},
  /* floors */
  {0,0},{&_floor_set0_E,&_floor_set1_E},
  /* residue */
  {0,0},{&_residue_set0_E,&_residue_set1_E},
  /* books */
  {&_vq_book_lsp12_0,      /* 0 */
   &_vq_book_lsp30_0,      /* 1 */

   &_huff_book_resaux0_128a_350,
   &_huff_book_resaux0_1024a_350,

   &_vq_book_res0_128a_350_1,
   &_vq_book_res0_128a_350_2,
   &_vq_book_res0_128a_350_3,
   &_vq_book_res0_128a_350_4,
   &_vq_book_res0_128a_350_5,
   &_vq_book_res0_1024a_350_1,
   &_vq_book_res0_1024a_350_2,
   &_vq_book_res0_1024a_350_3,
   &_vq_book_res0_1024a_350_4,
   &_vq_book_res0_1024a_350_5,

  },
  /* psy */
  {&_psy_set_E},
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, 24, 6, -96.
};

#define PREDEF_INFO_MAX 0

#endif
