/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty <monty@xiph.org> and the XIPHOPHORUS Company       *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: code raw Vorbis packets into framed vorbis stream and
           decode vorbis streams back into raw packets
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Jul 13 1999

 note: The CRC code is directly derived from public domain code by
 Ross Williams (ross@guest.adelaide.edu.au).  See framing.txt for
 details.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "codec.h"

/* A complete description of Vorbis framing exists in docs/framing.txt */

int vorbis_page_version(vorbis_page *vg){
  return((int)(vg->header[4]));
}

int vorbis_page_continued(vorbis_page *vg){
  return((int)(vg->header[5]&0x01));
}

int vorbis_page_bos(vorbis_page *vg){
  return((int)(vg->header[5]&0x02));
}

int vorbis_page_eos(vorbis_page *vg){
  return((int)(vg->header[5]&0x04));
}

size64 vorbis_page_pcmpos(vorbis_page *vg){
  char *page=vg->header;
  size64 pcmpos=page[13];
  pcmpos= (pcmpos<<8)|page[12];
  pcmpos= (pcmpos<<8)|page[11];
  pcmpos= (pcmpos<<8)|page[10];
  pcmpos= (pcmpos<<8)|page[9];
  pcmpos= (pcmpos<<8)|page[8];
  pcmpos= (pcmpos<<8)|page[7];
  pcmpos= (pcmpos<<8)|page[6];
  return(pcmpos);
}

int vorbis_page_serialno(vorbis_page *vg){
  return(vg->header[14] |
	 (vg->header[15]<<8) |
	 (vg->header[16]<<16) |
	 (vg->header[17]<<24));
}
 
int vorbis_page_pageno(vorbis_page *vg){
  return(vg->header[18] |
	 (vg->header[19]<<8) |
	 (vg->header[20]<<16) |
	 (vg->header[21]<<24));
}

/* helper to initialize lookup for direct-table CRC */

static unsigned size32 crc_lookup[256];
static int crc_ready=0;

static unsigned size32 _vorbis_crc_entry(unsigned long index){
  int           i;
  unsigned long r;

  r = index << 24;
  for (i=0; i<8; i++)
    if (r & 0x80000000UL)
      r = (r << 1) ^ 0x04c11db7; /* The same as the ethernet generator
				    polynomial, although we use an
				    unreflected alg and an init/final
				    of 0, not 0xffffffff */
    else
       r<<=1;
 return (r & 0xffffffffUL);
}

/* mind this in threaded code; sync_init and stream_init call it.
   It's thread safe only after the first time it returns */

static void _vorbis_crc_init(void){
  if(!crc_ready){
    /* initialize the crc_lookup table */
    int i;
    for (i=0;i<256;i++)
      crc_lookup[i]=_vorbis_crc_entry((unsigned long)i);
    crc_ready=0;
  }
}

/* init the encode/decode logical stream state */

int vorbis_stream_init(vorbis_stream_state *vs,int serialno){
  if(vs){
    memset(vs,0,sizeof(vorbis_stream_state));
    vs->body_storage=16*1024;
    vs->body_data=malloc(vs->body_storage*sizeof(char));

    vs->lacing_storage=1024;
    vs->lacing_vals=malloc(vs->lacing_storage*sizeof(int));
    vs->pcm_vals=malloc(vs->lacing_storage*sizeof(size64));

    /* initialize the crc_lookup table if not done */
    _vorbis_crc_init();

    vs->serialno=serialno;

    return(0);
  }
  return(-1);
} 

/* _clear does not free vs, only the non-flat storage within */
int vorbis_stream_clear(vorbis_stream_state *vs){
  if(vs){
    if(vs->body_data)free(vs->body_data);
    if(vs->lacing_vals)free(vs->lacing_vals);
    if(vs->pcm_vals)free(vs->pcm_vals);

    memset(vs,0,sizeof(vorbis_stream_state));    
  }
  return(0);
} 

int vorbis_stream_destroy(vorbis_stream_state *vs){
  if(vs){
    vorbis_stream_clear(vs);
    free(vs);
  }
  return(0);
} 

/* Helpers for vorbis_stream_encode; this keeps the structure and
   what's happening fairly clear */

static void _vs_body_expand(vorbis_stream_state *vs,int needed){
  if(vs->body_storage<=vs->body_fill+needed){
    vs->body_storage+=(needed+1024);
    vs->body_data=realloc(vs->body_data,vs->body_storage);
  }
}

static void _vs_lacing_expand(vorbis_stream_state *vs,int needed){
  if(vs->lacing_storage<=vs->lacing_fill+needed){
    vs->lacing_storage+=(needed+32);
    vs->lacing_vals=realloc(vs->lacing_vals,vs->lacing_storage*sizeof(int));
    vs->pcm_vals=realloc(vs->pcm_vals,vs->lacing_storage*sizeof(size64));
  }
}

/* checksum the page */
/* Direct table CRC; note that this will be faster in the future if we
   perform the checksum silmultaneously with other copies */

static void _vs_checksum(vorbis_page *vg){
  unsigned size32 crc_reg=0;
  int i;

  for(i=0;i<vg->header_len;i++)
    crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)&0xff)^vg->header[i]];
  for(i=0;i<vg->body_len;i++)
    crc_reg=(crc_reg<<8)^crc_lookup[((crc_reg >> 24)&0xff)^vg->body[i]];
  
  vg->header[22]=crc_reg&0xff;
  vg->header[23]=(crc_reg>>8)&0xff;
  vg->header[24]=(crc_reg>>16)&0xff;
  vg->header[25]=(crc_reg>>24)&0xff;
}

/* submit data to the internal buffer of the framing engine */
int vorbis_stream_encode(vorbis_stream_state *vs,vorbis_packet *vp){
  int lacing_vals=vp->bytes/255+1,i;

  /* make sure we have the buffer storage */
  _vs_body_expand(vs,vp->bytes);
  _vs_lacing_expand(vs,lacing_vals);

  /* Copy in the submitted packet.  Yes, the copy is a waste; this is
     the liability of overly clean abstraction for the time being.  It
     will actually be fairly easy to eliminate the extra copy in the
     future */

  memcpy(vs->body_data+vs->body_fill,vp->packet,vp->bytes);
  vs->body_fill+=vp->bytes;

  /* Store lacing vals for this packet */
  for(i=0;i<lacing_vals-1;i++){
    vs->lacing_vals[vs->lacing_fill+i]=255;
    vs->pcm_vals[vs->lacing_fill+i]=vp->pcm_pos;
  }
  vs->lacing_vals[vs->lacing_fill+i]=(vp->bytes)%255;
  vs->pcm_vals[vs->lacing_fill+i]=vp->pcm_pos;

  /* flag the first segment as the beginning of the packet */
  vs->lacing_vals[vs->lacing_fill]|= 0x100;

  vs->lacing_fill+=lacing_vals;

  if(vp->e_o_s)vs->e_o_s=1;

  return(0);
}

/* This constructs pages from buffered packet segments.  The pointers
returned are to static buffers; do not free. The returned buffers are
good only until the next call (using the same vs) */

int vorbis_stream_page(vorbis_stream_state *vs, vorbis_page *vg){
  int i;

  if(vs->body_returned){
    /* advance packet data according to the body_returned pointer. We
       had to keep it around to return a pointer into the buffer last
       call */

    vs->body_fill-=vs->body_returned;
    memmove(vs->body_data,vs->body_data+vs->body_returned,
	    (vs->body_fill-vs->body_returned)*sizeof(char));
    vs->body_returned=0;
  }

  if((vs->e_o_s&&vs->lacing_fill) || 
     vs->body_fill > 4096 || 
     vs->lacing_fill>=255){
    int vals=0,bytes=0;
    int maxvals=(vs->lacing_fill>255?255:vs->lacing_fill);
    long acc=0;
    size64 pcm_pos=vs->pcm_vals[0];

    /* construct a page */
    /* decide how many segments to include */
    for(vals=0;vals<maxvals;vals++){
      if(acc>4096)break;
      acc+=vs->lacing_vals[vals]&0x0ff;
      if((vs->lacing_vals[vals]&0x0ff)<255)pcm_pos=vs->pcm_vals[vals];
    }

    /* construct the header in temp storage */
    memcpy(vs->header,"OggS",4);

    /* stream structure version */
    vs->header[4]=0x00;

    
    vs->header[5]=0x00;
    /* continued packet flag? */
    if((vs->lacing_vals[0]&0x100)==0)vs->header[5]|=0x01;
    /* first page flag? */
    if(vs->b_o_s==0)vs->header[5]|=0x02;
    /* last page flag? */
    if(vs->e_o_s && vs->lacing_fill==vals)vs->header[5]|=0x04;
    vs->b_o_s=1;

    /* 64 bits of PCM position */
    for(i=6;i<14;i++){
      vs->header[i]=(pcm_pos&0xff);
      pcm_pos>>=8;
    }

    /* 32 bits of stream serial number */
    {
      long serialno=vs->serialno;
      for(i=14;i<18;i++){
	vs->header[i]=(serialno&0xff);
	serialno>>=8;
      }
    }

    /* 32 bits of page counter (we have both counter and page header
       because this val can roll over) */
    {
      long pageno=vs->pageno++;
      for(i=18;i<22;i++){
	vs->header[i]=(pageno&0xff);
	pageno>>=8;
      }
    }

    /* zero for computation; filled in later */
    vs->header[22]=0;
    vs->header[23]=0;
    vs->header[24]=0;
    vs->header[25]=0;

    /* segment table */
    vs->header[26]=vals&0xff;
    for(i=0;i<vals;i++)
      bytes+=vs->header[i+27]=(vs->lacing_vals[i]&0xff);
      
    /* advance the lacing data and set the body_returned pointer */

    vs->lacing_fill-=vals;
    memmove(vs->lacing_vals,vs->lacing_vals+vals,vs->lacing_fill*sizeof(int));
    memmove(vs->pcm_vals,vs->pcm_vals+vals,vs->lacing_fill*sizeof(size64));
    vs->body_returned=bytes;

    /* set pointers in the vorbis_page struct */
    vg->header=vs->header;
    vg->header_len=vs->header_fill=vals+27;
    vg->body=vs->body_data;
    vg->body_len=bytes;

    /* calculate the checksum */

    _vs_checksum(vg);

    return(1);
  }

  /* not enough data to construct a page and not end of stream */
  return(0);
}

int vorbis_stream_eof(vorbis_stream_state *vs){
  return vs->e_o_s;
}

/* DECODING PRIMITIVES: packet streaming layer **********************/

/* This has two layers to place more of the multi-serialno and paging
   control in the application's hands.  First, we expose a data buffer
   using vorbis_decode_buffer().  The app either copies into the
   buffer, or passes it directly to read(), etc.  We then call
   vorbis_decode_wrote() to tell how many bytes we just added.

   Pages are returned (pointers into the buffer in vorbis_sync_state)
   by vorbis_decode_stream().  The page is then submitted to
   vorbis_decode_page() along with the appropriate
   vorbis_stream_state* (ie, matching serialno).  We then get raw
   packets out calling vorbis_stream_packet() with a
   vorbis_stream_state.  See the 'frame-prog.txt' docs for details and
   example code. */

/* initialize the struct to a known state */
int vorbis_sync_init(vorbis_sync_state *vs){
  if(vs){
    memset(vs,0,sizeof(vorbis_sync_state));
    _vorbis_crc_init();
  }
  return(0);
}

/* clear non-flat storage within */
int vorbis_sync_clear(vorbis_sync_state *vs){
  if(vs){
    if(vs->data)free(vs->data);
    vorbis_sync_init(vs);
  }
  return(0);
}

char *vorbis_decode_buffer(vorbis_sync_state *vs, long size){

  /* first, clear out any space that has been previously returned */
  if(vs->returned){
    vs->fill-=vs->returned;
    if(vs->fill-vs->returned>0)
      memmove(vs->data,vs->data+vs->returned,
	      (vs->fill-vs->returned)*sizeof(char));
    vs->returned=0;
  }

  if(size>vs->storage-vs->fill){
    /* We need to extend the internal buffer */
    long newsize=size+vs->fill+4096; /* an extra page to be nice */

    if(vs->data)
      vs->data=realloc(vs->data,newsize);
    else
      vs->data=malloc(newsize);
    vs->storage=newsize;
  }

  /* expose a segment at least as large as requested at the fill mark */
  return(vs->data+vs->fill);
}

int vorbis_decode_wrote(vorbis_sync_state *vs, long bytes){
  if(vs->fill+bytes>vs->storage)return(-1);
  vs->fill+=bytes;
  return(0);
}

/* sync the stream.  If we had to recapture, return -1.  If we
   couldn't capture at all, return 0.  If a page was already framed
   and ready to go, return 1 and fill in the page struct.

   Returns pointers into buffered data; invalidated by next call to
   _stream, _clear, _init, or _buffer */

int vorbis_decode_stream(vorbis_sync_state *vs, vorbis_page *vg){

  /* all we need to do is verify a page at the head of the stream
     buffer.  If it doesn't verify, we look for the next potential
     frame */

  while(1){
    char *page=vs->data+vs->returned;
    long bytes=vs->fill-vs->returned;
      
    if(vs->headerbytes==0){
      int headerbytes,i;
      if(bytes<27)return(0); /* not enough for a header */

      /* verify capture pattern */
      if(memcmp(page,"OggS",4))goto sync_fail;
      
      headerbytes=page[26]+27;
      if(bytes<headerbytes)return(0); /* not enough for header + seg table */

      /* count up body length in the segment table */

      for(i=0;i<page[26];i++)
	vs->bodybytes+=page[27+i];
      vs->headerbytes=headerbytes;
    }
    
    if(vs->bodybytes+vs->headerbytes>bytes)return(0);

    /* The whole test page is buffered.  Verify the checksum */
    {
      /* Grab the checksum bytes, set the header field to zero */
      char chksum[4];
      vorbis_page lvg;
      
      memcpy(chksum,page+22,4);
      memset(page+22,0,4);
      
      /* set up a temp page struct and recompute the checksum */
      lvg.header=page;
      lvg.header_len=vs->headerbytes;
      lvg.body=page+vs->headerbytes;
      lvg.body_len=vs->bodybytes;
      _vs_checksum(&lvg);
	
      /* Compare */
      if(memcmp(chksum,page+22,4)){
	/* D'oh.  Mismatch! Corrupt page (or miscapture and not a page
	   at all) */
	/* replace the computed checksum with the one actually read in */
	memcpy(page+22,chksum,4);
	  
	/* Bad checksum. Lose sync */
	goto sync_fail;
      }
    }
	
    vg->header=page;
    vg->header_len=vs->headerbytes;
    vg->body=page+vs->headerbytes;
    vg->body_len=vs->bodybytes;
    
    vs->unsynced=0;
    vs->returned+=vs->headerbytes+vs->bodybytes;
    vs->headerbytes=0;
    vs->bodybytes=0;

    return(1);

  sync_fail:
    
    vs->headerbytes=0;
    vs->bodybytes=0;

    /* search for possible capture */
    
    page=memchr(page+1,'O',bytes-1);
    if(page)
      vs->returned=page-vs->data;
    else
      vs->returned=vs->fill;

    if(!vs->unsynced){
      vs->unsynced=1;
      return(-1);
    }

    /* loop; verifier above will take care of determining capture or not */

  }
}

/* add the incoming page to the stream state; we decompose the page
   into packet segments here as well. */

int vorbis_decode_page(vorbis_stream_state *vs, vorbis_page *vg){
  unsigned char *header=vg->header;
  unsigned char *body=vg->body;
  long            bodysize=vg->body_len;
  int segptr=0;

  int version=vorbis_page_version(vg);
  int continued=vorbis_page_continued(vg);
  int bos=vorbis_page_bos(vg);
  int eos=vorbis_page_eos(vg);
  size64 pcmpos=vorbis_page_pcmpos(vg);
  int serialno=vorbis_page_serialno(vg);
  int pageno=vorbis_page_pageno(vg);
  int segments=header[26];
  
  /* clean up 'returned data' */
  {
    long lr=vs->lacing_returned;
    long br=vs->body_returned;

    /* body data */
    if(vs->body_fill-br)
      memmove(vs->body_data,vs->body_data+br,vs->body_fill-br);
    vs->body_fill-=br;
    vs->body_returned=0;

    /* segment table */
    if(vs->lacing_fill-lr){
      memmove(vs->lacing_vals,vs->lacing_vals+lr,
	      (vs->lacing_fill-lr)*sizeof(int));
      memmove(vs->pcm_vals,vs->pcm_vals+lr,
	      (vs->lacing_fill-lr)*sizeof(size64));
    }
    vs->lacing_fill-=lr;
    vs->lacing_packet-=lr;
    vs->lacing_returned=0;
  }

  /* check the serial number */
  if(serialno!=vs->serialno)return(-1);
  if(version>0)return(-1);

  _vs_lacing_expand(vs,segments+1);

  /* are we in sequence? */
  if(pageno!=vs->pageno){
    int i;

    /* unroll previous partial packet (if any) */
    for(i=vs->lacing_packet;i<vs->lacing_fill;i++)
      vs->body_fill-=vs->lacing_vals[i]&0xff;
    vs->lacing_fill=vs->lacing_packet;

    /* make a note of dropped data in segment table */
    vs->lacing_vals[vs->lacing_fill++]=0x400;
    vs->lacing_packet++;

    /* are we a 'continued packet' page?  If so, we'll need to skip
       some segments */
    if(continued){
      bos=0;
      for(;segptr<segments;segptr++){
	int val=header[27+segptr];
	body+=val;
	bodysize-=val;
	if(val<255){
	  segptr++;
	  break;
	}
      }
    }
  }
  
  if(bodysize){
    _vs_body_expand(vs,bodysize);
    memcpy(vs->body_data+vs->body_fill,body,bodysize);
    vs->body_fill+=bodysize;
  }

  while(segptr<segments){
    int val=header[27+segptr];
    vs->lacing_vals[vs->lacing_fill]=val;
    vs->pcm_vals[vs->lacing_fill]=pcmpos;

    if(bos){
      vs->lacing_vals[vs->lacing_fill]|=0x100;
      bos=0;
    }

    vs->lacing_fill++;
    segptr++;

    if(val<255)vs->lacing_packet=vs->lacing_fill;
  }
  if(eos){
    vs->e_o_s=1;
    if(vs->lacing_fill>0)
      vs->lacing_vals[vs->lacing_fill-1]|=0x200;
  }

  vs->pageno=pageno;

  return(0);
}

/* clear things to an initial state.  Good to call, eg, before seeking */
int vorbis_sync_reset(vorbis_sync_state *vs){
  vs->fill=0;
  vs->returned=0;
  vs->unsynced=0;
  vs->headerbytes=0;
  vs->bodybytes=0;
  return(0);
}

int vorbis_stream_reset(vorbis_stream_state *vs){
  vs->body_fill=0;
  vs->body_returned=0;

  vs->lacing_fill=0;
  vs->lacing_packet=0;
  vs->lacing_returned=0;

  vs->header_fill=0;

  vs->e_o_s=0;
  vs->b_o_s=0;
  vs->pageno=0;
  vs->pcmpos=0;

  return(0);
}

int vorbis_stream_packet(vorbis_stream_state *vs,vorbis_packet *vp){

  /* The last part of decode. We have the stream broken into packet
     segments.  Now we need to group them into packets (or return the
     out of sync markers) */

  int ptr=vs->lacing_returned;

  if(vs->lacing_packet<=ptr)return(0);
  if(vs->lacing_vals[ptr]&=0x400){
    /* We lost sync here; let the app know */
    vs->lacing_returned++;
    return(-1);
  }

  /* Gather the whole packet. We'll have no holes or a partial packet */
  {
    int size=vs->lacing_vals[ptr]&0xff;
    int bytes=0;

    vp->packet=vs->body_data+vs->body_returned;
    vp->e_o_s=vs->lacing_vals[ptr]&0x200; /* last packet of the stream? */
    vp->b_o_s=vs->lacing_vals[ptr]&0x100; /* first packet of the stream? */
    bytes+=size;

    while(size==255){
      int val=vs->lacing_vals[++ptr];
      size=val&0xff;
      if(val&0x200)vp->e_o_s=0x200;
      bytes+=size;
    }
    vp->pcm_pos=vs->pcm_vals[ptr];
    vp->bytes=bytes;

    vs->body_returned+=bytes;
    vs->lacing_returned=ptr+1;
  }
  return(1);
}

#ifdef _V_SELFTEST
#include <stdio.h>

void test_pack(vorbis_stream_state *vs,int *pl, int **headers){
  unsigned char *data=malloc(1024*1024); /* for scripted test cases only */
  long inptr=0;
  long outptr=0;
  long pcm_pos=0;
  int i,j,packets,pageno=0;

  for(packets=0;;packets++)if(pl[packets]==-1)break;

  for(i=0;i<packets;i++){
    /* construct a test packet */
    vorbis_packet vp;
    int len=pl[i];
    
    vp.packet=data+inptr;
    vp.bytes=len;
    vp.e_o_s=(pl[i+1]<0?1:0);
    vp.pcm_pos=pcm_pos;

    pcm_pos+=1024;

    for(j=0;j<len;j++)data[inptr++]=i+j;

    /* submit the test packet */
    vorbis_stream_encode(vs,&vp);

    /* retrieve any finished pages */
    {
      vorbis_page vg;
      
      while(vorbis_stream_page(vs,&vg)){
	/* We have a page.  Check it carefully */

	fprintf(stderr,"%d, ",pageno);

	if(headers[pageno]==NULL){
	  fprintf(stderr,"coded too many pages!\n");
	  exit(1);
	}

	/* Test data */
	for(j=0;j<vg.body_len;j++)
	if(vg.body[j]!=(data+outptr)[j]){
	  fprintf(stderr,"body data mismatch at pos %ld: %x!=%x!\n\n",
		  outptr+j,(data+outptr)[j],vg.body[j]);
	  exit(1);
	}
	outptr+=vg.body_len;

	/* Test header */
	for(j=0;j<vg.header_len;j++){
	  if(vg.header[j]!=headers[pageno][j]){
	    fprintf(stderr,"header content mismatch at pos %ld:\n",j);
	    for(j=0;j<headers[pageno][26]+27;j++)
	      fprintf(stderr," (%d)%02x:%02x",j,headers[pageno][j],vg.header[j]);
	    fprintf(stderr,"\n");
	    exit(1);
	  }
	}
	if(vg.header_len!=headers[pageno][26]+27){
	  fprintf(stderr,"header length incorrect! (%ld!=%d)\n",
		  vg.header_len,headers[pageno][26]+27);
	  exit(1);
	}
	pageno++;
      }
    }
  }
  free(data);
  if(headers[pageno]!=NULL){
    fprintf(stderr,"did not write last page!\n");
    exit(1);
  }
  if(inptr!=outptr){
    fprintf(stderr,"packet data incomplete!\n");
    exit(1);
  }
  fprintf(stderr,"ok.\n");
}

int main(void){
  vorbis_stream_state vs;

  /* Exercise each code path in the framing code.  Also verify that
     the checksums are working.  */

  {
    /* 17, 254, 255, 256, 500, 510, 600 byte, pad */
    int packets[]={17, 254, 255, 256, 500, 510, 600, -1};
    int head1[] = {0x4f,0x67,0x67,0x53,0,0,
		   0x00,0x18,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x7e,0xea,0x18,0xd0,
		   14,
		   17,254,255,0,255,1,255,245,255,255,0,
		   255,255,90};
    int *headret[]={head1,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing basic page encoding... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }

  {
    /* nil packets; beginning,middle,end */
    int packets[]={0,17, 254, 255, 0, 256, 0, 500, 510, 600, 0, -1};

    int head1[] = {0x4f,0x67,0x67,0x53,0,0,
		   0x00,0x28,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xcf,0x70,0xa4,0x1b,
		   18,
		   0,17,254,255,0,0,255,1,0,255,245,255,255,0,
		   255,255,90,0};
    int *headret[]={head1,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing basic nil packets... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }

  {
    /* starting new page with first segment */
    int packets[]={4345,259,255,-1};

    int head1[] = {0x4f,0x67,0x67,0x53,0,0,
		   0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xe7,0xdc,0x6d,0x09,
		   17,
		   255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255,255};

    int head2[] = {0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x08,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x5f,0x54,0x07,0x69,
		   5,
		   10,255,4,255,0};
    int *headret[]={head1,head2,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing single-packet page span... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }

  {
    /* starting new page with first segment */
    int packets[]={100,4345,259,255,-1};

    int head1[] = {0x4f,0x67,0x67,0x53,0,0x00,
		   0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x6f,0xac,0x43,0x67,
		   17,
		   100,255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255};

    int head2[] = {0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0xe7,0xa3,0x34,0x53,
		   6,
		   255,10,255,4,255,0};
    int *headret[]={head1,head2,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing multi-packet page span... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }


  /* page with the 255 segment limit */
  {

    int packets[]={10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,50,-1};

    int head1[] = {0x4f,0x67,0x67,0x53,0,0x00,
		   0x00,0xf8,0x03,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0xb1,0xd0,0xab,0xbc,
		   255,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10,10,
		   10,10,10,10,10,10,10};

    int head2[] = {0x4f,0x67,0x67,0x53,0,0x01,
		   0x00,0xfc,0x03,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0xb1,0x1e,0x4f,0x41,
		   1,
		   50};
    int *headret[]={head1,head2,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing max packet segments... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }

  {
    /* packet that overspans over an entire page */

    int packets[]={100,9000,259,255,-1};

    int head1[] = {0x4f,0x67,0x67,0x53,0,0x00,
		   0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x6f,0xac,0x43,0x67,
		   17,
		   100,255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255};

    int head2[] = {0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x65,0x4e,0xbd,0x96,
		   17,
		   255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255,255};

    int head3[] = {0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,2,0,0,0,
		   0x84,0x86,0x86,0xf4,
		   7,
		   255,255,75,255,4,255,0};
    int *headret[]={head1,head2,head3,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing very large packets... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }

  {
    /* nil page.  why not? */

    int packets[]={100,4080,-1};

    int head1[] = {0x4f,0x67,0x67,0x53,0,0x00,
		   0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,0,0,0,0,
		   0x6f,0xac,0x43,0x67,
		   17,
		   100,255,255,255,255,255,255,255,255,
		   255,255,255,255,255,255,255,255};

    int head2[] = {0x4f,0x67,0x67,0x53,0,0x02,
		   0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,
		   0x01,0x02,0x03,0x04,1,0,0,0,
		   0x71,0xc8,0x17,0x8c,
		   1,0};

    int *headret[]={head1,head2,NULL};
    
    vorbis_stream_init(&vs,0x04030201);
    fprintf(stderr,"testing nil page... ");
    test_pack(&vs,packets,headret);
    vorbis_stream_clear(&vs);
  }

  

  return(0);
}

#endif




