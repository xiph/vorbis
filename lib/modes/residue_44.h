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
 last mod: $Id: residue_44.h,v 1.11.6.7 2002/06/24 00:06:05 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

/***** residue backends *********************************************/

/* the books used depend on stereo-or-not, but the residue setup below
   can apply to coupled or not.  These templates are for a first pass;
   a last pass is mechanically added in vorbisenc for residue backfill
   at 1/3 and 1/9, as well as an optional middle pass for stereo
   backfill */


static vorbis_info_residue0 _residue_44_mid={
  0,-1, -1, 10,-1,
  /*  0     1     2     3     4     5     6     7     8  */
  {0},
  {-1},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  8.5,  16.5, 32.5},
  {  .5,   .5, 999.,   .5,  999., 4.5,  8.5,  16.5, 32.5},
};

static vorbis_info_residue0 _residue_44_lo={
  0,-1, -1, 12,-1,
  /*  0     1     2     3     4     5     6     7     8     9     10 */
  {0},
  {-1},
  {  .5,  1.5,  1.5,  2.5,  2.5,  4.5,  4.5,  8.5,  8.5,  16.5, 32.5},
  {  .5,   .5, 999.,   .5,  999.,  .5,  4.5,   .5,  8.5,  16.5, 32.5},
};

#include "books/coupled/_44c0_s_short.vqh"
#include "books/coupled/_44c0_s_long.vqh"

#include "books/coupled/_44c0_s_p1_0.vqh"
#include "books/coupled/_44c0_s_p2_0.vqh"
#include "books/coupled/_44c0_s_p3_0.vqh"
#include "books/coupled/_44c0_s_p4_0.vqh"
#include "books/coupled/_44c0_s_p5_0.vqh"
#include "books/coupled/_44c0_s_p6_0.vqh"
#include "books/coupled/_44c0_s_p7_0.vqh"
#include "books/coupled/_44c0_s_p8_0.vqh"
#include "books/coupled/_44c0_s_p9_0.vqh"
#include "books/coupled/_44c0_s_p9_1.vqh"
#include "books/coupled/_44c0_s_p10_0.vqh"
#include "books/coupled/_44c0_s_p10_1.vqh"
#include "books/coupled/_44c0_s_p11_0.vqh"
#include "books/coupled/_44c0_s_p11_1.vqh"

#include "books/coupled/_44c4_s_short.vqh"
#include "books/coupled/_44c4_s_long.vqh"

#include "books/coupled/_44c4_s_p1_0.vqh"
#include "books/coupled/_44c4_s_p2_0.vqh"
#include "books/coupled/_44c4_s_p3_0.vqh"
#include "books/coupled/_44c4_s_p4_0.vqh"
#include "books/coupled/_44c4_s_p5_0.vqh"
#include "books/coupled/_44c4_s_p6_0.vqh"
#include "books/coupled/_44c4_s_p7_0.vqh"
#include "books/coupled/_44c4_s_p7_1.vqh"
#include "books/coupled/_44c4_s_p8_0.vqh"
#include "books/coupled/_44c4_s_p8_1.vqh"
#include "books/coupled/_44c4_s_p9_0.vqh"
#include "books/coupled/_44c4_s_p9_1.vqh"
#include "books/coupled/_44c4_s_p9_2.vqh"

#include "books/coupled/_44c4_sm_short.vqh"
#include "books/coupled/_44c4_sm_long.vqh"


/* mapping conventions:
   only one submap (this would change for efficient 5.1 support for example)*/
/* Four psychoacoustic profiles are used, one for each blocktype */
static vorbis_info_mapping0 _map_nominal[2]={
  {1, {0,0}, {0}, {0}, 1,{0},{1}},
  {1, {0,0}, {1}, {1}, 1,{0},{1}}
};

static static_bookblock _resbook_44s_4={
  {
    {0},{0,0,&_44c4_s_p1_0},{0,0,&_44c4_s_p2_0},{0,0,&_44c4_s_p3_0},
    {0,0,&_44c4_s_p4_0},{0,0,&_44c4_s_p5_0},{0,0,&_44c4_s_p6_0},
    {&_44c4_s_p7_0,&_44c4_s_p7_1},{&_44c4_s_p8_0,&_44c4_s_p8_1},
    {&_44c4_s_p9_0,&_44c4_s_p9_1,&_44c4_s_p9_2}
   }
};

static static_bookblock _resbook_44s_0={
  {
    {0},
    {0,0,&_44c0_s_p1_0},{0,0,&_44c0_s_p2_0},
    {0,&_44c0_s_p3_0},  {0,&_44c0_s_p4_0},
    {0,&_44c0_s_p5_0},  {0,&_44c0_s_p6_0},
    {0,&_44c0_s_p7_0},  {0,&_44c0_s_p8_0},
    {&_44c0_s_p9_0,&_44c0_s_p9_1},
    {&_44c0_s_p10_0,&_44c0_s_p10_1},
    {&_44c0_s_p11_0,&_44c0_s_p11_1}
   }
};

static vorbis_residue_template _res_44s_4[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__44c4_s_short,&_huff_book__44c4_sm_short,&_resbook_44s_4},

  {2,0,  &_residue_44_mid,
   &_huff_book__44c4_s_long,&_huff_book__44c4_sm_long,&_resbook_44s_4}
};

static vorbis_residue_template _res_44s_0[]={
  {2,0,  &_residue_44_lo,
   &_huff_book__44c0_s_short,&_huff_book__44c0_s_short,&_resbook_44s_0},

  {2,0,  &_residue_44_lo,
   &_huff_book__44c0_s_long,&_huff_book__44c0_s_long,&_resbook_44s_0}
};

static vorbis_mapping_template _mapres_template_44_stereo[]={
  { _map_nominal, _res_44s_4 }, /* 0 */
  { _map_nominal, _res_44s_4 }, /* 1 */
  { _map_nominal, _res_44s_4 }, /* 2 */
  { _map_nominal, _res_44s_4 }, /* 3 */
  { _map_nominal, _res_44s_4 }, /* 4 */
  { _map_nominal, _res_44s_4 }, /* 5 */
  { _map_nominal, _res_44s_4 }, /* 6 */
  { _map_nominal, _res_44s_4 }, /* 7 */
  { _map_nominal, _res_44s_4 }, /* 8 */
  { _map_nominal, _res_44s_4 }, /* 9 */
};
