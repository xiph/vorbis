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
 last mod: $Id: residue_44.h,v 1.11 2001/12/23 11:53:54 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

static bitrate_manager_info _bm_44_default={
  /* progressive coding and bitrate controls */
  4.,.0,
  2.,       0,           0,  
            0,           0,
           
  4.0,2.3, -9999,              .02, 

  3.2,5.0,
  0.,0.
};

/***** residue backends *********************************************/

/* the books used depend on stereo-or-not, but the residue setup below
   can apply to coupled or not.  These templates are for a first pass;
   a last pass is mechanically added in vorbisenc for residue backfill
   at 1/3 and 1/9, as well as an optional middle pass for stereo
   backfill */

/*     0   1   2   4  26   1   2   4  26   +      
           0   0   0   0         

       0   1   2   3   4   5   6   7   8   9
   1                   .               .   .
   2                   .               .   .
   4       .   .   .       .   .   .       .
 
       0   4   4   4   3   4   4   4   3   7 */
static vorbis_info_residue0 _residue_44_low={
  0,-1, -1, 8,-1,
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  2.5,  4.5, 26.5,  1.5,  4.5},
  {0},
  {  99,   -1,   -1,   -1,   -1,   99,   99}
};
/* 26 doesn't cascade well; use 28 instead */
static vorbis_info_residue0 _residue_44_low_un={
  0,-1, -1, 8,-1,
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  2.5,  4.5, 28.5,  1.5,  4.5},
  {0},
  {  99,   -1,   -1,   -1,   -1,   99,   99}
};

/*     0   1   2   4   1   2   4  16  42   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1                               .   .   .
   2                               .   .   .
   4       .   .   .   .   .   .           .
 
       0   4   4   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_mid={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  4.5, 16.5, 42.5},
  {0},
  {  99,   -1,   99,   -1,   99,   -1,   99,   99,   99}
};


/*     0   8  42   1   2   4   8  16  56   +      
           0   0   0            

       0   1   2   3   4   5   6   7   8   9
   1           .                   .   .   .
   2           .                   .   .   .
   4       .       .   .   .   .           .
 
       0   4   3   4   4   4   4   3   3   7 */
static vorbis_info_residue0 _residue_44_high={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  8.5, 42.5,  1.5,  2.5,  4.5,  8.5, 16.5, 56.5},
  {0},
  {  99,   -1,   -1,   99,   99,   99,   99,   99,   99}
};
/* 56 doesn't cascade well; use 59 */
static vorbis_info_residue0 _residue_44_high_un={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8     9 */
  {0},
  {-1},
  {9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999, 9999},
  {  .5,  8.5, 42.5,  1.5,  2.5,  4.5,  8.5, 16.5, 59.5},
  {0},
  {  99,   -1,   -1,   99,   99,   99,   99,   99,   99}
};

#include "books/coupled/_44c0_short.vqh"
#include "books/coupled/_44c0_long.vqh"

#include "books/coupled/_44c0_s0_p1_0.vqh"
#include "books/coupled/_44c0_s0_p2_0.vqh"
#include "books/coupled/_44c0_s0_p3_0.vqh"
#include "books/coupled/_44c0_s0_p4_0.vqh"
#include "books/coupled/_44c0_s0_p4_1.vqh"
#include "books/coupled/_44c0_s0_p5_0.vqh"
#include "books/coupled/_44c0_s1_p5_0.vqh"
#include "books/coupled/_44c0_s0_p6_0.vqh"
#include "books/coupled/_44c0_s1_p6_0.vqh"
#include "books/coupled/_44c0_s2_p6_0.vqh"
#include "books/coupled/_44c0_s0_p7_0.vqh"
#include "books/coupled/_44c0_s0_p7_1.vqh"
#include "books/coupled/_44c0_s0_p7_2.vqh"
#include "books/coupled/_44c0_s1_p7_0.vqh"
#include "books/coupled/_44c0_s1_p7_1.vqh"
#include "books/coupled/_44c0_s1_p7_2.vqh"
#include "books/coupled/_44c0_s2_p7_0.vqh"
#include "books/coupled/_44c0_s2_p7_1.vqh"
#include "books/coupled/_44c0_s2_p7_2.vqh"
#include "books/coupled/_44c0_s3_p7_0.vqh"
#include "books/coupled/_44c0_s3_p7_1.vqh"
#include "books/coupled/_44c0_s3_p7_2.vqh"

#include "books/coupled/_44c0_s1_p5_s0.vqh"
#include "books/coupled/_44c0_s1_p6_s0.vqh"
#include "books/coupled/_44c0_s1_p7_s0.vqh"
#include "books/coupled/_44c0_s2_p6_s0.vqh"
#include "books/coupled/_44c0_s2_p7_s0.vqh"
#include "books/coupled/_44c0_s3_p7_s0.vqh"

#include "books/coupled/_44c0_s0_p0_r0.vqh"
#include "books/coupled/_44c0_s0_pN_r0.vqh"
#include "books/coupled/_44c0_s1_pS_r0.vqh"
#include "books/coupled/_44c0_s0_p0_r1.vqh"
#include "books/coupled/_44c0_s0_pN_r1.vqh"
#include "books/coupled/_44c0_s1_pS_r1.vqh"

#include "books/coupled/_44c1_short.vqh"
#include "books/coupled/_44c1_long.vqh"

#include "books/coupled/_44c1_s0_p1_0.vqh"
#include "books/coupled/_44c1_s0_p2_0.vqh"
#include "books/coupled/_44c1_s0_p3_0.vqh"
#include "books/coupled/_44c1_s0_p4_0.vqh"
#include "books/coupled/_44c1_s0_p4_1.vqh"
#include "books/coupled/_44c1_s0_p5_0.vqh"
#include "books/coupled/_44c1_s1_p5_0.vqh"
#include "books/coupled/_44c1_s0_p6_0.vqh"
#include "books/coupled/_44c1_s1_p6_0.vqh"
#include "books/coupled/_44c1_s2_p6_0.vqh"
#include "books/coupled/_44c1_s0_p7_0.vqh"
#include "books/coupled/_44c1_s0_p7_1.vqh"
#include "books/coupled/_44c1_s0_p7_2.vqh"
#include "books/coupled/_44c1_s1_p7_0.vqh"
#include "books/coupled/_44c1_s1_p7_1.vqh"
#include "books/coupled/_44c1_s1_p7_2.vqh"
#include "books/coupled/_44c1_s2_p7_0.vqh"
#include "books/coupled/_44c1_s2_p7_1.vqh"
#include "books/coupled/_44c1_s2_p7_2.vqh"
#include "books/coupled/_44c1_s3_p7_0.vqh"
#include "books/coupled/_44c1_s3_p7_1.vqh"
#include "books/coupled/_44c1_s3_p7_2.vqh"

#include "books/coupled/_44c2_short.vqh"
#include "books/coupled/_44c2_long.vqh"

#include "books/coupled/_44c2_s0_p1_0.vqh"
#include "books/coupled/_44c2_s0_p2_0.vqh"
#include "books/coupled/_44c2_s0_p3_0.vqh"
#include "books/coupled/_44c2_s0_p4_0.vqh"
#include "books/coupled/_44c2_s0_p4_1.vqh"
#include "books/coupled/_44c2_s0_p5_0.vqh"
#include "books/coupled/_44c2_s1_p5_0.vqh"
#include "books/coupled/_44c2_s0_p6_0.vqh"
#include "books/coupled/_44c2_s1_p6_0.vqh"
#include "books/coupled/_44c2_s2_p6_0.vqh"
#include "books/coupled/_44c2_s0_p7_0.vqh"
#include "books/coupled/_44c2_s0_p7_1.vqh"
#include "books/coupled/_44c2_s0_p7_2.vqh"
#include "books/coupled/_44c2_s1_p7_0.vqh"
#include "books/coupled/_44c2_s1_p7_1.vqh"
#include "books/coupled/_44c2_s1_p7_2.vqh"
#include "books/coupled/_44c2_s2_p7_0.vqh"
#include "books/coupled/_44c2_s2_p7_1.vqh"
#include "books/coupled/_44c2_s2_p7_2.vqh"
#include "books/coupled/_44c2_s3_p7_0.vqh"
#include "books/coupled/_44c2_s3_p7_1.vqh"
#include "books/coupled/_44c2_s3_p7_2.vqh"


#include "books/coupled/_44c3_short.vqh"
#include "books/coupled/_44c3_long.vqh"

#include "books/coupled/_44c3_s0_p1_0.vqh"
#include "books/coupled/_44c3_s0_p2_0.vqh"
#include "books/coupled/_44c3_s0_p3_0.vqh"
#include "books/coupled/_44c3_s0_p4_0.vqh"
#include "books/coupled/_44c3_s0_p4_1.vqh"
#include "books/coupled/_44c3_s0_p5_0.vqh"
#include "books/coupled/_44c3_s1_p5_0.vqh"
#include "books/coupled/_44c3_s0_p6_0.vqh"
#include "books/coupled/_44c3_s1_p6_0.vqh"
#include "books/coupled/_44c3_s2_p6_0.vqh"
#include "books/coupled/_44c3_s0_p7_0.vqh"
#include "books/coupled/_44c3_s0_p7_1.vqh"
#include "books/coupled/_44c3_s0_p7_2.vqh"
#include "books/coupled/_44c3_s1_p7_0.vqh"
#include "books/coupled/_44c3_s1_p7_1.vqh"
#include "books/coupled/_44c3_s1_p7_2.vqh"
#include "books/coupled/_44c3_s2_p7_0.vqh"
#include "books/coupled/_44c3_s2_p7_1.vqh"
#include "books/coupled/_44c3_s2_p7_2.vqh"
#include "books/coupled/_44c3_s3_p7_0.vqh"
#include "books/coupled/_44c3_s3_p7_1.vqh"
#include "books/coupled/_44c3_s3_p7_2.vqh"

#include "books/coupled/_44c4_short.vqh"
#include "books/coupled/_44c4_long.vqh"

#include "books/coupled/_44c4_s0_p1_0.vqh"
#include "books/coupled/_44c4_s0_p2_0.vqh"
#include "books/coupled/_44c4_s1_p2_0.vqh"
#include "books/coupled/_44c4_s0_p3_0.vqh"
#include "books/coupled/_44c4_s0_p4_0.vqh"
#include "books/coupled/_44c4_s1_p4_0.vqh"
#include "books/coupled/_44c4_s0_p5_0.vqh"
#include "books/coupled/_44c4_s0_p6_0.vqh"
#include "books/coupled/_44c4_s1_p6_0.vqh"
#include "books/coupled/_44c4_s2_p6_0.vqh"
#include "books/coupled/_44c4_s0_p7_0.vqh"
#include "books/coupled/_44c4_s0_p7_1.vqh"
#include "books/coupled/_44c4_s1_p7_0.vqh"
#include "books/coupled/_44c4_s1_p7_1.vqh"
#include "books/coupled/_44c4_s2_p7_0.vqh"
#include "books/coupled/_44c4_s2_p7_1.vqh"
#include "books/coupled/_44c4_s3_p7_0.vqh"
#include "books/coupled/_44c4_s3_p7_1.vqh"
#include "books/coupled/_44c4_s0_p8_0.vqh"
#include "books/coupled/_44c4_s0_p8_1.vqh"
#include "books/coupled/_44c4_s1_p8_0.vqh"
#include "books/coupled/_44c4_s1_p8_1.vqh"
#include "books/coupled/_44c4_s2_p8_0.vqh"
#include "books/coupled/_44c4_s2_p8_1.vqh"
#include "books/coupled/_44c4_s3_p8_0.vqh"
#include "books/coupled/_44c4_s3_p8_1.vqh"
#include "books/coupled/_44c4_s0_p9_0.vqh"
#include "books/coupled/_44c4_s0_p9_1.vqh"
#include "books/coupled/_44c4_s0_p9_2.vqh"
#include "books/coupled/_44c4_s1_p9_0.vqh"
#include "books/coupled/_44c4_s1_p9_1.vqh"
#include "books/coupled/_44c4_s1_p9_2.vqh"
#include "books/coupled/_44c4_s2_p9_0.vqh"
#include "books/coupled/_44c4_s2_p9_1.vqh"
#include "books/coupled/_44c4_s2_p9_2.vqh"
#include "books/coupled/_44c4_s3_p9_0.vqh"
#include "books/coupled/_44c4_s3_p9_1.vqh"
#include "books/coupled/_44c4_s3_p9_2.vqh"

#include "books/coupled/_44c4_s1_p2_s0.vqh"
#include "books/coupled/_44c4_s1_p4_s0.vqh"
#include "books/coupled/_44c4_s1_p6_s0.vqh"
#include "books/coupled/_44c4_s1_p7_s0.vqh"
#include "books/coupled/_44c4_s1_p8_s0.vqh"
#include "books/coupled/_44c4_s1_p9_s0.vqh"

#include "books/coupled/_44c4_s2_p6_s0.vqh"
#include "books/coupled/_44c4_s2_p7_s0.vqh"
#include "books/coupled/_44c4_s2_p8_s0.vqh"
#include "books/coupled/_44c4_s2_p9_s0.vqh"

#include "books/coupled/_44c4_s3_p9_s0.vqh"

#include "books/coupled/_44c4_s0_p0_r0.vqh"
#include "books/coupled/_44c4_s0_p0_r1.vqh"
#include "books/coupled/_44c4_s0_pN_r0.vqh"
#include "books/coupled/_44c4_s0_pN_r1.vqh"
#include "books/coupled/_44c4_s1_pS_r0.vqh"
#include "books/coupled/_44c4_s1_pS_r1.vqh"

#include "books/coupled/_44c5_short.vqh"
#include "books/coupled/_44c5_long.vqh"

#include "books/coupled/_44c5_s0_p1_0.vqh"
#include "books/coupled/_44c5_s0_p2_0.vqh"
#include "books/coupled/_44c5_s1_p2_0.vqh"
#include "books/coupled/_44c5_s0_p3_0.vqh"
#include "books/coupled/_44c5_s0_p4_0.vqh"
#include "books/coupled/_44c5_s1_p4_0.vqh"
#include "books/coupled/_44c5_s0_p5_0.vqh"
#include "books/coupled/_44c5_s0_p6_0.vqh"
#include "books/coupled/_44c5_s1_p6_0.vqh"
#include "books/coupled/_44c5_s2_p6_0.vqh"
#include "books/coupled/_44c5_s0_p7_0.vqh"
#include "books/coupled/_44c5_s0_p7_1.vqh"
#include "books/coupled/_44c5_s1_p7_0.vqh"
#include "books/coupled/_44c5_s1_p7_1.vqh"
#include "books/coupled/_44c5_s2_p7_0.vqh"
#include "books/coupled/_44c5_s2_p7_1.vqh"
#include "books/coupled/_44c5_s3_p7_0.vqh"
#include "books/coupled/_44c5_s3_p7_1.vqh"
#include "books/coupled/_44c5_s0_p8_0.vqh"
#include "books/coupled/_44c5_s0_p8_1.vqh"
#include "books/coupled/_44c5_s1_p8_0.vqh"
#include "books/coupled/_44c5_s1_p8_1.vqh"
#include "books/coupled/_44c5_s2_p8_0.vqh"
#include "books/coupled/_44c5_s2_p8_1.vqh"
#include "books/coupled/_44c5_s3_p8_0.vqh"
#include "books/coupled/_44c5_s3_p8_1.vqh"
#include "books/coupled/_44c5_s0_p9_0.vqh"
#include "books/coupled/_44c5_s0_p9_1.vqh"
#include "books/coupled/_44c5_s0_p9_2.vqh"
#include "books/coupled/_44c5_s1_p9_0.vqh"
#include "books/coupled/_44c5_s1_p9_1.vqh"
#include "books/coupled/_44c5_s1_p9_2.vqh"
#include "books/coupled/_44c5_s2_p9_0.vqh"
#include "books/coupled/_44c5_s2_p9_1.vqh"
#include "books/coupled/_44c5_s2_p9_2.vqh"
#include "books/coupled/_44c5_s3_p9_0.vqh"
#include "books/coupled/_44c5_s3_p9_1.vqh"
#include "books/coupled/_44c5_s3_p9_2.vqh"

#include "books/coupled/_44c6_short.vqh"
#include "books/coupled/_44c6_long.vqh"

#include "books/coupled/_44c6_s0_p1_0.vqh"
#include "books/coupled/_44c6_s0_p2_0.vqh"
#include "books/coupled/_44c6_s1_p2_0.vqh"
#include "books/coupled/_44c6_s0_p3_0.vqh"
#include "books/coupled/_44c6_s0_p4_0.vqh"
#include "books/coupled/_44c6_s1_p4_0.vqh"
#include "books/coupled/_44c6_s0_p5_0.vqh"
#include "books/coupled/_44c6_s0_p6_0.vqh"
#include "books/coupled/_44c6_s1_p6_0.vqh"
#include "books/coupled/_44c6_s2_p6_0.vqh"
#include "books/coupled/_44c6_s0_p7_0.vqh"
#include "books/coupled/_44c6_s0_p7_1.vqh"
#include "books/coupled/_44c6_s1_p7_0.vqh"
#include "books/coupled/_44c6_s1_p7_1.vqh"
#include "books/coupled/_44c6_s2_p7_0.vqh"
#include "books/coupled/_44c6_s2_p7_1.vqh"
#include "books/coupled/_44c6_s3_p7_0.vqh"
#include "books/coupled/_44c6_s3_p7_1.vqh"
#include "books/coupled/_44c6_s0_p8_0.vqh"
#include "books/coupled/_44c6_s0_p8_1.vqh"
#include "books/coupled/_44c6_s1_p8_0.vqh"
#include "books/coupled/_44c6_s1_p8_1.vqh"
#include "books/coupled/_44c6_s2_p8_0.vqh"
#include "books/coupled/_44c6_s2_p8_1.vqh"
#include "books/coupled/_44c6_s3_p8_0.vqh"
#include "books/coupled/_44c6_s3_p8_1.vqh"
#include "books/coupled/_44c6_s0_p9_0.vqh"
#include "books/coupled/_44c6_s0_p9_1.vqh"
#include "books/coupled/_44c6_s0_p9_2.vqh"
#include "books/coupled/_44c6_s1_p9_0.vqh"
#include "books/coupled/_44c6_s1_p9_1.vqh"
#include "books/coupled/_44c6_s1_p9_2.vqh"
#include "books/coupled/_44c6_s2_p9_0.vqh"
#include "books/coupled/_44c6_s2_p9_1.vqh"
#include "books/coupled/_44c6_s2_p9_2.vqh"
#include "books/coupled/_44c6_s3_p9_0.vqh"
#include "books/coupled/_44c6_s3_p9_1.vqh"
#include "books/coupled/_44c6_s3_p9_2.vqh"

#include "books/coupled/_44c7_short.vqh"
#include "books/coupled/_44c7_long.vqh"

#include "books/coupled/_44c7_s0_p1_0.vqh"
#include "books/coupled/_44c7_s0_p1_1.vqh"
#include "books/coupled/_44c7_s0_p2_0.vqh"
#include "books/coupled/_44c7_s0_p2_1.vqh"
#include "books/coupled/_44c7_s0_p3_0.vqh"
#include "books/coupled/_44c7_s0_p4_0.vqh"
#include "books/coupled/_44c7_s0_p5_0.vqh"
#include "books/coupled/_44c7_s0_p6_0.vqh"
#include "books/coupled/_44c7_s0_p6_1.vqh"
#include "books/coupled/_44c7_s0_p7_0.vqh"
#include "books/coupled/_44c7_s0_p7_1.vqh"
#include "books/coupled/_44c7_s0_p8_0.vqh"
#include "books/coupled/_44c7_s0_p8_1.vqh"
#include "books/coupled/_44c7_s0_p9_0.vqh"
#include "books/coupled/_44c7_s0_p9_1.vqh"
#include "books/coupled/_44c7_s0_p9_2.vqh"

#include "books/coupled/_44c7_s0_p0_r0.vqh"
#include "books/coupled/_44c7_s0_p0_r1.vqh"
#include "books/coupled/_44c7_s0_pN_r0.vqh"
#include "books/coupled/_44c7_s0_pN_r1.vqh"

#include "books/coupled/_44c8_short.vqh"
#include "books/coupled/_44c8_long.vqh"

#include "books/coupled/_44c8_s0_p1_0.vqh"
#include "books/coupled/_44c8_s0_p1_1.vqh"
#include "books/coupled/_44c8_s0_p2_0.vqh"
#include "books/coupled/_44c8_s0_p2_1.vqh"
#include "books/coupled/_44c8_s0_p3_0.vqh"
#include "books/coupled/_44c8_s0_p4_0.vqh"
#include "books/coupled/_44c8_s0_p5_0.vqh"
#include "books/coupled/_44c8_s0_p6_0.vqh"
#include "books/coupled/_44c8_s0_p6_1.vqh"
#include "books/coupled/_44c8_s0_p7_0.vqh"
#include "books/coupled/_44c8_s0_p7_1.vqh"
#include "books/coupled/_44c8_s0_p8_0.vqh"
#include "books/coupled/_44c8_s0_p8_1.vqh"
#include "books/coupled/_44c8_s0_p9_0.vqh"
#include "books/coupled/_44c8_s0_p9_1.vqh"
#include "books/coupled/_44c8_s0_p9_2.vqh"

#include "books/coupled/_44c9_short.vqh"
#include "books/coupled/_44c9_long.vqh"

#include "books/coupled/_44c9_s0_p1_0.vqh"
#include "books/coupled/_44c9_s0_p1_1.vqh"
#include "books/coupled/_44c9_s0_p2_0.vqh"
#include "books/coupled/_44c9_s0_p2_1.vqh"
#include "books/coupled/_44c9_s0_p3_0.vqh"
#include "books/coupled/_44c9_s0_p4_0.vqh"
#include "books/coupled/_44c9_s0_p5_0.vqh"
#include "books/coupled/_44c9_s0_p6_0.vqh"
#include "books/coupled/_44c9_s0_p6_1.vqh"
#include "books/coupled/_44c9_s0_p7_0.vqh"
#include "books/coupled/_44c9_s0_p7_1.vqh"
#include "books/coupled/_44c9_s0_p8_0.vqh"
#include "books/coupled/_44c9_s0_p8_1.vqh"
#include "books/coupled/_44c9_s0_p9_0.vqh"
#include "books/coupled/_44c9_s0_p9_1.vqh"
#include "books/coupled/_44c9_s0_p9_2.vqh"

/* residue backfill is entered in the template array as if stereo
   backfill is not in use.  It's up to vorbisenc to make the
   appropriate index adjustment */
static vorbis_residue_template _residue_template_44_stereo[11]={
  /* mode 0; 64-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   /* mostly temporary entries pending training */
   { {{0},{0,0,&_44c0_s0_p1_0},{0,0,&_44c0_s0_p2_0},{0,0,&_44c0_s0_p3_0},
      {&_44c0_s0_p4_0,&_44c0_s0_p4_1},{0,0,&_44c0_s0_p5_0},{0,0,&_44c0_s0_p6_0},
      {&_44c0_s0_p7_0,&_44c0_s0_p7_1,&_44c0_s0_p7_2}}, /* lossless stereo */
     {{0},{0,0,&_44c0_s0_p1_0},{0,0,&_44c0_s0_p2_0},{0,0,&_44c0_s0_p3_0},
      {&_44c0_s0_p4_0,&_44c0_s0_p4_1},{0,0,&_44c0_s1_p5_0},{0,0,&_44c0_s1_p6_0},
      {&_44c0_s1_p7_0,&_44c0_s1_p7_1,&_44c0_s1_p7_2}}, /* 6dB (2.5) stereo */
     {{0},{0,0,&_44c0_s0_p1_0},{0,0,&_44c0_s0_p2_0},{0,0,&_44c0_s0_p3_0},
      {&_44c0_s0_p4_0,&_44c0_s0_p4_1},{0,0,&_44c0_s1_p5_0},{0,0,&_44c0_s2_p6_0},
      {&_44c0_s2_p7_0,&_44c0_s2_p7_1,&_44c0_s2_p7_2}}, /* 12dB (4.5) stereo */
     {{0},{0,0,&_44c0_s0_p1_0},{0,0,&_44c0_s0_p2_0},{0,0,&_44c0_s0_p3_0},
      {&_44c0_s0_p4_0,&_44c0_s0_p4_1},{0,0,&_44c0_s1_p5_0},{0,0,&_44c0_s2_p6_0},
      {&_44c0_s3_p7_0,&_44c0_s3_p7_1,&_44c0_s3_p7_2}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0,0,0,0,0,&_44c0_s1_p5_s0,&_44c0_s1_p6_s0,&_44c0_s1_p7_s0}, /* 6dB (2.5) stereo */
     {0,0,0,0,0,0,&_44c0_s2_p6_s0,&_44c0_s2_p7_s0}, /* 12dB (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c0_s3_p7_s0}, /* 18dB (8.5) stereo */
     {0},
   },
   { 
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* lossless stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 6dB (2.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 12dB (4.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },
  /* mode 1; 80-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   /* mostly temporary entries pending training */
   { {{0},{0,0,&_44c1_s0_p1_0},{0,0,&_44c1_s0_p2_0},{0,0,&_44c1_s0_p3_0},
      {&_44c1_s0_p4_0,&_44c1_s0_p4_1},{0,0,&_44c1_s0_p5_0},{0,0,&_44c1_s0_p6_0},
      {&_44c1_s0_p7_0,&_44c1_s0_p7_1,&_44c1_s0_p7_2}}, /* lossless stereo */
     {{0},{0,0,&_44c1_s0_p1_0},{0,0,&_44c1_s0_p2_0},{0,0,&_44c1_s0_p3_0},
      {&_44c1_s0_p4_0,&_44c1_s0_p4_1},{0,0,&_44c1_s1_p5_0},{0,0,&_44c1_s1_p6_0},
      {&_44c1_s1_p7_0,&_44c1_s1_p7_1,&_44c1_s1_p7_2}}, /* 6dB (2.5) stereo */
     {{0},{0,0,&_44c1_s0_p1_0},{0,0,&_44c1_s0_p2_0},{0,0,&_44c1_s0_p3_0},
      {&_44c1_s0_p4_0,&_44c1_s0_p4_1},{0,0,&_44c1_s1_p5_0},{0,0,&_44c1_s2_p6_0},
      {&_44c1_s2_p7_0,&_44c1_s2_p7_1,&_44c1_s2_p7_2}}, /* 12dB (4.5) stereo */
     {{0},{0,0,&_44c1_s0_p1_0},{0,0,&_44c1_s0_p2_0},{0,0,&_44c1_s0_p3_0},
      {&_44c1_s0_p4_0,&_44c1_s0_p4_1},{0,0,&_44c1_s1_p5_0},{0,0,&_44c1_s2_p6_0},
      {&_44c1_s3_p7_0,&_44c1_s3_p7_1,&_44c1_s3_p7_2}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0,0,0,0,0,&_44c0_s1_p5_s0,&_44c0_s1_p6_s0,&_44c0_s1_p7_s0}, /* 6dB (2.5) stereo */
     {0,0,0,0,0,0,&_44c0_s2_p6_s0,&_44c0_s2_p7_s0}, /* 12dB (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c0_s3_p7_s0}, /* 18dB (8.5) stereo */
     {0},
   },
   { 
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* lossless stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 6dB (2.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 12dB (4.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },
  /* mode 2; 96-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   /* mostly temporary entries pending training */
   { {{0},{0,0,&_44c2_s0_p1_0},{0,0,&_44c2_s0_p2_0},{0,0,&_44c2_s0_p3_0},
      {&_44c2_s0_p4_0,&_44c2_s0_p4_1},{0,0,&_44c2_s0_p5_0},{0,0,&_44c2_s0_p6_0},
      {&_44c2_s0_p7_0,&_44c2_s0_p7_1,&_44c2_s0_p7_2}}, /* lossless stereo */
     {{0},{0,0,&_44c2_s0_p1_0},{0,0,&_44c2_s0_p2_0},{0,0,&_44c2_s0_p3_0},
      {&_44c2_s0_p4_0,&_44c2_s0_p4_1},{0,0,&_44c2_s1_p5_0},{0,0,&_44c2_s1_p6_0},
      {&_44c2_s1_p7_0,&_44c2_s1_p7_1,&_44c2_s1_p7_2}}, /* 6dB (2.5) stereo */
     {{0},{0,0,&_44c2_s0_p1_0},{0,0,&_44c2_s0_p2_0},{0,0,&_44c2_s0_p3_0},
      {&_44c2_s0_p4_0,&_44c2_s0_p4_1},{0,0,&_44c2_s1_p5_0},{0,0,&_44c2_s2_p6_0},
      {&_44c2_s2_p7_0,&_44c2_s2_p7_1,&_44c2_s2_p7_2}}, /* 12dB (4.5) stereo */
     {{0},{0,0,&_44c2_s0_p1_0},{0,0,&_44c2_s0_p2_0},{0,0,&_44c2_s0_p3_0},
      {&_44c2_s0_p4_0,&_44c2_s0_p4_1},{0,0,&_44c2_s1_p5_0},{0,0,&_44c2_s2_p6_0},
      {&_44c2_s3_p7_0,&_44c2_s3_p7_1,&_44c2_s3_p7_2}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0,0,0,0,0,&_44c0_s1_p5_s0,&_44c0_s1_p6_s0,&_44c0_s1_p7_s0}, /* 6dB (2.5) stereo */
     {0,0,0,0,0,0,&_44c0_s2_p6_s0,&_44c0_s2_p7_s0}, /* 12dB (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c0_s3_p7_s0}, /* 18dB (8.5) stereo */
     {0},
   },
   { 
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* lossless stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 6dB (2.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 12dB (4.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },
  /* mode 3; 112-ish */
  {{&_residue_44_low, &_residue_44_low},  
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   /* mostly temporary entries pending training */
   { {{0},{0,0,&_44c3_s0_p1_0},{0,0,&_44c3_s0_p2_0},{0,0,&_44c3_s0_p3_0},
      {&_44c3_s0_p4_0,&_44c3_s0_p4_1},{0,0,&_44c3_s0_p5_0},{0,0,&_44c3_s0_p6_0},
      {&_44c3_s0_p7_0,&_44c3_s0_p7_1,&_44c3_s0_p7_2}}, /* lossless stereo */
     {{0},{0,0,&_44c3_s0_p1_0},{0,0,&_44c3_s0_p2_0},{0,0,&_44c3_s0_p3_0},
      {&_44c3_s0_p4_0,&_44c3_s0_p4_1},{0,0,&_44c3_s1_p5_0},{0,0,&_44c3_s1_p6_0},
      {&_44c3_s1_p7_0,&_44c3_s1_p7_1,&_44c3_s1_p7_2}}, /* 6dB (2.5) stereo */
     {{0},{0,0,&_44c3_s0_p1_0},{0,0,&_44c3_s0_p2_0},{0,0,&_44c3_s0_p3_0},
      {&_44c3_s0_p4_0,&_44c3_s0_p4_1},{0,0,&_44c3_s1_p5_0},{0,0,&_44c3_s2_p6_0},
      {&_44c3_s2_p7_0,&_44c3_s2_p7_1,&_44c3_s2_p7_2}}, /* 12dB (4.5) stereo */
     {{0},{0,0,&_44c3_s0_p1_0},{0,0,&_44c3_s0_p2_0},{0,0,&_44c3_s0_p3_0},
      {&_44c3_s0_p4_0,&_44c3_s0_p4_1},{0,0,&_44c3_s1_p5_0},{0,0,&_44c3_s2_p6_0},
      {&_44c3_s3_p7_0,&_44c3_s3_p7_1,&_44c3_s3_p7_2}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0,0,0,0,0,&_44c0_s1_p5_s0,&_44c0_s1_p6_s0,&_44c0_s1_p7_s0}, /* 6dB (2.5) stereo */
     {0,0,0,0,0,0,&_44c0_s2_p6_s0,&_44c0_s2_p7_s0}, /* 12dB (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c0_s3_p7_s0}, /* 18dB (8.5) stereo */
     {0},
   },
   { 
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* lossless stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
      {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 6dB (2.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 12dB (4.5) stereo */
     {{&_44c0_s0_p0_r0,&_44c0_s0_p0_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s1_pS_r0,&_44c0_s1_pS_r1},
       {&_44c0_s0_pN_r0,&_44c0_s0_pN_r1}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },

  /* mode 4; 128-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c4_short,&_huff_book__44c4_long},
   { 
     {{0},{0,0,&_44c4_s0_p1_0},{0,0,&_44c4_s0_p2_0},{0,0,&_44c4_s0_p3_0},
      {0,0,&_44c4_s0_p4_0},{0,0,&_44c4_s0_p5_0},{0,0,&_44c4_s0_p6_0},
      {&_44c4_s0_p7_0,&_44c4_s0_p7_1},{&_44c4_s0_p8_0,&_44c4_s0_p8_1},
      {&_44c4_s0_p9_0,&_44c4_s0_p9_1,&_44c4_s0_p9_2}},
     {{0},{0,0,&_44c4_s0_p1_0},{0,0,&_44c4_s1_p2_0},{0,0,&_44c4_s0_p3_0},
      {0,0,&_44c4_s1_p4_0},{0,0,&_44c4_s0_p5_0},{0,0,&_44c4_s1_p6_0},
      {&_44c4_s1_p7_0,&_44c4_s1_p7_1},{&_44c4_s1_p8_0,&_44c4_s1_p8_1},
      {&_44c4_s1_p9_0,&_44c4_s1_p9_1,&_44c4_s1_p9_2}},
     {{0},{0,0,&_44c4_s0_p1_0},{0,0,&_44c4_s1_p2_0},{0,0,&_44c4_s0_p3_0},
      {0,0,&_44c4_s1_p4_0},{0,0,&_44c4_s0_p5_0},{0,0,&_44c4_s2_p6_0},
      {&_44c4_s2_p7_0,&_44c4_s2_p7_1},{&_44c4_s2_p8_0,&_44c4_s2_p8_1},
      {&_44c4_s2_p9_0,&_44c4_s2_p9_1,&_44c4_s2_p9_2}},
     {{0},{0,0,&_44c4_s0_p1_0},{0,0,&_44c4_s1_p2_0},{0,0,&_44c4_s0_p3_0},
      {0,0,&_44c4_s1_p4_0},{0,0,&_44c4_s0_p5_0},{0,0,&_44c4_s2_p6_0},
      {&_44c4_s3_p7_0,&_44c4_s3_p7_1},{&_44c4_s3_p8_0,&_44c4_s3_p8_1},
      {&_44c4_s3_p9_0,&_44c4_s3_p9_1,&_44c4_s3_p9_2}},
     {{0}}, /* 24dB (16.5) stereo */
   },

   { {0}, /* lossless stereo */
     {0,0,&_44c4_s1_p2_s0,0,&_44c4_s1_p4_s0,0,&_44c4_s1_p6_s0,&_44c4_s1_p7_s0,
      &_44c4_s1_p8_s0,&_44c4_s1_p9_s0}, /* (2.5) stereo */
     {0,0,0,0,0,0,&_44c4_s2_p6_s0,&_44c4_s2_p7_s0,&_44c4_s2_p8_s0,
      &_44c4_s2_p9_s0}, /* (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c4_s3_p9_s0,&_44c4_s3_p9_s0,&_44c4_s3_p9_s0}, /* (8.5) stereo */
     {0},
   },
   {
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* lossless stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 6dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 12dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 18dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 24dB stereo */
   },
  },
  /* mode 5; 160-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c5_short,&_huff_book__44c5_long},
   { 
     {{0},{0,0,&_44c5_s0_p1_0},{0,0,&_44c5_s0_p2_0},{0,0,&_44c5_s0_p3_0},
      {0,0,&_44c5_s0_p4_0},{0,0,&_44c5_s0_p5_0},{0,0,&_44c5_s0_p6_0},
      {&_44c5_s0_p7_0,&_44c5_s0_p7_1},{&_44c5_s0_p8_0,&_44c5_s0_p8_1},
      {&_44c5_s0_p9_0,&_44c5_s0_p9_1,&_44c5_s0_p9_2}},
     {{0},{0,0,&_44c5_s0_p1_0},{0,0,&_44c5_s1_p2_0},{0,0,&_44c5_s0_p3_0},
      {0,0,&_44c5_s1_p4_0},{0,0,&_44c5_s0_p5_0},{0,0,&_44c5_s1_p6_0},
      {&_44c5_s1_p7_0,&_44c5_s1_p7_1},{&_44c5_s1_p8_0,&_44c5_s1_p8_1},
      {&_44c5_s1_p9_0,&_44c5_s1_p9_1,&_44c5_s1_p9_2}},
     {{0},{0,0,&_44c5_s0_p1_0},{0,0,&_44c5_s1_p2_0},{0,0,&_44c5_s0_p3_0},
      {0,0,&_44c5_s1_p4_0},{0,0,&_44c5_s0_p5_0},{0,0,&_44c5_s2_p6_0},
      {&_44c5_s2_p7_0,&_44c5_s2_p7_1},{&_44c5_s2_p8_0,&_44c5_s2_p8_1},
      {&_44c5_s2_p9_0,&_44c5_s2_p9_1,&_44c5_s2_p9_2}},
     {{0},{0,0,&_44c5_s0_p1_0},{0,0,&_44c5_s1_p2_0},{0,0,&_44c5_s0_p3_0},
      {0,0,&_44c5_s1_p4_0},{0,0,&_44c5_s0_p5_0},{0,0,&_44c5_s2_p6_0},
      {&_44c5_s3_p7_0,&_44c5_s3_p7_1},{&_44c5_s3_p8_0,&_44c5_s3_p8_1},
      {&_44c5_s3_p9_0,&_44c5_s3_p9_1,&_44c5_s3_p9_2}},
     {{0}}, /* 24dB (16.5) stereo */
   },

   { {0}, /* lossless stereo */
     {0,0,&_44c4_s1_p2_s0,0,&_44c4_s1_p4_s0,0,&_44c4_s1_p6_s0,&_44c4_s1_p7_s0,
      &_44c4_s1_p8_s0,&_44c4_s1_p9_s0}, /* (2.5) stereo */
     {0,0,0,0,0,0,&_44c4_s2_p6_s0,&_44c4_s2_p7_s0,&_44c4_s2_p8_s0,
      &_44c4_s2_p9_s0}, /* (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c4_s3_p9_s0,&_44c4_s3_p9_s0,&_44c4_s3_p9_s0}, /* (8.5) stereo */
     {0},
   },
   {
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* lossless stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 6dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 12dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 18dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 24dB stereo */
   },
  },
  /* mode 6; 192-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c6_short,&_huff_book__44c6_long},
   { 
     {{0},{0,0,&_44c6_s0_p1_0},{0,0,&_44c6_s0_p2_0},{0,0,&_44c6_s0_p3_0},
      {0,0,&_44c6_s0_p4_0},{0,0,&_44c6_s0_p5_0},{0,0,&_44c6_s0_p6_0},
      {&_44c6_s0_p7_0,&_44c6_s0_p7_1},{&_44c6_s0_p8_0,&_44c6_s0_p8_1},
      {&_44c6_s0_p9_0,&_44c6_s0_p9_1,&_44c6_s0_p9_2}},
     {{0},{0,0,&_44c6_s0_p1_0},{0,0,&_44c6_s1_p2_0},{0,0,&_44c6_s0_p3_0},
      {0,0,&_44c6_s1_p4_0},{0,0,&_44c6_s0_p5_0},{0,0,&_44c6_s1_p6_0},
      {&_44c6_s1_p7_0,&_44c6_s1_p7_1},{&_44c6_s1_p8_0,&_44c6_s1_p8_1},
      {&_44c6_s1_p9_0,&_44c6_s1_p9_1,&_44c6_s1_p9_2}},
     {{0},{0,0,&_44c6_s0_p1_0},{0,0,&_44c6_s1_p2_0},{0,0,&_44c6_s0_p3_0},
      {0,0,&_44c6_s1_p4_0},{0,0,&_44c6_s0_p5_0},{0,0,&_44c6_s2_p6_0},
      {&_44c6_s2_p7_0,&_44c6_s2_p7_1},{&_44c6_s2_p8_0,&_44c6_s2_p8_1},
      {&_44c6_s2_p9_0,&_44c6_s2_p9_1,&_44c6_s2_p9_2}},
     {{0},{0,0,&_44c6_s0_p1_0},{0,0,&_44c6_s1_p2_0},{0,0,&_44c6_s0_p3_0},
      {0,0,&_44c6_s1_p4_0},{0,0,&_44c6_s0_p5_0},{0,0,&_44c6_s2_p6_0},
      {&_44c6_s3_p7_0,&_44c6_s3_p7_1},{&_44c6_s3_p8_0,&_44c6_s3_p8_1},
      {&_44c6_s3_p9_0,&_44c6_s3_p9_1,&_44c6_s3_p9_2}},
     {{0}}, /* 24dB (16.5) stereo */
   },

   { {0}, /* lossless stereo */
     {0,0,&_44c4_s1_p2_s0,0,&_44c4_s1_p4_s0,0,&_44c4_s1_p6_s0,&_44c4_s1_p7_s0,
      &_44c4_s1_p8_s0,&_44c4_s1_p9_s0}, /* (2.5) stereo */
     {0,0,0,0,0,0,&_44c4_s2_p6_s0,&_44c4_s2_p7_s0,&_44c4_s2_p8_s0,
      &_44c4_s2_p9_s0}, /* (4.5) stereo */
     {0,0,0,0,0,0,0,&_44c4_s3_p9_s0,&_44c4_s3_p9_s0,&_44c4_s3_p9_s0}, /* (8.5) stereo */
     {0},
   },
   {
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* lossless stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 6dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 12dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 18dB stereo */
     {{&_44c4_s0_p0_r0,&_44c4_s0_p0_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s1_pS_r0,&_44c4_s1_pS_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1},
      {&_44c4_s0_pN_r0,&_44c4_s0_pN_r1}}, /* 24dB stereo */
   },
  },

  /* mode 7; 224-ish */
  {{&_residue_44_high, &_residue_44_high},  
   {&_huff_book__44c7_short,&_huff_book__44c7_long},
   { {{0},{&_44c7_s0_p1_0,&_44c7_s0_p1_1},
      {&_44c7_s0_p2_0,&_44c7_s0_p2_1},
      {0,0,&_44c7_s0_p3_0},{0,0,&_44c7_s0_p4_0},{0,0,&_44c7_s0_p5_0},
      {&_44c7_s0_p6_0,&_44c7_s0_p6_1},
      {&_44c7_s0_p7_0,&_44c7_s0_p7_1},
      {&_44c7_s0_p8_0,&_44c7_s0_p8_1},
      {&_44c7_s0_p9_0,&_44c7_s0_p9_1,&_44c7_s0_p9_2}}, 
     {{0}}, /* 6dB  (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     /*{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},*/ /* 24dB (16.5) stereo */
   },
   { 
     {{&_44c7_s0_p0_r0,&_44c7_s0_p0_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     /*{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
      {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
      {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
      {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
      {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}},*/ /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },

  /* mode 8; 256-ish */
  {{&_residue_44_high, &_residue_44_high},  
   {&_huff_book__44c8_short,&_huff_book__44c8_long},
   { {{0},{&_44c8_s0_p1_0,&_44c8_s0_p1_1},
      {&_44c8_s0_p2_0,&_44c8_s0_p2_1},
      {0,0,&_44c8_s0_p3_0},{0,0,&_44c8_s0_p4_0},{0,0,&_44c8_s0_p5_0},
      {&_44c8_s0_p6_0,&_44c8_s0_p6_1},
      {&_44c8_s0_p7_0,&_44c8_s0_p7_1},
      {&_44c8_s0_p8_0,&_44c8_s0_p8_1},
      {&_44c8_s0_p9_0,&_44c8_s0_p9_1,&_44c8_s0_p9_2}}, 
     {{0}}, /* 6dB  (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     /*{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},*/ /* 24dB (16.5) stereo */
   },
   { 
     {{&_44c7_s0_p0_r0,&_44c7_s0_p0_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     /*{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
      {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
      {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
      {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
      {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}},*/ /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  },
  /* mode 9; 320-ish */
  {{&_residue_44_high, &_residue_44_high},  
   {&_huff_book__44c9_short,&_huff_book__44c9_long},
   { {{0},{&_44c9_s0_p1_0,&_44c9_s0_p1_1},
      {&_44c9_s0_p2_0,&_44c9_s0_p2_1},
      {0,0,&_44c9_s0_p3_0},{0,0,&_44c9_s0_p4_0},{0,0,&_44c9_s0_p5_0},
      {&_44c9_s0_p6_0,&_44c9_s0_p6_1},
      {&_44c9_s0_p7_0,&_44c9_s0_p7_1},
      {&_44c9_s0_p8_0,&_44c9_s0_p8_1},
      {&_44c9_s0_p9_0,&_44c9_s0_p9_1,&_44c9_s0_p9_2}}, 
     {{0}}, /* 6dB  (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
   { {0}, /* lossless stereo */
     {0}, /* 6dB (2.5) stereo */
     {0}, /* 12dB (4.5) stereo */
     {0}, /* 18dB (8.5) stereo */
     {0},
     /*{0,0,0,0,0,0,0,0,&44c0_s4_s8,&44c0_s4_s9},*/ /* 24dB (16.5) stereo */
   },
   { 
     {{&_44c7_s0_p0_r0,&_44c7_s0_p0_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1},
      {&_44c7_s0_pN_r0,&_44c7_s0_pN_r1}}, /* lossless stereo */
     {{0}}, /* 6dB (2.5) stereo */
     {{0}}, /* 12dB (4.5) stereo */
     {{0}}, /* 18dB (8.5) stereo */
     /*{{&44c0_s0_r0_0,&44c0_s0_r0_1},{&44c0_s0_r1_0,&44c0_s0_r1_1},
      {&44c0_s0_r2_0,&44c0_s0_r2_1},{&44c0_s0_r3_0,&44c0_s0_r3_1},
      {&44c0_s0_r4_0,&44c0_s0_r4_1},{&44c0_s1_r5_0,&44c0_s1_r5_1},
      {&44c0_s1_r6_0,&44c0_s1_r6_1},{&44c0_s2_r7_0,&44c0_s2_r7_1},
      {&44c0_s4_r8_0,&44c0_s3_r8_1},{&44c0_s4_r9_0,&44c0_s3_r9_1}},*/ /* 18dB (8.5) stereo */
     {{0}}, /* 24dB (16.5) stereo */
   },
  }

};

#include "books/uncoupled/_44u0_p1_0.vqh"
#include "books/uncoupled/_44u0_p2_0.vqh"
#include "books/uncoupled/_44u0_p3_0.vqh"
#include "books/uncoupled/_44u0_p4_0.vqh"
#include "books/uncoupled/_44u0_p4_1.vqh"
#include "books/uncoupled/_44u0_p5_0.vqh"
#include "books/uncoupled/_44u0_p6_0.vqh"
#include "books/uncoupled/_44u0_p7_0.vqh"
#include "books/uncoupled/_44u0_p7_1.vqh"
#include "books/uncoupled/_44u0_p7_2.vqh"

#include "books/uncoupled/_44u0_p0_r0.vqh"
#include "books/uncoupled/_44u0_p1_r0.vqh"

#include "books/uncoupled/_44u0_p0_r1.vqh"
#include "books/uncoupled/_44u0_p1_r1.vqh"

#include "books/uncoupled/_44u4_p1_0.vqh"
#include "books/uncoupled/_44u4_p2_0.vqh"
#include "books/uncoupled/_44u4_p3_0.vqh"
#include "books/uncoupled/_44u4_p4_0.vqh"
#include "books/uncoupled/_44u4_p5_0.vqh"
#include "books/uncoupled/_44u4_p6_0.vqh"
#include "books/uncoupled/_44u4_p7_0.vqh"
#include "books/uncoupled/_44u4_p7_1.vqh"
#include "books/uncoupled/_44u4_p8_0.vqh"
#include "books/uncoupled/_44u4_p8_1.vqh"
#include "books/uncoupled/_44u4_p9_0.vqh"
#include "books/uncoupled/_44u4_p9_1.vqh"
#include "books/uncoupled/_44u4_p9_2.vqh"

#include "books/uncoupled/_44u4_p0_r0.vqh"
#include "books/uncoupled/_44u4_p1_r0.vqh"

#include "books/uncoupled/_44u4_p0_r1.vqh"
#include "books/uncoupled/_44u4_p1_r1.vqh"

#include "books/uncoupled/_44u7_p1_0.vqh"
#include "books/uncoupled/_44u7_p2_0.vqh"
#include "books/uncoupled/_44u7_p2_1.vqh"
#include "books/uncoupled/_44u7_p3_0.vqh"
#include "books/uncoupled/_44u7_p4_0.vqh"
#include "books/uncoupled/_44u7_p5_0.vqh"
#include "books/uncoupled/_44u7_p6_0.vqh"
#include "books/uncoupled/_44u7_p7_0.vqh"
#include "books/uncoupled/_44u7_p7_1.vqh"
#include "books/uncoupled/_44u7_p8_0.vqh"
#include "books/uncoupled/_44u7_p8_1.vqh"
#include "books/uncoupled/_44u7_p9_0.vqh"
#include "books/uncoupled/_44u7_p9_1.vqh"
#include "books/uncoupled/_44u7_p9_2.vqh"

#include "books/uncoupled/_44u7_p0_r0.vqh"
#include "books/uncoupled/_44u7_p1_r0.vqh"

#include "books/uncoupled/_44u7_p0_r1.vqh"
#include "books/uncoupled/_44u7_p1_r1.vqh"


static vorbis_residue_template _residue_template_44_uncoupled[11]={
  /* mode 0; 40/c-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c0_short,&_huff_book__44c0_long},
   { {{0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u0_p0_r0,&_44u0_p0_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1}},
   }
  },
  /* mode 1; 50-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c1_short,&_huff_book__44c1_long},
   { {{0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u0_p0_r0,&_44u0_p0_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1}},
   }
  },
  /* mode 2; 60-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c2_short,&_huff_book__44c2_long},
   { {{0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u0_p0_r0,&_44u0_p0_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1}},
   }
  },
  /* mode 3; 70-ish */
  {{&_residue_44_low_un, &_residue_44_low_un},  
   {&_huff_book__44c3_short,&_huff_book__44c3_long},
   { {{0},
      {0,0,&_44u0_p1_0},
      {0,0,&_44u0_p2_0},
      {0,0,&_44u0_p3_0},
      {&_44u0_p4_0,&_44u0_p4_1},
      {0,0,&_44u0_p5_0},
      {0,0,&_44u0_p6_0},
      {&_44u0_p7_0,&_44u0_p7_1,&_44u0_p7_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u0_p0_r0,&_44u0_p0_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1},
      {&_44u0_p1_r0,&_44u0_p1_r1}},
   }
  },
  /* mode 4; 80-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c4_short,&_huff_book__44c4_long},
   { {{0},
      {0,0,&_44u4_p1_0},
      {0,0,&_44u4_p2_0},
      {0,0,&_44u4_p3_0},
      {0,0,&_44u4_p4_0},
      {0,0,&_44u4_p5_0},
      {0,0,&_44u4_p6_0},
      {&_44u4_p7_0,&_44u4_p7_1},
      {&_44u4_p8_0,&_44u4_p8_1},
      {&_44u4_p9_0,&_44u4_p9_1,&_44u4_p9_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u4_p0_r0,&_44u4_p0_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1}},
   }
  },
  /* mode 5; 90-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c5_short,&_huff_book__44c5_long},
   { {{0},
      {0,0,&_44u4_p1_0},
      {0,0,&_44u4_p2_0},
      {0,0,&_44u4_p3_0},
      {0,0,&_44u4_p4_0},
      {0,0,&_44u4_p5_0},
      {0,0,&_44u4_p6_0},
      {&_44u4_p7_0,&_44u4_p7_1},
      {&_44u4_p8_0,&_44u4_p8_1},
      {&_44u4_p9_0,&_44u4_p9_1,&_44u4_p9_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u4_p0_r0,&_44u4_p0_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1}},
   }
  },
  /* mode 6; 100-ish */
  {{&_residue_44_mid, &_residue_44_mid},  
   {&_huff_book__44c6_short,&_huff_book__44c6_long},
   { {{0},
      {0,0,&_44u4_p1_0},
      {0,0,&_44u4_p2_0},
      {0,0,&_44u4_p3_0},
      {0,0,&_44u4_p4_0},
      {0,0,&_44u4_p5_0},
      {0,0,&_44u4_p6_0},
      {&_44u4_p7_0,&_44u4_p7_1},
      {&_44u4_p8_0,&_44u4_p8_1},
      {&_44u4_p9_0,&_44u4_p9_1,&_44u4_p9_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u4_p0_r0,&_44u4_p0_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1},
      {&_44u4_p1_r0,&_44u4_p1_r1}},
   }
  },
  /* mode 7 */
  {{&_residue_44_high_un, &_residue_44_high_un},  
   {&_huff_book__44c7_short,&_huff_book__44c7_long},
   { {{0},
      {0,0,&_44u7_p1_0},
      {&_44u7_p2_0,&_44u7_p2_1},
      {0,0,&_44u7_p3_0},
      {0,0,&_44u7_p4_0},
      {0,0,&_44u7_p5_0},
      {0,0,&_44u7_p6_0},
      {&_44u7_p7_0,&_44u7_p7_1},
      {&_44u7_p8_0,&_44u7_p8_1},
      {&_44u7_p9_0,&_44u7_p9_1,&_44u7_p9_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u7_p0_r0,&_44u7_p0_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1}},
   }
  },
  /* mode 8 */
  {{&_residue_44_high_un, &_residue_44_high_un},  
   {&_huff_book__44c8_short,&_huff_book__44c8_long},
   { {{0},
      {0,0,&_44u7_p1_0},
      {&_44u7_p2_0,&_44u7_p2_1},
      {0,0,&_44u7_p3_0},
      {0,0,&_44u7_p4_0},
      {0,0,&_44u7_p5_0},
      {0,0,&_44u7_p6_0},
      {&_44u7_p7_0,&_44u7_p7_1},
      {&_44u7_p8_0,&_44u7_p8_1},
      {&_44u7_p9_0,&_44u7_p9_1,&_44u7_p9_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u7_p0_r0,&_44u7_p0_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1}},
   }
  },
  /* mode 9 */
  {{&_residue_44_high_un, &_residue_44_high_un},  
   {&_huff_book__44c9_short,&_huff_book__44c9_long},
   { {{0},
      {0,0,&_44u7_p1_0},
      {&_44u7_p2_0,&_44u7_p2_1},
      {0,0,&_44u7_p3_0},
      {0,0,&_44u7_p4_0},
      {0,0,&_44u7_p5_0},
      {0,0,&_44u7_p6_0},
      {&_44u7_p7_0,&_44u7_p7_1},
      {&_44u7_p8_0,&_44u7_p8_1},
      {&_44u7_p9_0,&_44u7_p9_1,&_44u7_p9_2}},
   },
   { {0} }, /* no stereo backfill in uncoupled modes */
   { {{&_44u7_p0_r0,&_44u7_p0_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1},
      {&_44u7_p1_r0,&_44u7_p1_r1}},
   }
  },
};





