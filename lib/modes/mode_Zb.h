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
 last mod: $Id: mode_Zb.h,v 1.2 2001/08/13 01:37:14 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_Zb_H_
#define _V_MODES_Zb_H_

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


static vorbis_info_psy_global _psy_set_ZbG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static struct vp_couple_pass _psy_passZb0[]={
  {1.f,1.f,
    {{24,    0,0,       0,0,      0,0},
     {9999,  0,0,   7.5f,12,  7.5f,0}}
  },
};

static vp_couple_pass _psy_passZb[]={
  {1.f,1.f,
    {{288,   0,0,       0,0,      0,0},
     {9999,  0,0,   7.5f,12,  7.5f,0}}
  }
};

static vp_attenblock _vp_tonemask_consbass_Zb={
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
  {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2000*/
  {-40.f,-40.f,-40.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*2800*/
  {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*4000*/
  {-35.f,-35.f,-35.f,-40.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*5600*/
  
  {-30.f,-30.f,-33.f,-35.f,-40.f,-50.f,-60.f,-70.f,-80.f,-90.f,-100.f}, /*8000*/
  {-30.f,-30.f,-33.f,-35.f,-35.f,-45.f,-50.f,-60.f,-70.f,-90.f,-100.f}, /*11500*/
  {-24.f,-24.f,-26.f,-32.f,-32.f,-42.f,-50.f,-60.f,-70.f,-90.f,-100.f}, /*16000*/
}};

static vp_attenblock _vp_tonemask_Zb={

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

static vp_attenblock _vp_peakatt_Zb={
  {{-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*63*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*88*/
   {-14.f,-16.f,-18.f,-19.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f,-24.f},/*125*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-20.f,-26.f,-24.f,-24.f,-24.f,-24.f},/*175*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-16.f,-18.f,-26.f,-24.f,-24.f,-24.f},/*250*/
   {-10.f,-10.f,-10.f,-10.f,-16.f,-16.f,-18.f,-20.f,-22.f,-24.f,-24.f},/*350*/
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

static vorbis_info_psy _psy_set_Zb0={
  ATH_Bark_dB_lineaggressive,

  -90.,
  -140.,

  /* tonemaskp */
  6.f, -20.f,&_vp_tonemask_consbass_Zb,

  /* peakattp */
  1, &_vp_peakatt_Zb,

  /*noisemaskp */
  1,-24.f,     /* suppress any noise curve over maxspec+n */
  .5f, .5f,   /* low/high window */
  5, 5, -1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-10,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1},
  {1.f,1.f,1.f,1.f,1.f,1.f,1.f,.8f,.8f,.8f,.8f,.8f,.85f,.95f,.95f,1.f,1.f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,
  _psy_passZb0
};

static vorbis_info_psy _psy_set_Zb={
  ATH_Bark_dB_lineaggressive,

  -90.f,
  -140.f,

  /* tonemask */
  6.f,-20.f,&_vp_tonemask_Zb,
  /* peakattp */
  1,  &_vp_peakatt_Zb,

  /*noisemaskp */
  1,  -24.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,-1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-10,-10,-10,-10,-10,-10,-10,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3},
  {.8f,.8f,.8f,.8f,.8f,.8f,.8f,.8f,.8f,.8f,.8f,.8f,.9f,.9f,.9f,.95f,.95f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_passZb
};

static vorbis_info_psy _psy_set_ZbT={
  ATH_Bark_dB_lineaggressive,

  -90.f,
  -140.f,

  /* tonemask */
  6.f,-20.f,&_vp_tonemask_consbass_Zb,
  /* peakattp */
  1,  &_vp_peakatt_Zb,

  /*noisemaskp */
  1,  -24.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,-1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-10,  0,  0,  0,  0,  0,  0,   0,  1,  2,  3},
  {1.f,1.f,1.f,1.f,1.f,1.f,.9f,.8f,.8f,.8f,.8f,.8f,.9f,.9f,.9f,.95f,.95f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_passZb
};

/* with GNUisms, this could be short and readable. Oh well */
static vorbis_info_time0 _time_set0Zb={0};
/*static vorbis_info_floor0 _floor_set0Zb={9, 44100,  64, 10,130, 2, {0,1},
  0.246f, .387f};*/
/*static vorbis_info_floor0 _floor_set1Zb={30, 44100, 256, 12,150, 2, {2,3}, 
  .082f, .126f};*/

static vorbis_info_floor1 _floor_set0Zb={3,
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

static vorbis_info_floor1 _floor_set1Zb={10,
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

static vorbis_info_residue0 _residue_set0Zb={0,180,12,5,23,
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
					    {2.5f,
					     7.5,
					     1.5f,
					     22.5f},
					    {0},
					    {2,
					     2,
					     99,
					     99,
					     99},
					    {3}};

static vorbis_info_residue0 _residue_set1Zb={0,1440, 32,5,24,
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
					    {2.5f,
					     7.5f,
					     1.5f,
					     22.5f},
					    {0},
					    {18,
					     18,
					     99,
					     99,
					     99},
					    {3}};

static vorbis_info_mapping0 _mapping_set0Zb={1, {0,0}, {0}, {0}, {0}, {0,0},
                                            1,{0},{1}};
static vorbis_info_mapping0 _mapping_set1Zb={1, {0,0}, {0}, {1}, {1}, {1,2},
                                            1,{0},{1}};
static vorbis_info_mode _mode_set0Zb={0,0,0,0};
static vorbis_info_mode _mode_set1Zb={1,0,0,1};

/* CD quality stereo, no channel coupling */
codec_setup_info info_Zb={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   32,   3,
  /* modes */
  {&_mode_set0Zb,&_mode_set1Zb},
  /* maps */
  {0,0},{&_mapping_set0Zb,&_mapping_set1Zb},
  /* times */
  {0,0},{&_time_set0Zb},
  /* floors */
  {1,1},{&_floor_set0Zb,&_floor_set1Zb},
  /* residue */
  {2,2},{&_residue_set0Zb,&_residue_set1Zb},
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
  {&_psy_set_Zb0,&_psy_set_ZbT,&_psy_set_Zb},
  &_psy_set_ZbG
};

#endif
