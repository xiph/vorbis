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
 last modification date: Oct 12 1999

 ********************************************************************/

#ifndef _vorbis_codec_h_
#define _vorbis_codec_h_

#include <sys/types.h> /* get BSD style 16/32/64 bit types */

/* If we have defines from the configure script, use those */
#ifndef size64
#  define size64 int64_t 
#endif
#ifndef size32
#  define size32 int32_t 
#endif
#ifndef size16
#  define size16 int16_t
#endif

/* lookup structures for various simple transforms *****************/

typedef struct {
  int n;
  int log2n;
  
  double *trig;
  int    *bitrev;

} mdct_lookup;

typedef struct {
  int n;
  double *trigcache;
  int *splitcache;
} drft_lookup;

typedef struct {
  int winlen;
  double *window;
} envelope_lookup;

typedef struct lpclook{
  /* encode lookups */
  int *bscale;
  double *escale;
  drft_lookup fft;

  /* en/decode lookups */
  int *iscale;
  double *norm;
  int n;
  int ln;
  int m;

} lpc_lookup;

/* structures for various internal data abstractions ********************/

typedef struct {
  long endbyte;     
  int  endbit;      

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
  
} oggpack_buffer;

/* vobis_info contains all the setup information specific to the specific
   compression/decompression mode in progress (eg, psychoacoustic settings,
   channel setup, options, codebook etc) *********************************/

typedef struct vorbis_info{
  int channels;
  int rate;
  int version;

  char **user_comments;
  int    comments;
  char *vendor;

  int blocksize[2];
  int floororder[2];
  int flooroctaves[2];

  int envelopesa;
  int envelopech;
  int floorch;

  /*  int smallblock;
  int largeblock;
  int floororder;
  int flooroctaves;
  int floorch;*/

  /* no mapping, coupling, balance yet. */

  /*int balanceorder;
    int balanceoctaves; 
    this would likely need to be by channel and blocksize anyway*/

  double preecho_thresh;
  double preecho_clamp;

  /* local storage, only used on the encoding size.  This way the
     application does not need to worry about freeing some packets'
     memory and not others'.  Packet storage is always tracked */
  char *header;
  char *header1;
  char *header2;

} vorbis_info;
 
/* ogg_page is used to encapsulate the data in one Ogg bitstream page *****/

typedef struct {
  unsigned char *header;
  long header_len;
  unsigned char *body;
  long body_len;
} ogg_page;

/* ogg_stream_state contains the current encode/decode state of a logical
   Ogg bitstream **********************************************************/

typedef struct {
  unsigned char   *body_data;    /* bytes from packet bodies */
  long    body_storage;          /* storage elements allocated */
  long    body_fill;             /* elements stored; fill mark */
  long    body_returned;         /* elements of fill returned */


  int    *lacing_vals;    /* The values that will go to the segment table */
  int64_t *pcm_vals;      /* pcm_pos values for headers. Not compact
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
  int64_t  pcmpos;

} ogg_stream_state;

/* ogg_packet is used to encapsulate the data and metadata belonging
   to a single raw Ogg/Vorbis packet *************************************/

typedef struct {
  unsigned char *packet;
  long  bytes;
  long  b_o_s;
  long  e_o_s;

  int64_t frameno;

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

/* vorbis_dsp_state buffers the current vorbis audio
   analysis/synthesis state.  The DSP state belongs to a specific
   logical bitstream ****************************************************/

typedef struct vorbis_dsp_state{
  int analysisp;
  vorbis_info *vi;

  double *window[2][2][2]; /* windowsize, leadin, leadout */
  envelope_lookup ve;
  mdct_lookup vm[2];
  lpc_lookup vl[2];
  lpc_lookup vbal[2];

  double **pcm;
  double **pcmret;
  int      pcm_storage;
  int      pcm_current;
  int      pcm_returned;

  double **multipliers;
  int      envelope_storage;
  int      envelope_current;

  int  eofflag;

  long lW;
  long W;
  long nW;
  long centerW;

  long frame;
  long samples;

  size64 gluebits;
  size64 time_envelope_bits;
  size64 spectral_envelope_bits;
  size64 spectral_residue_bits;

} vorbis_dsp_state;

/* vorbis_block is a single block of data to be processed as part of
the analysis/synthesis stream; it belongs to a specific logical
bitstream, but is independant from other vorbis_blocks belonging to
that logical bitstream. *************************************************/

typedef struct vorbis_block{
  double **pcm;
  double **mult;
  double **lpc;
  double **lsp;
  double *amp;
  oggpack_buffer opb;
  
  int    pcm_channels;  /* allocated, not used */
  int    pcm_storage;   /* allocated, not used */
  int    mult_channels; /* allocated, not used */
  int    mult_storage;  /* allocated, not used */
  int    floor_channels;
  int    floor_storage;

  long  lW;
  long  W;
  long  nW;
  int   pcmend;
  int   multend;

  int eofflag;
  int frameno;
  vorbis_dsp_state *vd; /* For read-only access of configuration */

  long gluebits;
  long time_envelope_bits;
  long spectral_envelope_bits;
  long spectral_residue_bits;

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

extern int     ogg_page_version(ogg_page *og);
extern int     ogg_page_continued(ogg_page *og);
extern int     ogg_page_bos(ogg_page *og);
extern int     ogg_page_eos(ogg_page *og);
extern int64_t ogg_page_frameno(ogg_page *og);
extern int     ogg_page_serialno(ogg_page *og);
extern int     ogg_page_pageno(ogg_page *og);

/* Vorbis PRIMITIVES: general ***************************************/

extern void vorbis_dsp_clear(vorbis_dsp_state *v);

extern void vorbis_info_init(vorbis_info *vi); 
extern void vorbis_info_clear(vorbis_info *vi); 
extern int  vorbis_info_modeset(vorbis_info *vi, int mode); 
extern int  vorbis_info_addcomment(vorbis_info *vi, char *comment); 
extern int  vorbis_info_headerin(vorbis_info *vi,ogg_packet *op);
extern int  vorbis_info_headerout(vorbis_info *vi,
				  ogg_packet *op,
				  ogg_packet *op_comm,
				  ogg_packet *op_code);

extern int  vorbis_block_init(vorbis_dsp_state *v, vorbis_block *vb);
extern int  vorbis_block_clear(vorbis_block *vb);

/* Vorbis PRIMITIVES: analysis/DSP layer ****************************/
extern int      vorbis_analysis_init(vorbis_dsp_state *v,vorbis_info *vi);

extern double **vorbis_analysis_buffer(vorbis_dsp_state *v,int vals);
extern int      vorbis_analysis_wrote(vorbis_dsp_state *v,int vals);
extern int      vorbis_analysis_blockout(vorbis_dsp_state *v,vorbis_block *vb);
extern int      vorbis_analysis(vorbis_block *vb,ogg_packet *op);

/* Vorbis PRIMITIVES: synthesis layer *******************************/
extern int  vorbis_synthesis_init(vorbis_dsp_state *v,vorbis_info *vi);

extern int vorbis_synthesis(vorbis_block *vb,ogg_packet *op);
extern int vorbis_synthesis_blockin(vorbis_dsp_state *v,vorbis_block *vb);
extern int vorbis_synthesis_pcmout(vorbis_dsp_state *v,double ***pcm);
extern int vorbis_synthesis_read(vorbis_dsp_state *v,int samples);

#define min(x,y)  ((x)>(y)?(y):(x))
#define max(x,y)  ((x)<(y)?(y):(x))

  /* 20log10(x) */
#define todB(x)   ((x)==0?-9.e40:log(fabs(x))*8.6858896)
#define fromdB(x) (exp((x)*.11512925))

#endif

