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
 last modification date: Jun 26 1999

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
  unsigned size32 crc_lookup[256];

  unsigned char   *body_data;    /* bytes from packet bodies */
  long    body_storage;
  long    body_fill;
  long    body_processed;

  int    *lacing_vals;    /* The values that will go to the segment table */
  size64 *pcm_vals;       /* pcm_pos values for headers. Not compact
			     this way, but it is simple coupled to the
			     lacing fifo */
  long    lacing_storage;
  long    lacing_fill;

  unsigned char    header[282];    /* working space for header */
  int              headerbytes;    

  int     e_o_s;          /* set when we have buffered the last packet in the
			     logical bitstream */
  int     b_o_s;          /* set after we've written the initial page
			     of a logical bitstream */
  long    serialno;
  long    pageno;
} vorbis_stream_state;

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  e_o_s;

  size64 pcm_pos;

} vorbis_packet;

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

/* ENCODING PRIMITIVES: packet streaming layer **********************/

extern int vorbis_stream_encode(vorbis_stream_state *vs,vorbis_packet *vp);
extern int vorbis_stream_page(vorbis_stream_state *vs, vorbis_page *vg);

/* DECODING PRIMITIVES: packet streaming layer **********************/

/* returns nonzero when it has a complete vorbis packet synced and
   framed for decoding. Generally, it wants to see two page headers at
   proper spacing before returning 'yea'. An exception is the initial
   header to make IDing the vorbis stream easier: it will return sync
   on the page header + beginning-of-stream marker.

   _sync will also abort framing if is sees a problem in the packet
   boundary lacing.  */

extern int vorbis_stream_decode(vorbis_stream_state *vs,char *stream,int size);
extern int vorbis_stream_sync(vorbis_stream_state *vs);
extern int vorbis_stream_verify(vorbis_stream_state *vs);
extern size64 vorbis_stream_position(vorbis_stream_state *vs);
extern int vorbis_stream_skippage(vorbis_stream_state *vs);
extern int vorbis_stream_packet(vorbis_stream_state *vs,vorbis_packet *vp);

/* DECODING PRIMITIVES: synthesis layer *****************************/

extern int vorbis_synthesis_info(vorbis_dsp_state *vd,vorbis_info *vi);
extern int vorbis_synthesis(vorbis_dsp_state *vd,vorbis_packet *vp);
extern int vorbis_synthesis_output(vorbis_dsp_state *vd,double **pcm);

#endif

