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
 last mod: $Id: mode_B.h,v 1.6 2000/12/21 21:04:47 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_B_H_
#define _V_MODES_B_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "backends.h"

#include "books/lsp12_0.vqh"
#include "books/lsp30_0.vqh"

#include "books/res0_160_128aux.vqh"
#include "books/res0_160_1024aux.vqh"

#include "books/res0_160_128_1.vqh"
#include "books/res0_160_128_2.vqh"
#include "books/res0_160_128_3.vqh"
#include "books/res0_160_128_4.vqh"
#include "books/res0_160_128_5.vqh"

#include "books/res0_160_1024_1.vqh"
#include "books/res0_160_1024_2.vqh"
#include "books/res0_160_1024_3.vqh"
#include "books/res0_160_1024_4.vqh"
#include "books/res0_160_1024_5.vqh"
#include "books/res0_160_1024_6.vqh"
#include "books/res0_160_1024_7.vqh"
#include "books/res0_160_1024_8.vqh"
#include "books/res0_160_1024_9.vqh"

static vorbis_info_psy _psy_set_B={
  1,/*athp*/
  1,/*decayp*/
  1,/*smoothp*/
  0,.1f,

  -100.f,
  -140.f,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

  1,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {{-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*63*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*88*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*125*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*175*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*250*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*350*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*500*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*700*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1000*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1400*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2000*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2800*/
   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*4000*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*5600*/
   {-30.f,-30.f,-35.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*8000*/
   {-30.f,-30.f,-35.f,-33.f,-35.f,-45.f,-55.f,-65.f,-75.f,-90.f,-100.f}, /*11500*/
   {-30.f,-28.f,-30.f,-32.f,-35.f,-45.f,-55.f,-65.f,-75.f,-90.f,-100.f}, /*16000*/
  },

  1,/* peakattp */
  {{-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*63*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*88*/
   {-14.f,-16.f,-18.f,-19.f,-20.f,-21.f,-22.f,-22.f,-28.f,-28.f,-28.f}, /*125*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-28.f,-28.f,-28.f}, /*175*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-28.f,-28.f,-28.f}, /*250*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-28.f,-28.f,-28.f}, /*350*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-28.f,-28.f,-28.f}, /*500*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*700*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*1000*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*1400*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*2000*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*2800*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*4000*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-18.f,-20.f}, /*5600*/
   { -7.f, -8.f, -9.f,-10.f,-10.f,-11.f,-12.f,-13.f,-15.f,-16.f,-17.f}, /*8000*/
   { -6.f, -7.f, -9.f, -9.f, -9.f, -9.f,-10.f,-11.f,-12.f,-13.f,-14.f}, /*11500*/
   { -6.f, -6.f, -9.f, -9.f, -9.f, -9.f, -9.f, -9.f,-10.f,-11.f,-12.f}, /*16000*/
  },

  1,/*noisemaskp */
  /*  0   10   20   30   40   50   60    70    80    90   100 */
  {{-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*63*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*88*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*125*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*175*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*250*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*350*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*500*/
   {-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-99.f,-199.f,-199.f,-199.f,-199.f}, /*700*/

   {  4.f,  4.f,  4.f,  4.f,  4.f,  2.f,  1.f,   0.f,  -1.f,  -2.f,  -3.f}, /*1000*/
   {  4.f,  4.f,  4.f,  4.f,  4.f,  2.f,  1.f,   0.f,   0.f,  -2.f,  -3.f}, /*1400*/
   {  4.f,  4.f,  4.f,  4.f,  2.f,  2.f,  1.f,   0.f,   0.f,  -2.f,  -3.f}, /*2000*/
   {  4.f,  4.f,  4.f,  4.f,  2.f,  2.f,  1.f,   0.f,   0.f,  -2.f,  -3.f}, /*2800*/
   {  4.f,  4.f,  4.f,  4.f,  4.f,  4.f,  2.f,   0.f,   0.f,   0.f,   0.f}, /*4000*/
   {  4.f,  4.f,  4.f,  4.f,  4.f,  4.f,  2.f,   2.f,   2.f,   2.f,   2.f}, /*5600*/
   {  6.f,  6.f,  6.f,  6.f,  4.f,  4.f,  4.f,   4.f,   4.f,   2.f,   2.f}, /*8000*/
   {  8.f,  8.f,  8.f,  8.f,  6.f,  6.f,  6.f,   6.f,   6.f,   4.f,   4.f}, /*11500*/
   {  8.f,  8.f,  8.f,  8.f,  8.f,  8.f,  8.f,   6.f,   6.f,   6.f,   4.f}, /*16000*/
  },
 
  110.f,

  -3.f, -.004f   /* attack/decay control */
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0B={0};
static vorbis_info_floor0 _floor_set0B={12, 44100,  64, 12,150, 1, {0} };
static vorbis_info_floor0 _floor_set1B={30, 44100, 256, 12,150, 1, {1} };
static vorbis_info_residue0 _residue_set0B={0,128, 32,6,2,
					    {0,1,1,1,1,1},
					    {4,5,6,7,8},
					    
					    {0,99999,9999,9999,9999},
					    {99.f,1.5f,2.5f,6.5f,12.5f},
					    {5,5,5,5,5},
					    {99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1B={0,1024, 32,10,3,
					    {0,1,1,1,1,1,1,1,1,1},
					    {9,10,11,12,13,14,15,16,17},
					   
					    {0,8,9999,17,9999,
					     22,9999,9999,9999},
					    {99.f,1.5f,1.5f,2.5f,2.5f,
					     4.f,4.f,6.5f,12.5f},
					    {5,5,5,5,5,5,5,5,5},
					    {99,99,99,99,99,99,99,99,99}};

static vorbis_info_mapping0 _mapping_set0B={1, {0,0}, {0}, {0}, {0}, {0}};
static vorbis_info_mapping0 _mapping_set1B={1, {0,0}, {0}, {1}, {1}, {0}};
static vorbis_info_mode _mode_set0B={0,0,0,0};
static vorbis_info_mode _mode_set1B={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_B={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   18,   1,
  /* modes */
  {&_mode_set0B,&_mode_set1B},
  /* maps */
  {0,0},{&_mapping_set0B,&_mapping_set1B},
  /* times */
  {0,0},{&_time_set0B},
  /* floors */
  {0,0},{&_floor_set0B,&_floor_set1B},
  /* residue */
  {0,0},{&_residue_set0B,&_residue_set1B},
  /* books */
  {&_vq_book_lsp12_0,      /* 0 */
   &_vq_book_lsp30_0,      /* 1 */

   &_huff_book_res0_160_128aux,
   &_huff_book_res0_160_1024aux,

   &_vq_book_res0_160_128_1,
   &_vq_book_res0_160_128_2,
   &_vq_book_res0_160_128_3,
   &_vq_book_res0_160_128_4,
   &_vq_book_res0_160_128_5,

   &_vq_book_res0_160_1024_1,
   &_vq_book_res0_160_1024_2,
   &_vq_book_res0_160_1024_3,
   &_vq_book_res0_160_1024_4,
   &_vq_book_res0_160_1024_5,
   &_vq_book_res0_160_1024_6,
   &_vq_book_res0_160_1024_7,
   &_vq_book_res0_160_1024_8,
   &_vq_book_res0_160_1024_9,

  },
  /* psy */
  {&_psy_set_B},
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, 32.f, 6.f, -96.f
};

#define PREDEF_INFO_MAX 0

#endif
