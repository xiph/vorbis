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

 function: stdio-based convenience library for opening/seeking/decoding
 last mod: $Id: vorbisfile.h,v 1.2.4.1 2000/03/30 01:46:47 xiphmont Exp $

 ********************************************************************/

#ifndef _OV_FILE_H_
#define _OV_FILE_H_

#include <stdio.h>
#include "codec.h"

typedef struct {
  FILE             *f;
  int              seekable;
  long             offset;
  long             end;
  ogg_sync_state   oy; 

  /* If the FILE handle isn't seekable (eg, a pipe), only the current
     stream appears */
  int              links;
  long             *offsets;
  long             *dataoffsets;
  long             *serialnos;
  int64_t           *pcmlengths;
  vorbis_info      *vi;
  vorbis_comment  *vc;

  /* Decoding working state local storage */
  int64_t          pcm_offset;
  int              decode_ready;
  long             current_serialno;
  int              current_link;

  ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

} OggVorbis_File;

extern int ov_clear(OggVorbis_File *vf);
extern int ov_open(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);

extern long ov_bitrate(OggVorbis_File *vf,int i);
extern long ov_streams(OggVorbis_File *vf);
extern long ov_seekable(OggVorbis_File *vf);
extern long ov_serialnumber(OggVorbis_File *vf,int i);

extern long ov_raw_total(OggVorbis_File *vf,int i);
extern int64_t ov_pcm_total(OggVorbis_File *vf,int i);
extern double ov_time_total(OggVorbis_File *vf,int i);

extern int ov_raw_seek(OggVorbis_File *vf,long pos);
extern int ov_pcm_seek(OggVorbis_File *vf,int64_t pos);
extern int ov_time_seek(OggVorbis_File *vf,double pos);

extern long ov_raw_tell(OggVorbis_File *vf);
extern int64_t ov_pcm_tell(OggVorbis_File *vf);
extern double ov_time_tell(OggVorbis_File *vf);

extern vorbis_info *ov_info(OggVorbis_File *vf,int link);
extern vorbis_comment *ov_comment(OggVorbis_File *vf,int link);

extern long ov_read(OggVorbis_File *vf,char *buffer,int length,
		    int bigendianp,int word,int sgned,int *bitstream);

#endif







