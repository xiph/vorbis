/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2002             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: toplevel residue templates 16/22kHz
 last mod: $Id: residue_16.h,v 1.1 2002/07/10 03:04:22 xiphmont Exp $

 ********************************************************************/

/***** residue backends *********************************************/

/*static static_bookblock _resbook_16s_0={
  {
    {0},{0,0,&_16c0_s_p1_0},{0,0,&_16c0_s_p2_0},{0,0,&_16c0_s_p3_0},
    {0,0,&_16c0_s_p4_0},{0,0,&_16c0_s_p5_0},{0,0,&_16c0_s_p6_0},
    {&_16c0_s_p7_0,&_16c0_s_p7_1},{&_16c0_s_p8_0,&_16c0_s_p8_1},
    {&_16c0_s_p9_0,&_16c0_s_p9_1,&_16c0_s_p9_2}
   }
};
static static_bookblock _resbook_16s_1={
  {
    {0},{0,0,&_16c1_s_p1_0},{0,0,&_16c1_s_p2_0},{0,0,&_16c1_s_p3_0},
    {0,0,&_16c1_s_p4_0},{0,0,&_16c1_s_p5_0},{0,0,&_16c1_s_p6_0},
    {&_16c1_s_p7_0,&_16c1_s_p7_1},{&_16c1_s_p8_0,&_16c1_s_p8_1},
    {&_16c1_s_p9_0,&_16c1_s_p9_1,&_16c1_s_p9_2}
   }
};

static vorbis_residue_template _res_16s_0[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__16c0_s_short,&_huff_book__16c0_sm_short,
   &_resbook_16s_0,&_resbook_16s_0},

  {2,0,  &_residue_44_mid,
   &_huff_book__16c0_s_long,&_huff_book__16c0_sm_long,
   &_resbook_16s_0,&_resbook_16s_0}
   };*/


static vorbis_mapping_template _mapres_template_16_stereo[3]={
  { _map_nominal, _res_44s_0 }, /* 0 */
  { _map_nominal, _res_44s_0 }, /* 1 */
  { _map_nominal, _res_44s_6 }, /* 2 */
};
static vorbis_mapping_template _mapres_template_16_uncoupled[3]={
  { _map_nominal_u, _res_44u_0 }, /* 0 */
  { _map_nominal_u, _res_44u_0 }, /* 1 */
  { _map_nominal_u, _res_44u_6 }, /* 2 */
};
