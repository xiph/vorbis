/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: predefined encoding modes
 last mod: $Id: mode_C.h,v 1.13.2.2 2001/08/03 06:48:13 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_C_H_
#define _V_MODES_C_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "backends.h"

#include "books/line0_class1.vqh"
#include "books/line0_class2.vqh"
#include "books/line1_class0.vqh"
#include "books/line1_class1.vqh"
#include "books/line1_class2.vqh"
#include "books/line1_class3.vqh"

#include "books/line0_0sub0.vqh"
#include "books/line0_1sub1.vqh"
#include "books/line0_1sub2.vqh"
#include "books/line0_1sub3.vqh"
#include "books/line0_2sub1.vqh"
#include "books/line0_2sub2.vqh"
#include "books/line0_2sub3.vqh"

#include "books/line1_0sub0.vqh"
#include "books/line1_0sub1.vqh"
#include "books/line1_1sub0.vqh"
#include "books/line1_1sub1.vqh"
#include "books/line1_2sub1.vqh"
#include "books/line1_2sub2.vqh"
#include "books/line1_2sub3.vqh"
#include "books/line1_3sub1.vqh"
#include "books/line1_3sub2.vqh"
#include "books/line1_3sub3.vqh"

#include "books/res0_128_128aux.vqh"
#include "books/res0_128_1024aux.vqh"

#include "books/res0_128_1024_1.vqh"
#include "books/res0_128_1024_2.vqh"
#include "books/res0_128_1024_3.vqh"
#include "books/res0_128_1024_4.vqh"
#include "books/res0_128_1024_5.vqh"
#include "books/res0_128_1024_5a.vqh"
#include "books/res0_128_1024_6.vqh"
#include "books/res0_128_1024_6a.vqh"
#include "books/res0_128_1024_7.vqh"
#include "books/res0_128_1024_7a.vqh"
#include "books/res0_128_1024_8.vqh"
#include "books/res0_128_1024_8a.vqh"
#include "books/res0_128_1024_8b.vqh"


static vorbis_info_psy_global _psy_set_CG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static vorbis_info_psy _psy_set_C0={
  ATH_Bark_dB_lineaggressive,

  -100.,
  -140.,
  0.f,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

   1,/* tonemaskp */
  0.f,
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*63*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*88*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*125*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*175*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*250*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*350*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*500*/
   {-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.,-999.}, /*700*/

   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1000*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1400*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2000*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2800*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*4000*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*5600*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*8000*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*11500*/
   {-30.f,-30.f,-33.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*16000*/

  },

  1,/* peakattp */
  {{-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*63*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*88*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*125*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*175*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*250*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*350*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*500*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*700*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*1000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*1400*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*2000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*2800*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*4000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*5600*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*8000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*11500*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*16000*/
  },

  1,/*noisemaskp */
  -0.f,  /* suppress any noise curve over maxspec+n */
  .5f,   /* low window */
  .5f,   /* high window */
  5,
  5,
  10,
  {.000f, 0.f,/*63*/
   .000f, 0.f,/*88*/
   .000f, 0.f,/*125*/
   .000f, 0.f,/*175*/
   .000f, 0.f,/*250*/
   .000f, 0.f,/*350*/
   .000f, 0.f,/*500*/
   .200f, -2.f,/*700*/
   .500f, -2.f,/*1000*/
   .500f, -2.f,/*1400*/
   .500f, -3.f,/*2000*/
   .500f, -6.f,/*2800*/
   .500f, -3.f,/*4000*/
   .700f, -4.f,/*5600*/
   .850f, -5.f,/*8000*/
   .900f, -4.f,/*11500*/
   .900f, -3.f,/*16000*/
  },
 
  90.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  -44.,

  12,
  NULL,NULL
};

static vorbis_info_psy _psy_set_C={
  ATH_Bark_dB_lineaggressive,

  -100.f,
  -140.f,
  0.f,
  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */
   1,/* tonemaskp */
  0.f,
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {
   {-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f}, /*63*/
   {-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f}, /*88*/
   {-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f}, /*125*/

   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*175*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*250*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*350*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*500*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*700*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1000*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1400*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2000*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2800*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*4000*/

   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*5600*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*8000*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*11500*/
   {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*16000*/

  },

  1,/* peakattp */
  {{-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*63*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*88*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*125*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*175*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*250*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*350*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*500*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*700*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*1000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*1400*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*2000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*2800*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*4000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*5600*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*8000*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*11500*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-40.f,-40.f,-40.f,-40.f,-40.f},/*16000*/
  },

  1,/*noisemaskp */
  -00.f,  /* suppress any noise curve over maxspec+n */
  .5f,   /* low window */
  .5f,   /* high window */
  10,
  10,
  25,
  {.000f, 0.f, /*63*/
   .000f, 0.f, /*88*/
   .000f, 0.f, /*125*/
   .000f, 0.f, /*175*/
   .000f, 0.f, /*250*/
   .000f, 0.f, /*350*/
   .000f, 0.f, /*500*/
   .000f, 0.f, /*700*/
   .000f, 0.f, /*1000*/
   .300f,-4.f, /*1400*/
   .300f,-4.f, /*2000*/
   .300f,-4.f, /*2800*/
   .500f,-4.f, /*4000*/
   .700f,-4.f, /*5600*/
   .850f,-5.f, /*8000*/
   .900f,-5.f, /*11500*/
   .900f,-4.f, /*16000*/
  },
 
  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  -44.,

  32,NULL,NULL
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0C={0};
/*static vorbis_info_floor0 _floor_set0A={9, 44100,  64, 10,130, 2, {0,1},
  0.246f, .387f};*/
/*static vorbis_info_floor0 _floor_set1A={30, 44100, 256, 12,150, 2, {2,3}, 
  .082f, .126f};*/

static vorbis_info_floor1 _floor_set0C={5,
					{0,1,1,2,2},
					
					{1,3,3},
					{0,2,2},
					{-1,0,1},
					{{2},{-1,3,4,5},{-1,6,7,8}},

					4,

					{0,128,  

					 26,7,48,

					 2,1,4, 18,13,22, 
					 35,30,41, 69,57,84},

					80,30,400,
					999,999,0,18.,
					8,70,
					128};

static vorbis_info_floor1 _floor_set1C={10,
					{0,1,2,2,2,2,2, 3,3,3},
					
					{3,4,3,3},
					{1,1,2,2},
					{9,10,11,12},
					{{13,14},
					 {15,16},
					 {-1,17,18,19},
					 {-1,20,21,22},
					},

					2,
					{0,1024,

					 88,31,243,

					 14,54,143,460,
					 
					 6,3,10, 22,18,26, 41,36,47, 
					 69,61,78, 112,99,126, 185,162,211,  
					 329,282,387, 672,553,825
					 },
					
					60,30,400,
					20,8,1,18.,
					20,600,
					960};

static vorbis_info_residue0 _residue_set0C={0,240,12,9,23,
					    {0,1,1,1,1,3,3,3,7},
					    {25,
					     26,
					     27,
					     28,
					     29,30,
					     31,32,
					     33,34,
					     35,36,37},
					    {9999,4,9999,10,9999,18,
					     9999,9999,9999},
					    {.5f,1.5f,1.5f,2.5f,2.5f,
					     7.5f,7.5f,22.5f},
					    {0},
					    {99,99,99,99,99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1C={0,1920, 32,9,24,
					    {0,1,1,1,1,3,3,3,7},
					    {25,
					     26,
					     27,
					     28,
					     29,30,
					     31,32,
					     33,34,
					     35,36,37},
					    {9999,6,9999,24,9999,40,
					     9999,9999,9999},
					    {.5f,1.5f,1.5f,2.5f,2.5f,
					     7.5f,7.5f,22.5f},
					    {0},
					    {99,99,99,99,99,99,99,99,99}};

static vorbis_info_mapping0 _mapping_set0C={1, {0,0}, {0}, {0}, {0}, 0,
                                            1,{0},{1}};
static vorbis_info_mapping0 _mapping_set1C={1, {0,0}, {0}, {1}, {1}, 1,
                                            1,{0},{1}};
static vorbis_info_mode _mode_set0C={0,0,0,0};
static vorbis_info_mode _mode_set1C={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_C={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   38,   2,
  /* modes */
  {&_mode_set0C,&_mode_set1C},
  /* maps */
  {0,0},{&_mapping_set0C,&_mapping_set1C},
  /* times */
  {0,0},{&_time_set0C},
  /* floors */
  {1,1},{&_floor_set0C,&_floor_set1C},
  /* residue */
  {2,2},{&_residue_set0C,&_residue_set1C},
  /* books */
    
  {  
   &_huff_book_line0_class1,
   &_huff_book_line0_class2, /* 1 */
   
   &_huff_book_line0_0sub0,  /* 2 */
   &_huff_book_line0_1sub1,  /* 3 */
   &_huff_book_line0_1sub2,
   &_huff_book_line0_1sub3,  /* 5 */
   &_huff_book_line0_2sub1,
   &_huff_book_line0_2sub2,  /* 7 */
   &_huff_book_line0_2sub3, 

   &_huff_book_line1_class0,
   &_huff_book_line1_class1, /* 10 */
   &_huff_book_line1_class2,
   &_huff_book_line1_class3, /* 12 */

   &_huff_book_line1_0sub0,
   &_huff_book_line1_0sub1, /* 14 */
   &_huff_book_line1_1sub0, 
   &_huff_book_line1_1sub1,
   &_huff_book_line1_2sub1,  
   &_huff_book_line1_2sub2, /* 18 */
   &_huff_book_line1_2sub3, 
   &_huff_book_line1_3sub1,
   &_huff_book_line1_3sub2,
   &_huff_book_line1_3sub3, /* 22 */

   &_huff_book_res0_128_128aux, 
   &_huff_book_res0_128_1024aux,

   &_vq_book_res0_128_1024_1,
   &_vq_book_res0_128_1024_2,
   &_vq_book_res0_128_1024_3,
   &_vq_book_res0_128_1024_4,
   &_vq_book_res0_128_1024_5,
   &_vq_book_res0_128_1024_5a,
   &_vq_book_res0_128_1024_6,
   &_vq_book_res0_128_1024_6a,
   &_vq_book_res0_128_1024_7,
   &_vq_book_res0_128_1024_7a,
   &_vq_book_res0_128_1024_8,
   &_vq_book_res0_128_1024_8a,
   &_vq_book_res0_128_1024_8b,

  },
  /* psy */
  {&_psy_set_C0,&_psy_set_C},
  &_psy_set_CG
};

#endif
