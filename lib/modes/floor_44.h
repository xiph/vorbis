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

 function: key floor settings for 44.1/48kHz
 last mod: $Id: floor_44.h,v 1.2 2001/12/12 09:45:55 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

#include "books/floor/line_128x7_class1.vqh"
#include "books/floor/line_128x7_class2.vqh"

#include "books/floor/line_128x7_0sub0.vqh"
#include "books/floor/line_128x7_1sub1.vqh"
#include "books/floor/line_128x7_1sub2.vqh"
#include "books/floor/line_128x7_1sub3.vqh"
#include "books/floor/line_128x7_2sub1.vqh"
#include "books/floor/line_128x7_2sub2.vqh"
#include "books/floor/line_128x7_2sub3.vqh"

#include "books/floor/line_128x9_class1.vqh"
#include "books/floor/line_128x9_class2.vqh"

#include "books/floor/line_128x9_0sub0.vqh"
#include "books/floor/line_128x9_1sub1.vqh"
#include "books/floor/line_128x9_1sub2.vqh"
#include "books/floor/line_128x9_1sub3.vqh"
#include "books/floor/line_128x9_2sub1.vqh"
#include "books/floor/line_128x9_2sub2.vqh"
#include "books/floor/line_128x9_2sub3.vqh"

#include "books/floor/line_128x19_class1.vqh"
#include "books/floor/line_128x19_class2.vqh"

#include "books/floor/line_128x19_0sub0.vqh"
#include "books/floor/line_128x19_1sub1.vqh"
#include "books/floor/line_128x19_1sub2.vqh"
#include "books/floor/line_128x19_1sub3.vqh"
#include "books/floor/line_128x19_2sub1.vqh"
#include "books/floor/line_128x19_2sub2.vqh"
#include "books/floor/line_128x19_2sub3.vqh"

#include "books/floor/line_1024x31_class0.vqh"
#include "books/floor/line_1024x31_class1.vqh"
#include "books/floor/line_1024x31_class2.vqh"
#include "books/floor/line_1024x31_class3.vqh"

#include "books/floor/line_1024x31_0sub0.vqh"
#include "books/floor/line_1024x31_0sub1.vqh"
#include "books/floor/line_1024x31_1sub0.vqh"
#include "books/floor/line_1024x31_1sub1.vqh"
#include "books/floor/line_1024x31_2sub1.vqh"
#include "books/floor/line_1024x31_2sub2.vqh"
#include "books/floor/line_1024x31_2sub3.vqh"
#include "books/floor/line_1024x31_3sub1.vqh"
#include "books/floor/line_1024x31_3sub2.vqh"
#include "books/floor/line_1024x31_3sub3.vqh"

static static_codebook *_floor_44_128x7_books[]={
  &_huff_book_line_128x7_class1,
  &_huff_book_line_128x7_class2,
  
  &_huff_book_line_128x7_0sub0,
  &_huff_book_line_128x7_1sub1,
  &_huff_book_line_128x7_1sub2,
  &_huff_book_line_128x7_1sub3,
  &_huff_book_line_128x7_2sub1,
  &_huff_book_line_128x7_2sub2,
  &_huff_book_line_128x7_2sub3, 
};
static static_codebook *_floor_44_128x9_books[]={
  &_huff_book_line_128x9_class1,
  &_huff_book_line_128x9_class2,
  
  &_huff_book_line_128x9_0sub0,
  &_huff_book_line_128x9_1sub1,
  &_huff_book_line_128x9_1sub2,
  &_huff_book_line_128x9_1sub3,
  &_huff_book_line_128x9_2sub1,
  &_huff_book_line_128x9_2sub2,
  &_huff_book_line_128x9_2sub3, 
};
static static_codebook *_floor_44_128x19_books[]={
  &_huff_book_line_128x19_class1,
  &_huff_book_line_128x19_class2,
  
  &_huff_book_line_128x19_0sub0,
  &_huff_book_line_128x19_1sub1,
  &_huff_book_line_128x19_1sub2,
  &_huff_book_line_128x19_1sub3,
  &_huff_book_line_128x19_2sub1,
  &_huff_book_line_128x19_2sub2,
  &_huff_book_line_128x19_2sub3, 
};

static static_codebook **_floor_44_128_books[3]={
  _floor_44_128x7_books,
  _floor_44_128x9_books,
  _floor_44_128x19_books,
};

static static_codebook *_floor_44_1024x31_books[]={
    &_huff_book_line_1024x31_class0,
    &_huff_book_line_1024x31_class1,
    &_huff_book_line_1024x31_class2,
    &_huff_book_line_1024x31_class3,
    
    &_huff_book_line_1024x31_0sub0,
    &_huff_book_line_1024x31_0sub1,
    &_huff_book_line_1024x31_1sub0, 
    &_huff_book_line_1024x31_1sub1,
    &_huff_book_line_1024x31_2sub1,  
    &_huff_book_line_1024x31_2sub2,
    &_huff_book_line_1024x31_2sub3, 
    &_huff_book_line_1024x31_3sub1,
    &_huff_book_line_1024x31_3sub2,
    &_huff_book_line_1024x31_3sub3,
};

static static_codebook **_floor_44_1024_books[1]={
  _floor_44_1024x31_books
};

static vorbis_info_floor1 _floor_44_128[3]={
  {
    3,{0,1,2},{1,3,3},{0,2,2},{-1,0,1},
    {{2},{-1,3,4,5},{-1,6,7,8}},
    4,{0,128, 7, 2,1,4, 23,13,45},
    
    60,30,500,
    999,999,0,18.,
    8,70,
    -1 /* lowpass! */
  },

  {
    3,{0,1,2},{1,4,4},{0,2,2},{-1,0,1},
    {{2},{-1,3,4,5},{-1,6,7,8}},
    4,{0,128, 13, 4,2,7,1,  44,30,62,20},
    
    60,30,500,
    999,999,0,18.,
    8,70,
    -1 /* lowpass! */
  },


  {
    6,{0,1,1,1,2,2},{4,3,3},{0,2,2},{-1,0,1},
    {{2},{-1,3,4,5},{-1,6,7,8}},
    2,{0,128, 6,17,30,58, 2,1,4, 11,8,14, 23,20,26, 41,35,48, 84,69,103},
    
    60,30,500,
    999,999,1,18.,
    8,70,
    -1 /* lowpass */
  }
};

static vorbis_info_floor1 _floor_44_1024[1]={
  {
    10,{0,1,2,2,2,2,2, 3,3,3},{3,4,3,3},{1,1,2,2},{0,1,2,3},
    {{4,5},{6,7},{-1,8,9,10},{-1,11,12,13}},
    2,{0,1024, 88,31,243, 14,54,143,460, 6,3,10, 22,18,26, 41,36,47, 
       69,61,78, 112,99,126, 185,162,211, 329,282,387, 672,553,825},
  
    60,30,400,
    20,8,1,18.,
    20,600,
    -1 /* lowpass */
  }
};

