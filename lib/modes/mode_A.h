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
 last mod: $Id: mode_A.h,v 1.14.4.5.2.1 2001/07/08 08:48:07 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_A_H_
#define _V_MODES_A_H_

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
#include "books/res0_128_1024_6.vqh"
#include "books/res0_128_1024_6a.vqh"
#include "books/res0_128_1024_7.vqh"
#include "books/res0_128_1024_7a.vqh"
#include "books/res0_128_1024_8.vqh"
#include "books/res0_128_1024_8a.vqh"
#include "books/res0_128_1024_9.vqh"
#include "books/res0_128_1024_9a.vqh"
#include "books/res0_128_1024_9b.vqh"


static vorbis_info_psy_global _psy_set_AG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static vorbis_info_psy _psy_set_A0={
  ATH_Bark_dB_lineaggressive,

  -100.,
  -140.,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */

   1,/* tonemaskp */
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

   {-30.f,-30.f,-33.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*8000*/
   {-30.f,-30.f,-33.f,-35.f,-35.f,-45.f,-50.f,-60.f,-70.f,-90.f,-100.f}, /*11500*/
   {-24.f,-24.f,-26.f,-32.f,-32.f,-42.f,-50.f,-60.f,-70.f,-90.f,-100.f}, /*16000*/


  },

  1,/* peakattp */
  {{-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*63*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*88*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*125*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*175*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*250*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*350*/
   {-14.f,-20.f,-20.f,-20.f,-26.f,-32.f,-32.f,-32.f,-32.f,-32.f,-32.f},/*500*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-14.f,-20.f,-26.f,-26.f,-26.f},/*700*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-14.f,-20.f,-22.f,-22.f,-22.f},/*1000*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-22.f,-22.f},/*1400*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-22.f,-22.f},/*2000*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-22.f,-22.f},/*2400*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-22.f,-22.f},/*4000*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-11.f,-12.f,-13.f,-22.f,-22.f,-22.f},/*5600*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-11.f,-12.f,-13.f,-22.f,-22.f,-22.f},/*8000*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-11.f,-22.f,-22.f,-22.f},/*11500*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-20.f,-21.f,-22.f},/*16000*/
  },

  1,/*noisemaskp */
  -10.f,  /* suppress any noise curve over maxspec+n */
  .5f,   /* low window */
  .5f,   /* high window */
  5,
  5,
  -1,
  {.300f, 0.f,/*63*/
   .300f, 0.f,/*88*/
   .300f, 0.f,/*125*/
   .300f, 0.f,/*175*/
   .300f, 0.f,/*250*/
   .300f, 0.f,/*350*/
   .300f, 0.f,/*500*/
   .300f, 0.f,/*700*/
   .500f, 0.f,/*1000*/
   .500f, 0.f,/*1400*/
   .500f, 0.f,/*2000*/
   .500f, 0.f,/*2800*/
   .700f, 0.f,/*4000*/
   .700f, 0.f,/*5600*/
   .850f, 0.f,/*8000*/
   .900f, 0.f,/*11500*/
   .900f, 0.f,/*16000*/
  },
 
  90.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  -26.,

};

static vorbis_info_psy _psy_set_A={
  ATH_Bark_dB_lineaggressive,

  -100.f,
  -140.f,

  /*     0  1  2   3   4   5   6   7   8   9  10  11  12  13  14  15   16   */
  /* x: 63 88 125 175 250 350 500 700 1k 1.4k 2k 2.8k 4k 5.6k 8k 11.5k 16k Hz */
  /* y: 0 10 20 30 40 50 60 70 80 90 100 dB */
   1,/* tonemaskp */
  /*  0   10   20   30   40   50   60   70   80   90   100 */
  {
   {-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f}, /*63*/
   {-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f}, /*88*/
   {-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f,-999.f}, /*125*/

   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*175*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*250*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*350*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*500*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*700*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1000*/
   {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1400*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2000*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2800*/
   {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*4000*/

   {-30.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*5600*/

   {-30.f,-30.f,-33.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*8000*/
   {-30.f,-30.f,-33.f,-35.f,-40.f,-45.f,-50.f,-60.f,-70.f,-85.f,-100.f}, /*11500*/
   {-24.f,-24.f,-26.f,-32.f,-32.f,-42.f,-50.f,-60.f,-70.f,-85.f,-100.f}, /*16000*/

  },

  1,/* peakattp */
  {{-14.f,-16.f,-18.f,-19.f,-24.f,-30.f,-30.f,-30.f,-30.f,-30.f,-32.f},/*63*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-30.f,-30.f,-30.f,-30.f,-30.f,-32.f},/*88*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-30.f,-30.f,-30.f,-30.f,-30.f,-32.f},/*125*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-20.f,-26.f,-30.f,-30.f,-30.f,-32.f},/*175*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-16.f,-18.f,-26.f,-30.f,-30.f,-32.f},/*250*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-16.f,-18.f,-20.f,-22.f,-24.f,-26.f},/*350*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-16.f,-18.f,-20.f,-22.f,-24.f,-26.f},/*500*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-26.f},/*700*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-26.f},/*1000*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-26.f},/*1400*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-26.f},/*2000*/
   {-10.f,-10.f,-10.f,-12.f,-16.f,-16.f,-16.f,-20.f,-22.f,-24.f,-26.f},/*2400*/
   {-10.f,-10.f,-10.f,-12.f,-16.f,-16.f,-16.f,-20.f,-22.f,-24.f,-26.f},/*4000*/
   {-10.f,-10.f,-10.f,-12.f,-12.f,-14.f,-16.f,-18.f,-22.f,-24.f,-26.f},/*5600*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-14.f,-16.f,-18.f,-22.f,-24.f,-26.f},/*8000*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-14.f,-16.f,-18.f,-22.f,-24.f,-26.f},/*11500*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-12.f,-16.f,-18.f,-22.f,-24.f,-24.f},/*16000*/
  },

  1,/*noisemaskp */
  -24.f,  /* suppress any noise curve over maxspec+n */
  .5f,   /* low window */
  .5f,   /* high window */
  10,
  10,
  -1,
  {.300f, 0.f, /*63*/
   .300f, 0.f, /*88*/
   .300f, 0.f, /*125*/
   .300f, 0.f, /*175*/
   .400f, 0.f, /*250*/
   .400f, 0.f, /*350*/
   .400f, 0.f, /*500*/
   .400f, 0.f, /*700*/
   .300f, 0.f, /*1000*/
   .300f, 0.f, /*1400*/
   .300f, 0.f, /*2000*/
   .300f, 0.f, /*2800*/
   .700f, 0.f, /*4000*/
   .850f, 0.f, /*5600*/
   .900f, 0.f, /*8000*/
   .900f, 0.f, /*11500*/
   .900f, 1.f, /*16000*/
  },
 
  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  -28.,

};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0A={0};
/*static vorbis_info_floor0 _floor_set0A={9, 44100,  64, 10,130, 2, {0,1},
  0.246f, .387f};*/
/*static vorbis_info_floor0 _floor_set1A={30, 44100, 256, 12,150, 2, {2,3}, 
  .082f, .126f};*/

static vorbis_info_floor1 _floor_set0A={3,
					{0,1,2},
					
					{1,3,3},
					{0,2,2},
					{-1,0,1},
					{{2},{-1,3,4,5},{-1,6,7,8}},

					2,

					{0,128,  

					 7,

					 2,1,4,
					 23,13,45},
					
					60,30,500,
					999,999,0,18.,
					8,70,
					96};

static vorbis_info_floor1 _floor_set1A={10,
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
					704};

static vorbis_info_residue0 _residue_set0A={0,192,12,10,23,
					    {0,1,1,1,1,1,3,3,3,7},
					    {25,26,
					     27,
					     28,
					     29,
					     30,
					     31,32,
					     33,34,
					     35,36,
					     37,38},
					    {9999,9999,3,9999,4,9999,8,
					     9999,9999,9999},
					    {.7f,7.5f,1.5f,1.5f,2.5f,2.5f,
					     7.5f,7.5f,22.5f},
					    {0},
					    {99,4,99,99,99,99,99,99,99,99}};

static vorbis_info_residue0 _residue_set1A={0,1408, 32,10,24,
					    {0,1,1,1,1,1,3,3,3,7},
					    {25,26,
					     27,
					     28,
					     29,
					     30,
					     31,32,
					     33,34,
					     35,36,
					     37,38},
					    {9999,9999,5,9999,10,9999,20,
					     9999,9999,9999},
					    {.7f,7.5f,1.5f,1.5f,2.5f,2.5f,
					     7.5f,7.5f,22.5f},
					    {0},
					    {99,12,99,99,99,99,99,99,99,99}};

static vorbis_info_mapping0 _mapping_set0A={1, {0,0}, {0}, {0}, {0}, {0},
                                            1,{0},{1}};
static vorbis_info_mapping0 _mapping_set1A={1, {0,0}, {0}, {1}, {1}, {1},
                                            1,{0},{1}};
static vorbis_info_mode _mode_set0A={0,0,0,0};
static vorbis_info_mode _mode_set1A={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_A={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   39,   2,
  /* modes */
  {&_mode_set0A,&_mode_set1A},
  /* maps */
  {0,0},{&_mapping_set0A,&_mapping_set1A},
  /* times */
  {0,0},{&_time_set0A},
  /* floors */
  {1,1},{&_floor_set0A,&_floor_set1A},
  /* residue */
  {2,2},{&_residue_set0A,&_residue_set1A},
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
   &_vq_book_res0_128_1024_6,
   &_vq_book_res0_128_1024_6a,
   &_vq_book_res0_128_1024_7,
   &_vq_book_res0_128_1024_7a,
   &_vq_book_res0_128_1024_8,
   &_vq_book_res0_128_1024_8a,
   &_vq_book_res0_128_1024_9,
   &_vq_book_res0_128_1024_9a,
   &_vq_book_res0_128_1024_9b,

  },
  /* psy */
  {&_psy_set_A0,&_psy_set_A},
  &_psy_set_AG
};

#endif
