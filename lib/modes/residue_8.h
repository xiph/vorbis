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

 function: toplevel residue templates 8/11kHz
 last mod: $Id: residue_8.h,v 1.1 2002/07/10 03:04:22 xiphmont Exp $

 ********************************************************************/

#include "vorbis/codec.h"
#include "backends.h"

/***** residue backends *********************************************/

static vorbis_residue_template _res_8s_0[]={
  {2,0,  &_residue_44_mid,
   &_huff_book__8c0_s_single,&_huff_book__8c0_s_single,
   &_resbook_44s_0,&_resbook_44sm_0},
};

static vorbis_mapping_template _mapres_template_8_stereo[2]={
  { _map_nominal, _res_8s_0 }, /* 0 */
  { _map_nominal, _res_8s_0 }, /* 1 */
};

static vorbis_residue_template _res_8u_0[]={
  {1,0,  &_residue_44_low_un,
   &_huff_book__44u0__short,&_huff_book__44u0__short,
   &_resbook_44u_0,&_resbook_44u_0},
};

static vorbis_mapping_template _mapres_template_8_uncoupled[2]={
  { _map_nominal_u, _res_8u_0 }, /* 0 */
  { _map_nominal_u, _res_8u_0 }, /* 1 */
};
