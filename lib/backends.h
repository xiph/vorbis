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

 function: libvorbis backend and mapping structures; needed for 
           static mode headers
 last mod: $Id: backends.h,v 1.12 2001/12/20 01:00:26 segher Exp $

 ********************************************************************/

/* this is exposed up here because we need it for static modes.
   Lookups for each backend aren't exposed because there's no reason
   to do so */

#ifndef _vorbis_backend_h_
#define _vorbis_backend_h_

#include "codec_internal.h"

/* this would all be simpler/shorter with templates, but.... */
/* Transform backend generic *************************************/

/* only mdct right now.  Flesh it out more if we ever transcend mdct
   in the transform domain */

/* Time backend generic ******************************************/
typedef struct{
  void              (*pack)  (vorbis_info_time *,oggpack_buffer *);
  vorbis_info_time *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_time *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
			      vorbis_info_time *);
  vorbis_info_time *(*copy_info)(vorbis_info_time *);

  void (*free_info) (vorbis_info_time *);
  void (*free_look) (vorbis_look_time *);
  int  (*forward)   (struct vorbis_block *,vorbis_look_time *,
		     float *,float *);
  int  (*inverse)   (struct vorbis_block *,vorbis_look_time *,
		     float *,float *);
} vorbis_func_time;

typedef struct{
  int dummy;
} vorbis_info_time0;

/* Floor backend generic *****************************************/
typedef struct{
  void                   (*pack)  (vorbis_info_floor *,oggpack_buffer *);
  vorbis_info_floor     *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_floor     *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				   vorbis_info_floor *);
  vorbis_info_floor     *(*copy_info)(vorbis_info_floor *);
  void (*free_info) (vorbis_info_floor *);
  void (*free_look) (vorbis_look_floor *);
  int  (*forward)   (struct vorbis_block *,vorbis_look_floor *,
		     float *, const float *, /* in */
		     const float *, const float *, /* in */
		     float *);                     /* out */
  void *(*inverse1)  (struct vorbis_block *,vorbis_look_floor *);
  int   (*inverse2)  (struct vorbis_block *,vorbis_look_floor *,
		     void *buffer,float *);
} vorbis_func_floor;

typedef struct{
  int   order;
  long  rate;
  long  barkmap;

  int   ampbits;
  int   ampdB;

  int   numbooks; /* <= 16 */
  int   books[16];

  float lessthan;     /* encode-only config setting hacks for libvorbis */
  float greaterthan;  /* encode-only config setting hacks for libvorbis */

} vorbis_info_floor0;

#define VIF_POSIT 63
#define VIF_CLASS 16
#define VIF_PARTS 31
typedef struct{
  int   partitions;                /* 0 to 31 */
  int   partitionclass[VIF_PARTS]; /* 0 to 15 */

  int   class_dim[VIF_CLASS];        /* 1 to 8 */
  int   class_subs[VIF_CLASS];       /* 0,1,2,3 (bits: 1<<n poss) */
  int   class_book[VIF_CLASS];       /* subs ^ dim entries */
  int   class_subbook[VIF_CLASS][8]; /* [VIF_CLASS][subs] */


  int   mult;                      /* 1 2 3 or 4 */ 
  int   postlist[VIF_POSIT+2];    /* first two implicit */ 


  /* encode side analysis parameters */
  float maxover;     
  float maxunder;  
  float maxerr;    

  int   twofitminsize;
  int   twofitminused;
  int   twofitweight;  
  float twofitatten;
  int   unusedminsize;
  int   unusedmin_n;

  int   n;

} vorbis_info_floor1;

/* Residue backend generic *****************************************/
typedef struct{
  void                 (*pack)  (vorbis_info_residue *,oggpack_buffer *);
  vorbis_info_residue *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_residue *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				 vorbis_info_residue *);
  vorbis_info_residue *(*copy_info)(vorbis_info_residue *);
  void (*free_info)    (vorbis_info_residue *);
  void (*free_look)    (vorbis_look_residue *);
  long **(*class)      (struct vorbis_block *,vorbis_look_residue *,
			float **,int *,int);
  int  (*forward)      (struct vorbis_block *,vorbis_look_residue *,
			float **,float **,int *,int,int,long **,ogg_uint32_t *);
  int  (*inverse)      (struct vorbis_block *,vorbis_look_residue *,
			float **,int *,int);
} vorbis_func_residue;

typedef struct vorbis_info_residue0{
/* block-partitioned VQ coded straight residue */
  long  begin;
  long  end;

  /* first stage (lossless partitioning) */
  int    grouping;         /* group n vectors per partition */
  int    partitions;       /* possible codebooks for a partition */
  int    groupbook;        /* huffbook for partitioning */
  int    secondstages[64]; /* expanded out to pointers in lookup */
  int    booklist[256];    /* list of second stage books */

  /* encode-only heuristic settings */
  float  entmax[64];       /* book entropy threshholds*/
  float  ampmax[64];       /* book amp threshholds*/
  int    subgrp[64];       /* book heuristic subgroup size */
  int    blimit[64];       /* subgroup position limits */
} vorbis_info_residue0;

/* Mapping backend generic *****************************************/
typedef struct{
  void                 (*pack)  (vorbis_info *,vorbis_info_mapping *,
				 oggpack_buffer *);
  vorbis_info_mapping *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_mapping *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				 vorbis_info_mapping *);
  vorbis_info_mapping *(*copy_info)(vorbis_info_mapping *);
  void (*free_info)    (vorbis_info_mapping *);
  void (*free_look)    (vorbis_look_mapping *);
  int  (*forward)      (struct vorbis_block *vb,vorbis_look_mapping *);
  int  (*inverse)      (struct vorbis_block *vb,vorbis_look_mapping *);
} vorbis_func_mapping;

typedef struct vorbis_info_mapping0{
  int   submaps;  /* <= 16 */
  int   chmuxlist[256];   /* up to 256 channels in a Vorbis stream */
  
  int   timesubmap[16];    /* [mux] */
  int   floorsubmap[16];   /* [mux] submap to floors */
  int   residuesubmap[16]; /* [mux] submap to residue */

  int   psy[2]; /* by blocktype; impulse/padding for short,
                   transition/normal for long */

  int   coupling_steps;
  int   coupling_mag[256];
  int   coupling_ang[256];
} vorbis_info_mapping0;

#endif





