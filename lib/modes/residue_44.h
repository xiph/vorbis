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

 function: toplevel residue templates for 32/44.1/48kHz
 last mod: $Id: residue_44.h,v 1.1.2.2 2001/12/05 08:03:20 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

static bitrate_manager_info _bm_44_default={
  /* progressive coding and bitrate controls */
  2.,.5,
  2.,       0,           0,  
            0,           0,
           
  4.0, 0.,  -1.,              .05, 
            -.05,             .05,
  3.5,5.0,
  -10.f,+2.f
};

/***** residue backends *********************************************/

/* the books used depend on stereo-or-not, but the residue setup below
   can apply to coupled or not.  These templates are for a first pass;
   a last pass is mechanically added in vorbisenc for residue backfill
   at 1/3 and 1/9, as well as an optional middle pass for stereo
   backfill */

/*     0   1   2   4  22   1   2   4  22   +      
           0   0   0   0         

       0   1   2   3   4   5   6   7   8   9
   1                   .               .   .
   2                   .               .   .
   4       .   .   .       .   .   .       .
 
       0   4   4   4   3   4   4   4   3   7 */
static vorbis_info_residue0 _residue_44_low={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {   0,    4,    4,    4,    3,    4,    4,    4,    3,    7},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  2.5,  4.5, 22.5,  1.5,  2.5,  4.5, 22.5},
  {0},
  {  99,   -1,   -1,   -1,   -1,   99,   99,   99,   99}
};

/*     0   1   2   4   1   2   4  22  84   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1                               .   .   .
   2                               .   .   .
   4       .   .   .   .   .   .           .
 
       0   4   4   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_mid={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {   0,    4,    4,    4,    4,    4,    4,    3,    3,    7},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  2.5,  4.5,  1.5,  2.5,  4.5, 22.5, 84.5},
  {0},
  {  99,   -1,   -1,   -1,   99,   99,   99,   99,   99}
};


/*     0   4  22   1   2   4   7  22  84   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1           .                   .   .   .
   2           .                   .   .   .
   4       .       .   .   .   .           .
 
       0   4   3   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_high={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {   0,    4,    3,    4,    4,    4,    4,    3,    3,    7},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  4.5, 22.5,  1.5,  2.5,  4.5,  7.5, 22.5, 84.5},
  {0},
  {  99,   -1,   -1,   99,   99,   99,   99,   99,   99}
};

/* residue backfill is entered in the template array as if stereo
   backfill is not in use.  It's up to vorbisenc to make the
   appropriate index adjustment */
#if 0
static vorbis_residue_template _residue_template_44_stereo[11]={
  /* mode 0; 64-ish */
  {&_residue_44_low,  {&44c0_short,&44c0_long},
   { {{-1}}, /* lossless stereo */
     {{-1}}, /* 6dB (2.5) stereo */
     {{-1}}, /* 12dB (4.5) stereo */
     {{-1}}, /* 17dB (7.5) stereo */
     {{&44c0_s0_p1_0},{&44c0_s0_p2_0},{&44c0_s0_p3_0},{&44c0_s0_p4_0,&44c0_p4_1},
      {&44c0_s1_p5_0},{&44c0_s1_p6_0},{&44c0_s2_p7_0},{&44c0_s4_p8_0,&44c0_s4_p8_1},
      {&44c0_s4_p9_0,&44c0_s4_p9_1,&44c0_s4_p9_2}}, /* 22dB (12.5) stereo */
     {{-1}}, /* 27dB (22.5) stereo */
   },
   { {-1}, /* lossless stereo */
     {-1}, /* 6dB (2.5) stereo */
     {-1}, /* 12dB (4.5) stereo */
     {-1}, /* 17dB (7.5) stereo */
     {-1,-1,-1,-1,-1,-1,-1,-1,&44c0_s4_s8,&44c0_s4_s9},/* 22dB (12.5) stereo */
     {-1}, /* 27dB (22.5) stereo */
   },
   { {{-1}}, /* lossless stereo */
     {{-1}}, /* 6dB (2.5) stereo */
     {{-1}}, /* 12dB (4.5) stereo */
     {{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
      {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
      {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
      {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
      {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}}, /* 17dB (7.5) stereo */
     {{-1}}, /* 22dB (12.5) stereo */
     {{-1}}, /* 27dB (22.5) stereo */
   },
  }

  /* mode 1; 80-ish */

};
#endif

#include "books/res_44c_Z_128aux.vqh"
#include "books/res_44c_Z_1024aux.vqh"

static vorbis_residue_template _residue_template_44_stereo_temp[11]={
  /* mode 0; 64-ish */
  {&_residue_44_low,  {&_huff_book_res_44c_Z_128aux, 
		      &_huff_book_res_44c_Z_1024aux},
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 17dB (7.5) stereo */
     {{0}},
     {{0}}, /* 27dB (22.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 17dB (7.5) stereo */
     {0},
     {0}, /* 27dB (22.5) stereo */
   },
   { {{0}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}},
     {{0}}, /* 22dB (12.5) stereo */
     {{0}}, /* 27dB (22.5) stereo */
   },
  }

  /* mode 1; 80-ish */

};
