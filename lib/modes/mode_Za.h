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
 last mod: $Id: mode_Za.h,v 1.2 2001/08/13 01:37:14 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_ZA_H_
#define _V_MODES_ZA_H_

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
#include "books/res0_128_1024_5b.vqh"


static vorbis_info_psy_global _psy_set_ZaG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static struct vp_couple_pass _psy_passZa0[]={
  {1.f,1.f,
    {{24,    0,0,        0,0,      0,0},
     {9999,  0,0,   12.5f,12,  4.5f,0}}
  },
};

static vp_couple_pass _psy_passZa[]={
  {1.f,1.f,
    {{288,   0,0,       0,0,      0,0},
     {9999,  0,0,   12.5f,12,  4.5f,0}}
  }
};

static vp_attenblock _vp_tonemask_consbass_Za={

  {
    {-30.f,-30.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*63*/
    {-30.f,-30.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, 
    {-30.f,-30.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*125*/

    {-30.f,-30.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, 
    {-30.f,-30.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*250*/
    {-30.f,-30.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, 
  
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
}};

static vp_attenblock _vp_tonemask_Za={

  {{-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*63*/
  {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*88*/
  {-30.f,-30.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*125*/
  
  
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
}};

static vp_attenblock _vp_peakatt_Za={
  {{-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*63*/
  {-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*88*/
  {-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*125*/
  {-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*175*/
  {-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*250*/
  {-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*350*/
  {-14.f,-20.f,-20.f,-20.f,-26.f,-22.f,-22.f,-22.f,-22.f,-22.f,-22.f},/*500*/
  {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-14.f,-20.f,-26.f,-22.f,-22.f},/*700*/
  {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-14.f,-20.f,-22.f,-22.f,-22.f},/*1000*/
  {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-22.f,-22.f},/*1400*/
  {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-22.f,-22.f},/*2000*/
  {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-30.f,-30.f},/*2400*/
  {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-30.f,-30.f},/*4000*/
  {-10.f,-10.f,-10.f,-10.f,-10.f,-11.f,-12.f,-13.f,-22.f,-30.f,-30.f},/*5600*/
  {-10.f,-10.f,-10.f,-10.f,-10.f,-11.f,-12.f,-13.f,-22.f,-30.f,-30.f},/*8000*/
  {-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-11.f,-22.f,-30.f,-30.f},/*11500*/
  {-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-10.f,-20.f,-30.f,-30.f},/*16000*/
}};

static vorbis_info_psy _psy_set_Za0={
  ATH_Bark_dB_lineaggressive,

  -100.,
  -140.,

  /* tonemaskp */
  0.f, -20.f,&_vp_tonemask_consbass_Za,

  /* peakattp */
  1, &_vp_peakatt_Za,

  /*noisemaskp */
  1,-20.f,     /* suppress any noise curve over absolute dB */
  .6f, .6f,   /* low/high window */
  5, 5, -1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-30,-30,-30,-30,-30,-20,-20,-10,-10,-10,-10,-10, -3,  1,  2,  3,  6},
  {.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,
  _psy_passZa0
};

static vorbis_info_psy _psy_set_Za={
  ATH_Bark_dB_lineaggressive,

  -100.f,
  -140.f,

  /* tonemask */
  0.f,-20.f,&_vp_tonemask_Za,
  /* peakattp */
  1,  &_vp_peakatt_Za,

  /*noisemaskp */
  1,  -20.f,     /* suppress any noise curve over maxspec+n */
      .4f,.4f,   /* low/high window */
      10,10,-1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-6,-6,-6,-6,-6,-6, -6, -6,-10,-10,-10,-10,-3, -1,  1,  2,  3},
  {.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_passZa
};

static vorbis_info_psy _psy_set_ZaT={
  ATH_Bark_dB_lineaggressive,

  -100.f,
  -140.f,

  /* tonemask */
  0.f,-20.f,&_vp_tonemask_Za,
  /* peakattp */
  1,  &_vp_peakatt_Za,

  /*noisemaskp */
  1,  -20.f,     /* suppress any noise curve over maxspec+n */
      .4f,.4f,   /* low/high window */
      10,10,-1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-30,-30,-30,-30,-30,-20, -6,-6,-10,-10,-10,-10, -3, -1,  1,  2,  3},
  {.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_passZa
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0Za={0};
/*static vorbis_info_floor0 _floor_set0A={9, 44100,  64, 10,130, 2, {0,1},
  0.246f, .387f};*/
/*static vorbis_info_floor0 _floor_set1A={30, 44100, 256, 12,150, 2, {2,3}, 
  .082f, .126f};*/

static vorbis_info_floor1 _floor_set0Zc={3,
					{0,1,2},
					
					{1,3,3},
					{0,2,2},
					{-1,0,1},
					{{2},{-1,3,4,5},{-1,6,7,8}},

                                        4,

					{0,128,  

					 7,

					 2,1,4,
					 23,13,45},

                                        60,30,500,
                                        999,999,0,18.,
                                        8,70,
                                        90};


static vorbis_info_floor1 _floor_set1Za={10,
					{0,1,2,2,2,2,2, 3,3,3},
					
					{3,4,3,3},
					{1,1,2,2},
					{9,10,11,12},
					{{13,14},
					 {15,16},
					 {-1,17,18,19},
					 {-1,20,21,22},
					},

					4,
					{0,1024,

					 88,31,243,

					 14,54,143,460,
					 
					 6,3,10, 22,18,26, 41,36,47, 
					 69,61,78, 112,99,126, 185,162,211,  
					 329,282,387, 672,553,825
					 },
					
					60,30,500,
					20,8,1,18.,
					20,600,
					704};

static vorbis_info_residue0 _residue_set0Za={0,192,16,5,23,
					    {1,1,1,1,7},
					    {25,
					     26,
					     27,
					     28,
					     29,30,31},
					    {9999,
					     9999,
					     9999,
					     9999,
					     9999},
					    {4.5,
					     12.5,
					     1.5f,
					     7.5f},
					    {0},
					     {3,3,99,99,99},
					    {3}};

static vorbis_info_residue0 _residue_set1Za={0,1408, 32,5,24,
					     {1,1,1,1,7},
					    {25,
					     26,
					     27,
					     28,
					     29,30,31},
					     {9999,
					      9999,
					      9999,
					      9999,
					      9999},
					     {4.5f,
					      12.5,
					      1.5,
					      7.5},
					     {0},
					     {18,18,99,99,99},
					     {3}};

static vorbis_info_mapping0 _mapping_set0Za={1, {0,0}, {0}, {0}, {0}, {0,0},
                                            1,{0},{1}};
static vorbis_info_mapping0 _mapping_set1Za={1, {0,0}, {0}, {1}, {1}, {1,2},
                                            1,{0},{1}};
static vorbis_info_mode _mode_set0Za={0,0,0,0};
static vorbis_info_mode _mode_set1Za={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_Za={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   32,   3,
  /* modes */
  {&_mode_set0Za,&_mode_set1Za},
  /* maps */
  {0,0},{&_mapping_set0Za,&_mapping_set1Za},
  /* times */
  {0,0},{&_time_set0Za},
  /* floors */
  {1,1},{&_floor_set0Za,&_floor_set1Za},
  /* residue */
  {2,2},{&_residue_set0Za,&_residue_set1Za},
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
   &_vq_book_res0_128_1024_5b,

  },
  /* psy */
  {&_psy_set_Za0,&_psy_set_ZaT,&_psy_set_Za},
  &_psy_set_ZaG
};

#endif
