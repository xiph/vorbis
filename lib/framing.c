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

 function: code raw Vorbis packets into framed vorbis stream and
           decode vorbis streams back into raw packets
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Jul 10 1999

 note: The CRC code is directly derived from public domain code by
 Ross Williams (ross@guest.adelaide.edu.au).  See framing.txt for
 details.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include "codec.h"

/* A complete description of Vorbis framing exists in docs/framing.txt */

/* helper to initialize lookup for direct-table CRC */
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

int vorbis_stream_init(vorbis_stream_state *vs,int serialno){
  int i;
  if(vs){
    memset(vs,0,sizeof(vorbis_stream_state));
    vs->body_storage=16*1024;
    vs->body_data=malloc(vs->body_storage*sizeof(char));

    vs->lacing_storage=1024;
    vs->lacing_vals=malloc(vs->lacing_storage*sizeof(int));
    vs->pcm_vals=malloc(vs->lacing_storage*sizeof(size64));

    /* initialize the crc_lookup table */
    for (i=0;i<256;i++)
      vs->crc_lookup[i]=_vorbis_crc_entry((unsigned long)i);

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

/* checksum the page */
/* Direct table CRC; note that this will be faster in the future if we
   perform the checksum silmultaneously with other copies */

static void _vs_checksum(vorbis_stream_state *vs,vorbis_page *vg){
  unsigned size32 crc_reg=0;
  unsigned size32 *lookup=vs->crc_lookup;
  int i;

  for(i=0;i<vg->header_len;i++)
    crc_reg=(crc_reg<<8)^lookup[((crc_reg >> 24)&0xff)^vg->header[i]];
  for(i=0;i<vg->body_len;i++)
    crc_reg=(crc_reg<<8)^lookup[((crc_reg >> 24)&0xff)^vg->body[i]];
  
  vg->header[22]=crc_reg&0xff;
  vg->header[23]=(crc_reg>>8)&0xff;
  vg->header[24]=(crc_reg>>16)&0xff;
  vg->header[25]=(crc_reg>>24)&0xff;
}

/* submit data to the internal buffer of the framing engine */
int vorbis_stream_encode(vorbis_stream_state *vs,vorbis_packet *vp){
  int lacing_vals=vp->bytes/255+1,i;

  /* make sure we have the buffer storage */
  if(vs->body_storage<=vs->body_fill+vp->bytes){
    vs->body_storage+=(vp->bytes+1024);
    vs->body_data=realloc(vs->body_data,vs->body_storage);
  }
  if(vs->lacing_storage<=vs->lacing_fill+lacing_vals){
    vs->lacing_storage+=(lacing_vals+32);
    vs->lacing_vals=realloc(vs->lacing_vals,vs->lacing_storage*sizeof(int));
    vs->pcm_vals=realloc(vs->pcm_vals,vs->lacing_storage*sizeof(size64));
  }

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
	    vs->body_fill*sizeof(char));
    vs->body_returned=0;
  }

  if((vs->e_o_s&&vs->lacing_fill) || 
     vs->body_fill > 4096 || 
     vs->lacing_fill>=255){
    int vals=0,bytes=0;
    int maxvals=(vs->lacing_fill>255?255:vs->lacing_fill);
    long acc=0;

    /* construct a page */
    /* decide how many segments to include */
    for(vals=0;vals<maxvals;vals++){
      if(acc>4096)break;
      acc+=vs->lacing_vals[vals]&0x0ff;
    }

    /* construct the header in temp storage */
    memcpy(vs->header,"OggS",4);

    /* stream structure version */
    vs->header[4]=0x00;

    if(vs->b_o_s==0){ 
      /* Ah, this is the first page */
      vs->header[5]=0x00;
    }else{
      if(vs->lacing_vals[0]&0x100){
	/* The first packet segment is the beginning of a packet */
	vs->header[5]=0x01;
      }else{
	vs->header[5]=0x02;
      }
    }
    vs->b_o_s=1;

    /* 64 bits of PCM position */
    {
      size64 pcm_pos=vs->pcm_vals[vals-1];

      for(i=6;i<14;i++){
	vs->header[i]=(pcm_pos&0xff);
	pcm_pos>>=8;
      }
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
    vg->header_len=vs->headerbytes=vals+27;
    vg->body=vs->body_data;
    vg->body_len=bytes;

    /* calculate the checksum */

    _vs_checksum(vs,vg);

    return(1);
  }

  /* not enough data to construct a page and not end of stream */
  return(0);
}

int vorbis_stream_eof(vorbis_stream_state *vs){
  return vs->e_o_s;
}

/* DECODING PRIMITIVES: packet streaming layer **********************/

/* Accepts data for decoding; syncs and frames the bitstream, then
   decodes pages into packets.  Works through errors and dropouts,
   reporting holes/errors in the bitstream when the missing/corrupt
   packet is requested by vorbis_stream_packet(). Call with size=-1
   for EOS */

/* all below:  <0 error, 0 not enough data, >0 success */

int vorbis_stream_decode(vorbis_stream_state *vs,char *stream,int size){
  /* beginning a page? ie, no header started yet?*/
  if vs->
  
  


}

size64 vorbis_stream_pcmpos(vorbis_stream_state *vs){
}

size64 vorbis_stream_pageno(vorbis_stream_state *vs){
}

int vorbis_stream_skippage(vorbis_stream_state *vs){
}

int vorbis_stream_clearbuf(vorbis_stream_state *vs){
}

int vorbis_stream_packet(vorbis_stream_state *vs,vorbis_packet *vp){
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




