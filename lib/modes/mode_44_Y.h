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
 last mod: $Id: mode_44_Y.h,v 1.1 2001/08/13 11:30:59 xiphmont Exp $

 ********************************************************************/

#ifndef _V_MODES_44_Y_H_
#define _V_MODES_44_Y_H_

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

#include "books/res_44_Y_128aux.vqh"
#include "books/res_44_Y_1024aux.vqh"

#include "books/res_Z_1.vqh"
#include "books/res_Z_2.vqh"
#include "books/res_Z_3.vqh"
#include "books/res_Z_4.vqh"
#include "books/res_Z_5.vqh"
#include "books/res_Z_6.vqh"
#include "books/res_Z_6a.vqh"
#include "books/res_Z_6b.vqh"

#include "maskadj_Z.h"

static vorbis_info_psy_global _psy_set_44_YG={
  0, /* decaydBpms */
  8,   /* lines per eighth octave */
  
  /* thresh sample period, preecho clamp trigger threshhold, range, minenergy */
  256, {26.f,26.f,26.f,30.f}, {-90.f,-90.f,-90.f,-90.f}, -90.f,
  -6.f, 
  
  0,
};

static struct vp_couple_pass _psy_pass_44_Y[]={
  {1.f,1.f, {0}}
};

static vorbis_info_psy _psy_set_44_Y0={
  ATH_Bark_dB_lineaggressive,
  -100.,-110.,

  /* tonemaskp */
  0.f, -24.f,&_vp_tonemask_consbass_Z,
  /* peakattp, curvelimitp */
  1, 0, &_vp_peakatt_Z,

  /*noisemaskp */
  1,-24.f,     /* suppress any noise curve over maxspec+n */
  1.f, 1.f,   /* low/high window */
  2, 2, -1,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-10, -5,  0,  0,  1,  1,  2,  2,  2,  2,  4},
  {1.f,1.f,1.f,1.f,1.f,1.f,.8f,.7f,.7f,.7f,.7f,.7f,.8f,.88f,.89f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,
  _psy_pass_44_Y
};

static vorbis_info_psy _psy_set_44_YT={
  ATH_Bark_dB_lineaggressive,
  -100.f,-110.f,

  /* tonemask */
  0.f,-20.f,&_vp_tonemask_consbass_Z,
  /* peakattp,curvelimitp */
  1, 0,  &_vp_peakatt_Z,

  /*noisemaskp */
  1,  -24.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,100,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-20, -6,  2,  2,  2,  2,  3,   3,  3,  4, 6},
  {1.f,1.f,1.f,1.f,1.f,1.f,.8f,.7f,.7f,.7f,.7f,.7f,.8f,.88f,.89f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_pass_44_Y
};

static vorbis_info_psy _psy_set_44_Y={
  ATH_Bark_dB_lineaggressive,
  -100.f,  -110.f,

  /* tonemask */
  0.f,-20.f,&_vp_tonemask_Z,
  /* peakattp, curvelimitp */
  1, 0, &_vp_peakatt_Z,

  /*noisemaskp */
  1,  -24.f,     /* suppress any noise curve over maxspec+n */
      .5f,.5f,   /* low/high window */
      10,10,100,

  /*63     125     250     500      1k      2k      4k       8k     16k*/
  {-20,-20,-20,-20,-20,-20,-10,-6,  0,  0,  1,  1,  2,   2,  2,  2, 6},
  {1.f,1.f,1.f,1.f,1.f,1.f,.8f,.7f,.7f,.7f,.7f,.8f,.85f,.88f,.89f,.9f,.9f},

  95.f,  /* even decade + 5 is important; saves an rint() later in a
            tight loop) */
  1,_psy_pass_44_Y
};

static vorbis_info_time0 _time_set_44_Y={0};

static vorbis_info_floor1 _floor_set_44_Y0={
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
  96};

static vorbis_info_floor1 _floor_set_44_Y={
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
  768};

static vorbis_info_residue0 _residue_set_44_Y0={
  0,96, 12, 7,23,
  {0,1,1,1,1,1,7},
  {25, 26, 27, 28, 29, 30, 31, 32},
  {9999, 3,9999, 9999, 9999, 9999, 9999},
  {.5, 1.5f,1.5, 2.5, 7.5, 22.5f},
  {0},
  {99, 99,99, 99, 99, 99},
  {3}};

static vorbis_info_residue0 _residue_set_44_Y={
  0,768, 32, 7,24,
  {0,1,1,1,1,1,7},
  {25, 26, 27, 28, 29, 30, 31, 32},
  {9999, 4,9999, 9999, 9999, 9999, 9999},
  {.5, 1.5f,1.5, 2.5, 7.5, 22.5f},
  {0},
  {99, 99,99, 99, 99, 99},
  {3}};

static vorbis_info_mapping0 _mapping_set_44_Y0={
  1, {0,0}, {0}, {0}, {0}, {0,0}, 0,{0},{0}};
static vorbis_info_mapping0 _mapping_set_44_Y={
  1, {0,0}, {0}, {1}, {1}, {1,2}, 0,{0},{0}};

static vorbis_info_mode _mode_set_44_Y0={0,0,0,0};
static vorbis_info_mode _mode_set_44_Y={1,0,0,1};

codec_setup_info info_44_Y={

  /* smallblock, largeblock */
  {256, 2048}, 
  /* modes,maps,times,floors,residues,books,psys */
  2,          2,    1,     2,       2,   33,   3,
  /* modes */
  {&_mode_set_44_Y0,&_mode_set_44_Y},
  /* maps */
  {0,0},{&_mapping_set_44_Y0,&_mapping_set_44_Y},
  /* times */
  {0,0},{&_time_set_44_Y},
  /* floors */
  {1,1},{&_floor_set_44_Y0,&_floor_set_44_Y},
  /* residue */
  {0,0},{&_residue_set_44_Y0,&_residue_set_44_Y},
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

   &_huff_book_res_44_Y_128aux, 
   &_huff_book_res_44_Y_1024aux,

   &_vq_book_res_Z_1,
   &_vq_book_res_Z_2,
   &_vq_book_res_Z_3,
   &_vq_book_res_Z_4,
   &_vq_book_res_Z_5,
   &_vq_book_res_Z_6,
   &_vq_book_res_Z_6a,
   &_vq_book_res_Z_6b,

  },
  /* psy */
  {&_psy_set_44_Y0,&_psy_set_44_YT,&_psy_set_44_Y},
  &_psy_set_44_YG
};

#endif
