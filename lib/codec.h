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

 function: PCM data vector blocking, windowing and dis/reassembly
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Jul 13 1999

 ********************************************************************/

#ifndef _vorbis_codec_h_
#define _vorbis_codec_h_

typedef struct vorbis_info{
  int channels;
  int rate;
  int version;
  int mode;
  char **user_comments;
  char *vendor;

} vorbis_info;
 
typedef struct vorbis_dsp_state{
  int samples_per_envelope_step;
  int block_size[2];
  double *window[2][2][2]; /* windowsize, leadin, leadout */

  double **pcm;
  int      pcm_storage;
  int      pcm_channels;
  int      pcm_current;

  double **deltas;
  int    **multipliers;
  int      envelope_storage;
  int      envelope_channels;
  int      envelope_current;

  int  initflag;

  long lW;
  long W;
  long Sl;
  long Sr;

  long beginW;
  long endW;
  long beginSl;
  long endSl;
  long beginSr;
  long endSr;

  long frame;
  long samples;

} vorbis_dsp_state;

typedef struct {
  unsigned char *header;
  long header_len;
  unsigned char *body;
  long body_len;
} vorbis_page;

typedef struct {
 
  /*             _________________________________________________
     body_data: |_________________________________________________|
     body_returned ----^       ^                          ^       ^
     body_processed------------'                          |       |
     body_fill     ---------------------------------------'       |     
     body_storage  _______________________________________________' 

     the header is labelled the same way.  Not all the pointers are 
     used by both encode and decode */

  unsigned char   *body_data;    /* bytes from packet bodies */
  long    body_storage;          /* storage elements allocated */
  long    body_fill;             /* elements stored; fill mark */
  long    body_returned;         /* elements of fill returned */


  int    *lacing_vals;    /* The values that will go to the segment table */
  size64 *pcm_vals;       /* pcm_pos values for headers. Not compact
			     this way, but it is simple coupled to the
			     lacing fifo */
  long    lacing_storage;
  long    lacing_fill;
  long    lacing_packet;
  long    lacing_returned;

  unsigned char    header[282];      /* working space for header encode */
  int              header_fill;

  int     e_o_s;          /* set when we have buffered the last packet in the
			     logical bitstream */
  int     b_o_s;          /* set after we've written the initial page
			     of a logical bitstream */
  long    serialno;
  long    pageno;
  size64  pcmpos;

} vorbis_stream_state;

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  size64 pcm_pos;

} vorbis_packet;

typedef struct {
  char *data;
  int storage;
  int fill;
  int returned;

  int unsynced;
  int headerbytes;
  int bodybytes;
} vorbis_sync_state;

/* libvorbis encodes in two abstraction layers; first we perform DSP
   and produce a packet (see docs/analysis.txt).  The packet is then
   coded into a framed bitstream by the second layer (see
   docs/framing.txt).  Decode is the reverse process; we sync/frame
   the bitstream and extract individual packets, then decode the
   packet back into PCM audio.

   The extra framing/packetizing is used in streaming formats, such as
   files.  Over the net (such as with UDP), the framing and
   packetization aren't necessary as they're provided by the transport
   and the streaming layer is not used */

/* ENCODING PRIMITIVES: analysis/DSP layer **************************/

extern int vorbis_analysis_init(vorbis_dsp_state *vd,vorbis_info *i);
extern void vorbis_dsp_state_free(vorbis_dsp_state *vd);
extern int vorbis_analysis_input(vorbis_dsp_state *vd,double **pcm,int vals);
extern int vorbis_analysis(vorbis_dsp_state *vd,vorbis_packet *vp);

/* GENERAL PRIMITIVES: packet streaming layer ***********************/

extern int vorbis_stream_init(vorbis_stream_state *vs,int serialno);
extern int vorbis_stream_clear(vorbis_stream_state *vs);
extern int vorbis_stream_destroy(vorbis_stream_state *vs);
extern int vorbis_stream_eof(vorbis_stream_state *vs);

extern int vorbis_page_version(vorbis_page *vg);
extern int vorbis_page_continued(vorbis_page *vg);
extern int vorbis_page_bos(vorbis_page *vg);
extern int vorbis_page_eos(vorbis_page *vg);
extern size64 vorbis_page_pcmpos(vorbis_page *vg);
extern int vorbis_page_serialno(vorbis_page *vg);
extern int vorbis_page_pageno(vorbis_page *vg);

/* ENCODING PRIMITIVES: packet streaming layer **********************/

extern int vorbis_stream_encode(vorbis_stream_state *vs,vorbis_packet *vp);
extern int vorbis_stream_page(vorbis_stream_state *vs, vorbis_page *vg);

/* DECODING PRIMITIVES: packet streaming layer **********************/

extern int vorbis_sync_init(vorbis_sync_state *vs);
extern int vorbis_sync_clear(vorbis_sync_state *vs);
extern int vorbis_sync_destroy(vorbis_sync_state *vs);

extern char *vorbis_decode_buffer(vorbis_sync_state *vs, long size);
extern int vorbis_decode_wrote(vorbis_sync_state *vs, long bytes);
extern int vorbis_decode_stream(vorbis_sync_state *vs, vorbis_page *vg);
extern int vorbis_decode_page(vorbis_stream_state *vs, vorbis_page *vg);
extern int vorbis_stream_packet(vorbis_stream_state *vs,vorbis_packet *vp);

extern int vorbis_sync_reset(vorbis_sync_state *vs);
extern int vorbis_stream_reset(vorbis_stream_state *vs);

/* DECODING PRIMITIVES: synthesis layer *****************************/

extern int vorbis_synthesis_info(vorbis_dsp_state *vd,vorbis_info *vi);
extern int vorbis_synthesis(vorbis_dsp_state *vd,vorbis_packet *vp);
extern int vorbis_synthesis_output(vorbis_dsp_state *vd,double **pcm);

#endif

