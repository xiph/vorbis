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
 last mod: $Id: backends.h,v 1.1 2000/01/22 10:40:36 xiphmont Exp $

 ********************************************************************/

/* this is exposed up here because we need it for static modes.
   Lookups for each backend aren't exposed because there's no reason
   to do so */

#ifndef _vorbis_time_backend_h_
#define _vorbis_time_backend_h_

/* this would all be simpler/shorter with templates, but.... */
/* mode setup ******************************************************/
typedef struct {
  int blockflag;
  int windowtype;
  int transformtype;
  int mapping;
} vorbis_info_mode;

/* Transform backend generic *************************************/
typedef void vorbis_look_transform;

typedef struct{
  void (*forward)(vorbis_look_transform *,double *in,double *out);
  void (*inverse)(vorbis_look_transform *,double *in,double *out);
} vorbis_func_transform;

/* Time backend generic ******************************************/

typedef void vorbis_info_time;
typedef void vorbis_look_time;

typedef struct{
  void              (*pack)  (vorbis_info_time *,oggpack_buffer *);
  vorbis_info_time *(*unpack)(oggpack_buffer *);
  vorbis_look_time *(*look)  (vorbis_info *,vorbis_info_mode *,
			      vorbis_info_time *);
  void (*free_info) (vorbis_info_time *);
  void (*free_look) (vorbis_look_time *);
  void (*forward)   (vorbis_info *,vorbis_look_time *,double *,double *);
  void (*inverse)   (vorbis_info *,vorbis_look_time *,double *,double *);
} vorbis_func_time;

typedef struct{
  int dummy;
} vorbis_info_time0;

/* Floor backend generic *****************************************/

typedef void vorbis_info_floor;
typedef void vorbis_look_floor;

typedef struct{
  void               (*pack)  (vorbis_info_floor *,oggpack_buffer *);
  vorbis_info_floor *(*unpack)(oggpack_buffer *);
  vorbis_look_floor *(*look)  (vorbis_info *,vorbis_info_mode *,
			       vorbis_info_floor *);
  void (*free_info) (vorbis_info_floor *);
  void (*free_look) (vorbis_look_floor *);
  void (*forward)   (vorbis_info *,vorbis_look_floor *,double *,double *);
  void (*inverse)   (vorbis_info *,vorbis_look_floor *,double *,double *);
} vorbis_func_floor;

typedef struct{
  int   order;
  long  rate;
  long  barkmap;
  int   stages;
  int  *books;
} vorbis_info_floor0;

/* Residue backend generic *****************************************/
typedef void vorbis_info_residue;
typedef void vorbis_look_residue;

typedef struct{
  void                 (*pack)  (vorbis_info_residue *,oggpack_buffer *);
  vorbis_info_residue *(*unpack)(oggpack_buffer *);
  vorbis_look_residue *(*look)  (vorbis_info *,vorbis_info_mode *,
				 vorbis_info_residue *);
  void (*free_info)    (vorbis_info_residue *);
  void (*free_look)    (vorbis_look_residue *);
  void (*forward)      (vorbis_info *,vorbis_look_residue *,double *,double *);
  void (*inverse)      (vorbis_info *,vorbis_look_residue *,double *,double *);
} vorbis_func_residue;

typedef struct vorbis_info_res0{
/* block-partitioned VQ coded straight residue */
  long  begin;
  long  end;

  /* way unfinished, just so you know while poking around CVS ;-) */
  int   stages;
  int  *books;
} vorbis_info_res0;

/* psychoacoustic setup ********************************************/
typedef struct vorbis_info_psy{
  double maskthresh[MAX_BARK];
  double lrolldB;
  double hrolldB;
} vorbis_info_psy;

/* Mapping backend generic *****************************************/
typedef void vorbis_info_mapping;
typedef void vorbis_look_mapping;

typedef struct{
  void                 (*pack)  (vorbis_info_mapping *,oggpack_buffer *);
  vorbis_info_mapping *(*unpack)(oggpack_buffer *);
  vorbis_look_mapping *(*look)  (vorbis_info *,vorbis_info_mode *,
				 vorbis_info_mapping *);
  void (*free_info)    (vorbis_info_mapping *);
  void (*free_look)    (vorbis_look_mapping *);
  void (*forward)      (int mode,struct vorbis_block *vb);
  void (*inverse)      (int mode,struct vorbis_block *vb);
} vorbis_func_residue;

typedef struct vorbis_info_mapping0{
  int    submaps;
  int   *chmuxlist;
  
  int   *timesubmap;    /* [mux] */
  int   *floorsubmap;   /* [mux] submap to floors */
  int   *residuesubmap; /* [mux] submap to residue */
  int   *psysubmap;     /* [mux]; encode only */
} vorbis_info_mapping0;

#endif





