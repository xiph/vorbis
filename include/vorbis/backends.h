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

 function: libvorbis backend and mapping structures; needed for 
           static mode headers
 last mod: $Id: backends.h,v 1.8 2000/05/08 20:49:42 xiphmont Exp $

 ********************************************************************/

/* this is exposed up here because we need it for static modes.
   Lookups for each backend aren't exposed because there's no reason
   to do so */

#ifndef _vorbis_time_backend_h_
#define _vorbis_time_backend_h_

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
  void (*free_info) (vorbis_info_time *);
  void (*free_look) (vorbis_look_time *);
  int  (*forward)   (struct vorbis_block *,vorbis_look_time *,
		     double *,double *);
  int  (*inverse)   (struct vorbis_block *,vorbis_look_time *,
		     double *,double *);
} vorbis_func_time;

typedef struct{
  int dummy;
} vorbis_info_time0;

/* Floor backend generic *****************************************/
typedef struct{
  void               (*pack)  (vorbis_info_floor *,oggpack_buffer *);
  vorbis_info_floor *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_floor *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
			       vorbis_info_floor *);
  void (*free_info) (vorbis_info_floor *);
  void (*free_look) (vorbis_look_floor *);
  int  (*forward)   (struct vorbis_block *,vorbis_look_floor *,
		     double *,double *);
  int  (*inverse)   (struct vorbis_block *,vorbis_look_floor *,
		     double *);
} vorbis_func_floor;

typedef struct{
  int   order;
  long  rate;
  long  barkmap;

  int   ampbits;
  int   ampdB;

  int   numbooks; /* <= 16 */
  int   books[16];
} vorbis_info_floor0;

/* Residue backend generic *****************************************/
typedef struct{
  void                 (*pack)  (vorbis_info_residue *,oggpack_buffer *);
  vorbis_info_residue *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_residue *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				 vorbis_info_residue *);
  void (*free_info)    (vorbis_info_residue *);
  void (*free_look)    (vorbis_look_residue *);
  int  (*forward)      (struct vorbis_block *,vorbis_look_residue *,
			double **,int);
  int  (*inverse)      (struct vorbis_block *,vorbis_look_residue *,
			double **,int);
} vorbis_func_residue;

typedef struct vorbis_info_residue0{
/* block-partitioned VQ coded straight residue */
  long  begin;
  long  end;

  /* first stage (lossless partitioning) */
  int    grouping;         /* group n vectors per partition */
  int    partitions;       /* possible codebooks ofr a partition */
  int    groupbook;        /* huffbook for partitioning */

  double ampmax[64];       /* book amp threshholds for *encode* */
  int    secondstages[64]; /* expanded out to pointers in lookup */
  int    booklist[256];    /* list of second stage books */

} vorbis_info_residue0;

/* Mapping backend generic *****************************************/
typedef struct{
  void                 (*pack)  (vorbis_info *,vorbis_info_mapping *,
				 oggpack_buffer *);
  vorbis_info_mapping *(*unpack)(vorbis_info *,oggpack_buffer *);
  vorbis_look_mapping *(*look)  (vorbis_dsp_state *,vorbis_info_mode *,
				 vorbis_info_mapping *);
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
  int   psysubmap[16];     /* [mux]; encode only */
} vorbis_info_mapping0;

#endif





