/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and The XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: registry for time, floor, res backends and channel mappings
 last mod: $Id: registry.h,v 1.1 2000/01/20 04:43:04 xiphmont Exp $

 ********************************************************************/

#ifndef _V_REG_H_
#define _V_REG_H_

typedef void _vi_info_time;
typedef void _vi_info_floor;
typedef void _vi_info_res;
typedef void _vi_info_map;

#define VI_TIMEB 1
extern _vi_info_time  *(*vorbis_time_dup_P[])    (_vi_info_time *);
extern void            (*vorbis_time_free_P[])   (_vi_info_time *);
extern void            (*vorbis_time_pack_P[])   (oggpack_buffer *,
						  _vi_info_time *);
extern _vi_info_time  *(*vorbis_time_unpack_P[]) (vorbis_info *,
						  oggpack_buffer *);

#define VI_FLOORB 1
extern _vi_info_floor *(*vorbis_floor_dup_P[])   (_vi_info_floor *);
extern void            (*vorbis_floor_free_P[])  (_vi_info_floor *);
extern void            (*vorbis_floor_pack_P[])  (oggpack_buffer *,
						  _vi_info_floor*);
extern _vi_info_floor *(*vorbis_floor_unpack_P[])(vorbis_info *,
						  oggpack_buffer *);

#define VI_RESB 1
extern _vi_info_res   *(*vorbis_res_dup_P[])     (_vi_info_res *);
extern void            (*vorbis_res_free_P[])    (_vi_info_res *);
extern void            (*vorbis_res_pack_P[])    (oggpack_buffer *,
						  _vi_info_res *);
extern _vi_info_res   *(*vorbis_res_unpack_P[])  (vorbis_info *,
						  oggpack_buffer *);

#define VI_MAPB 1
extern _vi_info_map   *(*vorbis_map_dup_P[])     (vorbis_info *,
						  _vi_info_map *);
extern void            (*vorbis_map_free_P[])    (_vi_info_map *);
extern void            (*vorbis_map_pack_P[])    (vorbis_info *,
						  oggpack_buffer *,
						  _vi_info_map *);
extern _vi_info_map   *(*vorbis_map_unpack_P[])  (vorbis_info *,
						  oggpack_buffer *);

extern int   (*vorbis_map_analysis_P[]) (vorbis_block *vb,_vi_info_map *,
					 ogg_packet *);
extern int   (*vorbis_map_synthesis_P[])(vorbis_block *vb,_vi_info_map *,
					 ogg_packet *);

#endif
