/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: predefined encoding modes; 44kHz stereo ~64kbps true VBR
 last mod: $Id: mode_44_B.h,v 1.1 2001/08/13 11:30:59 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_44_B_H_
#define _V_MODES_44_B_H_

#include <stdio.h>
#include "vorbis/codec.h"
#include "backends.h"

#include "books/line_128x7_class1.vqh"
#include "books/line_128x7_class2.vqh"

#include "books/line_128x7_0sub0.vqh"
#include "books/line_128x7_1sub1.vqh"
#include "books/line_128x7_1sub2.vqh"
#include "books/line_128x7_1sub3.vqh"
#include "books/line_128x7_2sub1.vqh"
#include "books/line_128x7_2sub2.vqh"
#include "books/line_128x7_2sub3.vqh"

#include "books/line_1024x31_class0.vqh"
#include "books/line_1024x31_class1.vqh"
#include "books/line_1024x31_class2.vqh"
#include "books/line_1024x31_class3.vqh"

#include "books/line_1024x31_0sub0.vqh"
#include "books/line_1024x31_0sub1.vqh"
#include "books/line_1024x31_1sub0.vqh"
#include "books/line_1024x31_1sub1.vqh"
#include "books/line_1024x31_2sub1.vqh"
#include "books/line_1024x31_2sub2.vqh"
#include "books/line_1024x31_2sub3.vqh"
#include "books/line_1024x31_3sub1.vqh"
#include "books/line_1024x31_3sub2.vqh"
#include "books/line_1024x31_3sub3.vqh"

#include "books/res_44_B_128aux.vqh"
#include "books/res_44_B_1024aux.vqh"

#include "books/res_B_1.vqh"
#include "books/res_B_2.vqh"
#include "books/res_B_3.vqh"
#include "books/res_B_4.vqh"
#include "books/res_B_5.vqh"
#include "books/res_B_5a.vqh"
#include "books/res_B_6.vqh"
#include "books/res_B_6a.vqh"
#include "books/res_B_6b.vqh"

#include "maskadj_A.h"

static vorbis_info_psy_global _psy_set_44_BG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static struct vp_couple_pass _psy_pass_44_B[]={
  {1.f,1.f, {0}}
};

static vorbis_info_psy _psy_set_44_B0={
  ATH_Bark_dB_lineconservative,
  -110.,-140.,

  /* tonemaskp */
  -3.f, -30.f,&_vp_tonemask_consbass_A,
  /* peakattp, curvelimitp */
  1, 30, &_vp_peakatt_D,

  /*noisemaskp */
  1,-30.f,     /* suppress any noise curve over maxspec+n */
  1.f, 1.f,   /* low/high window */
  2, 2, -1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-30,-30,-30,-30,-30,-30,-20,-10, -6, -6, -6, -6,  0,  0,  0,  0,  0},
  {.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.5f,.5f,.6f,.7f},

  105.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,
  _psy_pass_44_B
};

static vorbis_info_psy _psy_set_44_BT={
  ATH_Bark_dB_lineconservative,
  -110.f,-140.f,

  /* tonemask */
  -3.f,-30.f,&_vp_tonemask_consbass_A,
  /* peakattp,curvelimitp */
  1, 30,  &_vp_peakatt_D,

  /*noisemaskp */
  1,  -30.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,100,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-30,-30,-30,-30,-30,-30,-30,-20, -6, -6, -6, -6,  0,   0,  0,  0,  0},
  {.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.5f,.6f,.7f,.7f},

  105.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_pass_44_B
};

static vorbis_info_psy _psy_set_44_B={
  ATH_Bark_dB_lineconservative,
  -110.f,  -140.f,

  /* tonemask */
  -3.f,-30.f,&_vp_tonemask_A,
  /* peakattp, curvelimitp */
  1, 30, &_vp_peakatt_D,

  /*noisemaskp */
  1,  -30.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,100,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-30,-30,-30,-30,-30,-30,-30,-20, -6, -6, -6, -6,  0,   0,  0,  0,  0},
  {.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.4f,.5f,.6f,.7f,.7f},

  105.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_pass_44_B
};

static vorbis_info_time0 _time_set_44_B={0};

static vorbis_info_floor1 _floor_set_44_B0={
  3,
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
  128};

static vorbis_info_floor1 _floor_set_44_B={
  10,
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
  1024};

static vorbis_info_residue0 _residue_set_44_B0={
  0,128, 16, 7,23,
  {0,1,1,1,1,3,7},
  {25, 26, 27, 28, 29, 30, 31, 32, 33},
  {9999, 9999 ,9999, 9999, 9999, 9999, 9999},
  {.5, 1.5, 2.5, 4.5, 16.5, 84.5f},
  {0},
  {99, 99,99, 99, 99, 99},
  {3}};

static vorbis_info_residue0 _residue_set_44_B={
  0,1024, 32, 7,24,
  {0,1,1,1,1,3,7},
  {25, 26, 27, 28, 29, 30, 31, 32, 33},
  {9999, 9999 ,9999, 9999, 9999, 9999, 9999},
  {.5, 1.5, 2.5, 4.5, 16.5, 84.5f},
  {0},
  {99, 99,99, 99, 99, 99},
  {3}};

static vorbis_info_mapping0 _mapping_set_44_B0={
  1, {0,0}, {0}, {0}, {0}, {0,0}, 0,{0},{0}};
static vorbis_info_mapping0 _mapping_set_44_B={
  1, {0,0}, {0}, {1}, {1}, {1,2}, 0,{0},{0}};

static vorbis_info_mode _mode_set_44_B0={0,0,0,0};
static vorbis_info_mode _mode_set_44_B={1,0,0,1};

codec_setup_info info_44_B={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   34,   3,
  /* modes */
  {&_mode_set_44_B0,&_mode_set_44_B},
  /* maps */
  {0,0},{&_mapping_set_44_B0,&_mapping_set_44_B},
  /* times */
  {0,0},{&_time_set_44_B},
  /* floors */
  {1,1},{&_floor_set_44_B0,&_floor_set_44_B},
  /* residue */
  {0,0},{&_residue_set_44_B0,&_residue_set_44_B},
  /* books */
    
  {  
   &_huff_book_line_128x7_class1,
   &_huff_book_line_128x7_class2, /* 1 */
   
   &_huff_book_line_128x7_0sub0,  /* 2 */
   &_huff_book_line_128x7_1sub1,  /* 3 */
   &_huff_book_line_128x7_1sub2,
   &_huff_book_line_128x7_1sub3,  /* 5 */
   &_huff_book_line_128x7_2sub1,
   &_huff_book_line_128x7_2sub2,  /* 7 */
   &_huff_book_line_128x7_2sub3, 

   &_huff_book_line_1024x31_class0,
   &_huff_book_line_1024x31_class1, /* 10 */
   &_huff_book_line_1024x31_class2,
   &_huff_book_line_1024x31_class3, /* 12 */

   &_huff_book_line_1024x31_0sub0,
   &_huff_book_line_1024x31_0sub1, /* 14 */
   &_huff_book_line_1024x31_1sub0, 
   &_huff_book_line_1024x31_1sub1,
   &_huff_book_line_1024x31_2sub1,  
   &_huff_book_line_1024x31_2sub2, /* 18 */
   &_huff_book_line_1024x31_2sub3, 
   &_huff_book_line_1024x31_3sub1,
   &_huff_book_line_1024x31_3sub2,
   &_huff_book_line_1024x31_3sub3, /* 22 */

   &_huff_book_res_44_B_128aux, 
   &_huff_book_res_44_B_1024aux,

   &_vq_book_res_B_1,
   &_vq_book_res_B_2,
   &_vq_book_res_B_3,
   &_vq_book_res_B_4,
   &_vq_book_res_B_5,
   &_vq_book_res_B_5a,
   &_vq_book_res_B_6,
   &_vq_book_res_B_6a,
   &_vq_book_res_B_6b,

  },
  /* psy */
  {&_psy_set_44_B0,&_psy_set_44_BT,&_psy_set_44_B},
  &_psy_set_44_BG
};

#endif
