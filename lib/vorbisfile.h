/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and The XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: stdio-based convenience library for opening/seeking/decoding
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Nov 02 1999

 ********************************************************************/

#ifndef _VO_FILE_H_
#define _VO_FILE_H_

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
  long             *serialnos;
  size64           *pcmlengths;
  vorbis_info      *vi;

  /* Decoding working state local storage */
  int ready;
  ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

} OggVorbis_File;

extern int ov_clear(OggVorbis_File *vf);
extern int ov_open(FILE *f,OggVorbis_File *vf,char *initial,long ibytes);

extern double ov_lbtime(OggVorbis_File *vf,int i);
extern double ov_totaltime(OggVorbis_File *vf);


#endif







