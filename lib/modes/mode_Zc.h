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
 last mod: $Id: mode_Zc.h,v 1.2 2001/08/13 01:37:14 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_Zc_H_
#define _V_MODES_Zc_H_

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
#include "books/res0_128_1024_7.vqh"
#include "books/res0_128_1024_8.vqh"
#include "books/res0_128_1024_9.vqh"
#include "books/res0_128_1024_9a.vqh"
#include "books/res0_128_1024_9b.vqh"


static vorbis_info_psy_global _psy_set_ZcG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static struct vp_couple_pass _psy_passZc0[]={
  {1.f,1.f,
    {{24,    0,0,       0,0,      0,0},
     {9999,  0,0,   7.5f,12,  7.5f,0}}
  },
};

static vp_couple_pass _psy_passZc[]={
  {1.f,1.f,
    {{288,   0,0,       0,0,      0,0},
     {9999,  0,0,   7.5f,12,  7.5f,0}}
  }
};

static vp_attenblock _vp_tonemask_consbass_Zc={
  {{-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*63*/
  {-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*88*/
  {-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*125*/
  {-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*175*/
  {-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*250*/
  {-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f,-110.f}, /*350*/

  {-35.f,-35.f,-35.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*500*/
  {-35.f,-35.f,-35.f,-45.f,-45.f,-55.f,-65.f,-75.f,-85.f,-95.f,-105.f}, /*700*/

  {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1000*/
  {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*1400*/

  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, /*2000*/
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, 
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, /*2000*/
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, 
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, /*2000*/
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, 
  {-24.f,-24.f,-26.f,-30.f,-30.f,-35.f,-45.f,-55.f,-65.f,-75.f,-90.f}, /*16000*/
}};

static vp_attenblock _vp_tonemask_Zc={

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


  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, /*2000*/
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, 
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, /*2000*/
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, 
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, /*2000*/
  {-30.f,-30.f,-30.f,-30.f,-30.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f}, 
  {-24.f,-24.f,-26.f,-30.f,-30.f,-35.f,-45.f,-55.f,-65.f,-75.f,-90.f}, /*16000*/

}};

static vp_attenblock _vp_peakatt_Zc={
  {{-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*63*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*88*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*125*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*125*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*125*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*125*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-16.f,-18.f,-20.f,-22.f,-24.f,-24.f},/*500*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-24.f},/*700*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-24.f},/*1000*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-24.f},/*1400*/
   {-10.f,-10.f,-10.f,-10.f,-14.f,-14.f,-16.f,-20.f,-22.f,-24.f,-24.f},/*2000*/
   {-10.f,-10.f,-10.f,-12.f,-16.f,-16.f,-16.f,-20.f,-22.f,-24.f,-24.f},/*2400*/
   {-10.f,-10.f,-10.f,-12.f,-16.f,-16.f,-16.f,-20.f,-22.f,-24.f,-24.f},/*4000*/
   {-10.f,-10.f,-10.f,-12.f,-12.f,-14.f,-16.f,-18.f,-22.f,-24.f,-24.f},/*5600*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-14.f,-16.f,-18.f,-22.f,-24.f,-24.f},/*8000*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-14.f,-16.f,-18.f,-22.f,-24.f,-24.f},/*11500*/
   {-10.f,-10.f,-10.f,-10.f,-10.f,-12.f,-16.f,-18.f,-22.f,-24.f,-24.f},/*16000*/
}};

static vorbis_info_psy _psy_set_Zc0={
  ATH_Bark_dB_lineaggressive,

  -100.,
  -110.,

  /* tonemaskp */
  3.f, -24.f,&_vp_tonemask_consbass_Zc,

  /* peakattp */
  1, &_vp_peakatt_Zc,

  /*noisemaskp */
  1,-24.f,     /* suppress any noise curve over maxspec+n */
  1.f, 1.f,   /* low/high window */
  2, 2, -1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-10, -5,  3,  3,  4,  4,  4,  4,  4,  4,  8},
  {1.f,1.f,1.f,1.f,1.f,1.f,.8f,.7f,.7f,.7f,.7f,.7f,.8f,.88f,.89f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,
  _psy_passZc0
};

static vorbis_info_psy _psy_set_ZcT={
  ATH_Bark_dB_lineaggressive,

  -100.f,
  -110.f,

  /* tonemask */
  3.f,-20.f,&_vp_tonemask_consbass_Zc,
  /* peakattp */
  1,  &_vp_peakatt_Zc,

  /*noisemaskp */
  1,  -24.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,100,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-20, -6,  3,  3,  4,  5,  5,   5,  5,  6, 10},
  {1.f,1.f,1.f,1.f,1.f,1.f,.8f,.7f,.7f,.7f,.7f,.7f,.8f,.88f,.89f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_passZc
};

static vorbis_info_psy _psy_set_Zc={
  ATH_Bark_dB_lineaggressive,

  -100.f,
  -110.f,

  /* tonemask */
  -3.f,-20.f,&_vp_tonemask_Zc,
  /* peakattp */
  1,  &_vp_peakatt_Zc,

  /*noisemaskp */
  1,  -24.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,100,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-10,  0,  3,  3,  4,  5,  5,   5,  5,  6, 10},
  {1.f,1.f,1.f,1.f,1.f,1.f,.8f,.7f,.7f,.7f,.7f,.8f,.85f,.88f,.89f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_passZc
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0Zc={0};
/*static vorbis_info_floor0 _floor_set0Zc={9, 44100,  64, 10,130, 2, {0,1},
  0.246f, .387f};*/
/*static vorbis_info_floor0 _floor_set1Zc={30, 44100, 256, 12,150, 2, {2,3}, 
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

static vorbis_info_floor1 _floor_set1Zc={10,
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
					
					60,30,400,
					20,8,1,18.,
					20,600,
					720};

static vorbis_info_residue0 _residue_set0Zc={0,180,12,10,23,
					     {0,1,1,1,1,1,1,1,1,7},
					     {25,
					      26,
					      27,
					      28,
					      29,
					      30,
					      31,
					      32,
					      33,34,35},
					     {9999,
					      9999,
					      9999,
					      9999,
					      2,9999,
					      9999,
					      9999,
					      9999,
					      9999},
					     {.5,
					      1.5,
					      2.5f,
					      7.5,
					      1.5f,
					      1.5,
					      2.5,
					      7.5,
					      22.5f},
					     {0},
					     {99,
					      4,
					      4,
					      4,
					      99,99,
					      99,
					      99,
					      99},
					    {3}};

static vorbis_info_residue0 _residue_set1Zc={0,1408, 32,10,24,
					     {0,1,1,1,1,1,1,1,1,7},
					     {25,
					      26,
					      27,
					      28,
					      29,
					      30,
					      31,
					      32,
					      33,34,35},
					     {9999,
					      9999,
					      9999,
					      9999,
					      3,9999,
					      9999,
					      9999,
					      9999,
					      9999},
					     {.5,
					      1.5,
					      2.5f,
					      7.5,
					      1.5f,
					      1.5,
					      2.5,
					      7.5,
					      22.5f},
					     {0},
					     {99,
					      18,
					      18,
					      18,
					      99,99,
					      99,
					      99,
					      99},
					    {3}};

static vorbis_info_mapping0 _mapping_set0Zc={1, {0,0}, {0}, {0}, {0}, {0,0},
                                            1,{0},{1}};
static vorbis_info_mapping0 _mapping_set1Zc={1, {0,0}, {0}, {1}, {1}, {1,2},
                                            1,{0},{1}};
static vorbis_info_mode _mode_set0Zc={0,0,0,0};
static vorbis_info_mode _mode_set1Zc={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_Zc={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   36,   3,
  /* modes */
  {&_mode_set0Zc,&_mode_set1Zc},
  /* maps */
  {0,0},{&_mapping_set0Zc,&_mapping_set1Zc},
  /* times */
  {0,0},{&_time_set0Zc},
  /* floors */
  {1,1},{&_floor_set0Zc,&_floor_set1Zc},
  /* residue */
  {2,2},{&_residue_set0Zc,&_residue_set1Zc},
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
   &_vq_book_res0_128_1024_7,
   &_vq_book_res0_128_1024_8,
   &_vq_book_res0_128_1024_9,
   &_vq_book_res0_128_1024_9a,
   &_vq_book_res0_128_1024_9b,

  },
  /* psy */
  {&_psy_set_Zc0,&_psy_set_ZcT,&_psy_set_Zc},
  &_psy_set_ZcG
};

#endif
