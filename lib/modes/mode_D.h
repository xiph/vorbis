/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: predefined encoding modes
 last mod: $Id: mode_D.h,v 1.7 2000/12/21 21:04:47 xiphmont Exp $

 ********************************************************************/

/* this is really a freeform VBR mode.  It roughly centers on 256 kbps stereo */

#ifndef _V_MODES_D_H_
#define _V_MODES_D_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "backends.h"

#include "books/lsp12_0.vqh"
#include "books/lsp30_0.vqh"

#include "books/res0_256_128aux.vqh"
#include "books/res0_256_1024aux.vqh"

#include "books/res0_256_128_1.vqh"
#include "books/res0_256_128_2.vqh"
#include "books/res0_256_128_3.vqh"
#include "books/res0_256_128_4.vqh"
#include "books/res0_256_128_5.vqh"

#include "books/res0_256_1024_1.vqh"
#include "books/res0_256_1024_2.vqh"
#include "books/res0_256_1024_3.vqh"
#include "books/res0_256_1024_4.vqh"
#include "books/res0_256_1024_5.vqh"

static vorbis_info_psy _psy_set_D0 ={
  1,/*athp*/
  0,/*decayp*/
  1,/*smoothp*/
  0,.1f,

  -100.f,
  -140.f,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  0,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {{0}},

  1,/* peakattp */
  {{-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*63*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*88*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*125*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*175*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*250*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*350*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*500*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*700*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*1000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*1400*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*2000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*2800*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*4000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*5600*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*8000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*11500*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*16000*/
  },

  0,/*noisemaskp */
  /*  0   10   20   30   40   50   60    70    80    90   100 */
  {{0}},
 
  110.f,

  -0.f, -.004f   /* attack/decay control */
};
static vorbis_info_psy _psy_set_D1 ={
  1,/*athp*/
  0,/*decayp*/
  1,/*smoothp*/
  0,.1f,

  -100.f,
  -140.f,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  1,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {{-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*63*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*88*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*125*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*175*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*250*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*350*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*500*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*700*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*1000*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*1400*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*2000*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*2800*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*4000*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*5600*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*8000*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*11500*/
   {-40.f,-40.f,-40.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*16000*/
  },

  1,/* peakattp */
  {{-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*63*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*88*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*125*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*175*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*250*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*350*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*500*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*700*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*1000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*1400*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*2000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*2800*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*4000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*5600*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*8000*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*11500*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-22.f,-22.f,-22.f}, /*16000*/
  },

  0,/*noisemaskp */
  /*  0   10   20   30   40   50   60    70    80    90   100 */
  {{0}},
 
  110.f,

  -0.f, -.004f   /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0_256={0};
static vorbis_info_floor0 _floor_set0_256={12, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1_256={30, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0_256={0,128, 32,6,2,
					   {0,1,1,1,1,1},
					    {4,5,6,7,8},

					   {0,9999,9999,9999,9999},
					   {99.f,2.5f,6.5f,15.5f,29.5f},
					   {5,5,5,5,5},
					   {99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1_256={0,1024, 32,6,3,
					   {0,1,1,1,1,1},
					    {9,10,11,12,13},
					   
					   {0,9999,9999,9999,9999},
					   {99.f,2.5f,6.5f,15.5f,29.5f},
					   {5,5,5,5,5},
					   {99,99,99,99,99}};

static vorbis_info_mapping0 _mapping_set0_256={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1_256={1, {0,0}, {0}, {1}, {1}, {1}};
static vorbis_info_mode _mode_set0_256={0,0,0,0};
static vorbis_info_mode _mode_set1_256={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_D={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   14,   2,
  /* modes */
  {&_mode_set0_256,&_mode_set1_256},
  /* maps */
  {0,0},{&_mapping_set0_256,&_mapping_set1_256},
  /* times */
  {0,0},{&_time_set0_256},
  /* floors */
  {0,0},{&_floor_set0_256,&_floor_set1_256},
  /* residue */
  {0,0},{&_residue_set0_256,&_residue_set1_256},
  /* books */
  {&_vq_book_lsp12_0,      /* 0 */
   &_vq_book_lsp30_0,      /* 1 */

   &_huff_book_res0_256_128aux,
   &_huff_book_res0_256_1024aux,

   &_vq_book_res0_256_128_1,
   &_vq_book_res0_256_128_2,
   &_vq_book_res0_256_128_3,
   &_vq_book_res0_256_128_4,
   &_vq_book_res0_256_128_5,

   &_vq_book_res0_256_1024_1,
   &_vq_book_res0_256_1024_2,
   &_vq_book_res0_256_1024_3,
   &_vq_book_res0_256_1024_4,
   &_vq_book_res0_256_1024_5,

  },
  /* psy */
  {&_psy_set_D0,&_psy_set_D1},
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, 24.f, 6.f, -96.f
};

#define PREDEF_INFO_MAX 0

#endif
