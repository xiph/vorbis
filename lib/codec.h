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

 function: codec headers
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Jul 28 1999

 ********************************************************************/

#ifndef _vorbis_codec_h_
#define _vorbis_codec_h_

typedef struct {
  long endbyte;     
  int  endbit;      

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
  
} oggpack_buffer;

typedef struct vorbis_info{
  int channels;
  int rate;
  int version;
  int mode;
  char **user_comments;
  char *vendor;

  int smallblock;
  int largeblock;
  int envelopesa;
  int envelopech;
  int *envelopemap; /* which envelope applies to what pcm channel */
  int **channelmap; /* which encoding channels produce what pcm */

  double preecho_thresh;
  double preecho_clamp;
} vorbis_info;
 
typedef struct {
  unsigned char *header;
  long header_len;
  unsigned char *body;
  long body_len;
} ogg_page;

typedef struct {
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

} ogg_stream_state;

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  size64 frameno;

} ogg_packet;

typedef struct {
  unsigned char *data;
  int storage;
  int fill;
  int returned;

  int unsynced;
  int headerbytes;
  int bodybytes;
} ogg_sync_state;

typedef struct {
  int winlen;
  double *window;
} envelope_lookup;

typedef struct vorbis_dsp_state{
  int samples_per_envelope_step;
  int block_size[2];
  double *window[2][2][2]; /* windowsize, leadin, leadout */

  envelope_lookup ve;
  vorbis_info vi;

  double **pcm;
  double **pcmret;
  int      pcm_storage;
  int      pcm_channels;
  int      pcm_current;
  int      pcm_returned;

  double **multipliers;
  int      envelope_storage;
  int      envelope_channels;
  int      envelope_current;

  int  eofflag;

  long lW;
  long W;
  long nW;
  long centerW;

  long frame;
  long samples;

} vorbis_dsp_state;

typedef struct vorbis_block{
  double **pcm;
  double **mult;
  int    pcm_channels; /* allocated, not used */
  int    pcm_storage;  /* allocated, not used */
  int    mult_channels; /* allocated, not used */
  int    mult_storage;  /* allocated, not used */

  long  lW;
  long  W;
  long  nW;
  int   pcmend;
  int   multend;

  int eofflag;
  int frameno;
  vorbis_dsp_state *vd; /* For read-only access of configuration */
} vorbis_block;

/* libvorbis encodes in two abstraction layers; first we perform DSP
   and produce a packet (see docs/analysis.txt).  The packet is then
   coded into a framed OggSquish bitstream by the second layer (see
   docs/framing.txt).  Decode is the reverse process; we sync/frame
   the bitstream and extract individual packets, then decode the
   packet back into PCM audio.

   The extra framing/packetizing is used in streaming formats, such as
   files.  Over the net (such as with UDP), the framing and
   packetization aren't necessary as they're provided by the transport
   and the streaming layer is not used */

/* OggSquish BITSREAM PRIMITIVES: encoding **************************/

extern int    ogg_stream_packetin(ogg_stream_state *os, ogg_packet *op);
extern int    ogg_stream_pageout(ogg_stream_state *os, ogg_page *og);

/* OggSquish BITSREAM PRIMITIVES: decoding **************************/

extern int    ogg_sync_init(ogg_sync_state *oy);
extern int    ogg_sync_clear(ogg_sync_state *oy);
extern int    ogg_sync_destroy(ogg_sync_state *oy);
extern int    ogg_sync_reset(ogg_sync_state *oy);

extern char  *ogg_sync_buffer(ogg_sync_state *oy, long size);
extern int    ogg_sync_wrote(ogg_sync_state *oy, long bytes);
extern int    ogg_sync_pageout(ogg_sync_state *oy, ogg_page *og);
extern int    ogg_stream_pagein(ogg_stream_state *os, ogg_page *og);
extern int    ogg_stream_packetout(ogg_stream_state *os,ogg_packet *op);

/* OggSquish BITSREAM PRIMITIVES: general ***************************/

extern int    ogg_stream_init(ogg_stream_state *os,int serialno);
extern int    ogg_stream_clear(ogg_stream_state *os);
extern int    ogg_stream_reset(ogg_stream_state *os);
extern int    ogg_stream_destroy(ogg_stream_state *os);
extern int    ogg_stream_eof(ogg_stream_state *os);

extern int    ogg_page_version(ogg_page *og);
extern int    ogg_page_continued(ogg_page *og);
extern int    ogg_page_bos(ogg_page *og);
extern int    ogg_page_eos(ogg_page *og);
extern size64 ogg_page_frameno(ogg_page *og);
extern int    ogg_page_serialno(ogg_page *og);
extern int    ogg_page_pageno(ogg_page *og);

/* Vorbis PRIMITIVES: analysis/DSP layer ****************************/

extern int      vorbis_analysis_init(vorbis_dsp_state *vd,vorbis_info *vi);
extern int      vorbis_analysis_reset(vorbis_dsp_state *vd);
extern void     vorbis_analysis_free(vorbis_dsp_state *vd);
extern double **vorbis_analysis_buffer(vorbis_dsp_state *vd,int vals);
extern int      vorbis_analysis_wrote(vorbis_dsp_state *vd,int vals);
extern int      vorbis_analysis_blockout(vorbis_dsp_state *vd,
					 vorbis_block *vb);
extern int      vorbis_analysis_packetout(vorbis_dsp_state *vd,
					  vorbis_block *vb,
					  ogg_packet *op);

/* Vorbis PRIMITIVES: synthesis layer *******************************/

extern void vorbis_synthesis_free(vorbis_dsp_state *vd);
extern int  vorbis_synthesis_init(vorbis_dsp_state *vd,vorbis_info *vi);

extern int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb);
extern int vorbis_synthesis_pcmout(vorbis_dsp_state *v,double ***pcm);
extern int vorbis_synthesis_read(vorbis_dsp_state *v,int bytes);



#endif

