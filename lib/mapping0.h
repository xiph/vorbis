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

 function: channel mapping 0 implementation
 last mod: $Id: mapping0.h,v 1.1 2000/01/20 04:43:02 xiphmont Exp $

 ********************************************************************/

typedef struct vorbis_info_mapping0{
  int    timesubmap;    /* list position of the time backend/settings
                           we're using */

  int   *floorsubmap;   /* for each floor map, which channels */
  int   *residuesubmap; /* residue to use for each incoming channel */
  int   *psysubmap;     /* psychoacoustics to use for each incoming channel */

} vorbis_info_mapping0;

extern _vi_info_map *_vorbis_map0_dup      (vorbis_info *vi,
					    _vi_info_map *source);
extern void          _vorbis_map0_free     (_vi_info_map *i);
extern void          _vorbis_map0_pack     (vorbis_info *vi,
					    oggpack_buffer *opb,
					    _vi_info_map *i);
extern _vi_info_map *_vorbis_map0_unpack   (vorbis_info *vi,
					    oggpack_buffer *opb);
extern int           _vorbis_map0_analysis (vorbis_block *vb,
					    _vi_info_map *i,ogg_packet *);
extern int           _vorbis_map0_synthesis(vorbis_block *vb,
					    _vi_info_map *i,ogg_packet *);

