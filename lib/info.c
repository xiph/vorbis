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

 function: maintain the info structure, info <-> header packets
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 04 1999

 ********************************************************************/

/* This fills in a vorbis_info structure with settings from a few
   pre-defined encoding modes.  Also handles choosing/blowing in the
   codebook */

#include <stdlib.h>
#include <string.h>
#include "modes.h"
#include "bitwise.h"

void vorbis_info_init(vorbis_info *vi){
  memset(vi,0,sizeof(vorbis_info));
}

/* one test mode for now; temporary of course */
int vorbis_info_modeset(vorbis_info *vi, int mode){
  if(mode<0 || mode>predef_mode_max)return(-1);

  /* handle the flat settings first */
  memcpy(vi,&(predef_modes[mode]),sizeof(vorbis_info));
  vi->user_comments=calloc(1,sizeof(char *));
  vi->vendor=strdup("Xiphophorus libVorbis I 19991003");

  return(0);
}

/* convenience function */
int vorbis_info_addcomment(vorbis_info *vi,char *comment){
  vi->user_comments=realloc(vi->user_comments,
			    (vi->comments+1)*sizeof(char *));
  vi->user_comments[vi->comments]=strdup(comment);
  vi->comments++;
  vi->user_comments[vi->comments]=NULL;
  return(0);
}

static void _v_writestring(oggpack_buffer *o,char *s){
  while(*s){
    _oggpack_write(o,*s++,8);
  }
}

static void _v_readstring(oggpack_buffer *o,char *buf,int bytes){
  while(bytes--){
    *buf++=_oggpack_read(o,8);
  }
}

/* The Vorbis header is in three packets; the initial small packet in
   the first page that identifies basic parameters, a second packet
   with bitstream comments and a third packet that holds the
   codebook. */

int vorbis_info_headerin(vorbis_info *vi,ogg_packet *op){

  oggpack_buffer opb;
  
  if(op){
    _oggpack_readinit(&opb,op->packet,op->bytes);

    /* Which of the three types of header is this? */
    /* Also verify header-ness, vorbis */
    {
      char buffer[6];
      memset(buffer,0,6);
      _v_readstring(&opb,buffer,6);
      if(memcmp(buffer,"vorbis",6)){
	/* not a vorbis header */
	return(-1);
      }
      switch(_oggpack_read(&opb,8)){
      case 0:
	if(!op->b_o_s){
	  /* Not the initial packet */
	  return(-1);
	}
	if(vi->rate!=0){
	  /* previously initialized info header */
	  return(-1);
	}

	vi->channels=_oggpack_read(&opb,32);
	vi->rate=_oggpack_read(&opb,32);
	vi->smallblock=_oggpack_read(&opb,32);
	vi->largeblock=_oggpack_read(&opb,32);
	vi->envelopesa=_oggpack_read(&opb,32);
	vi->envelopech=_oggpack_read(&opb,16);
	vi->floororder=_oggpack_read(&opb,8);
	vi->flooroctaves=_oggpack_read(&opb,8);
	vi->floorch=_oggpack_read(&opb,16);

	return(0);
      case 1:
	if(vi->rate==0){
	  /* um... we didn;t get the initial header */
	  return(-1);
	}
	{
	  int vendorlen=_oggpack_read(&opb,32);
	  vi->vendor=calloc(vendorlen+1,1);
	  _v_readstring(&opb,vi->vendor,vendorlen);
	}
	{
	  int i;
	  vi->comments=_oggpack_read(&opb,32);
	  vi->user_comments=calloc(vi->comments+1,sizeof(char **));
	    
	  for(i=0;i<=vi->comments;i++){
	    int len=_oggpack_read(&opb,32);
	    vi->user_comments[i]=calloc(len+1,1);
	    _v_readstring(&opb,vi->user_comments[i],len);
	  }	  
	}

	return(0);
      case 2:
	if(vi->rate==0){
	  /* um... we didn;t get the initial header */
	  return(-1);
	}

	/* not implemented quite yet */

	return(0);
      default:
	/* Not a valid vorbis header type */
	return(-1);
	break;
      }
    }
  }
  return(-1);
}

int vorbis_info_headerout(vorbis_info *vi,
			  ogg_packet *op,
			  ogg_packet *op_comm,
			  ogg_packet *op_code){

  oggpack_buffer opb;
  /* initial header:

     codec id     "vorbis"
     header id    0 (byte)
     codec ver    (4 octets, lsb first: currently 0x00)
     pcm channels (4 octets, lsb first)
     pcm rate     (4 octets, lsb first)
     
     small block  (4 octets, lsb first)
     large block  (4 octets, lsb first)
     envelopesa   (4 octets, lsb first)
     envelopech   (4 octets, lsb first)
     floororder   octet
     flooroctaves octet
     floorch      (4 octets, lsb first)
   */

  _oggpack_writeinit(&opb);
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x00,8);

  _oggpack_write(&opb,0x00,32);

  _oggpack_write(&opb,vi->channels,32);
  _oggpack_write(&opb,vi->rate,32);
  _oggpack_write(&opb,vi->smallblock,32);
  _oggpack_write(&opb,vi->largeblock,32);
  _oggpack_write(&opb,vi->envelopesa,32);
  _oggpack_write(&opb,vi->envelopech,16);
  _oggpack_write(&opb,vi->floororder,8);
  _oggpack_write(&opb,vi->flooroctaves,8);
  _oggpack_write(&opb,vi->floorch,16);

  /* build the packet */
  if(vi->header)free(vi->header);
  vi->header=malloc(_oggpack_bytes(&opb));
  memcpy(vi->header,opb.buffer,_oggpack_bytes(&opb));
  op->packet=vi->header;
  op->bytes=_oggpack_bytes(&opb);
  op->b_o_s=1;
  op->e_o_s=0;
  op->frameno=0;

  /* comment header:
     codec id       "vorbis"
     header id      1 (byte)
     vendor len     (4 octets, lsb first)
     vendor and id  (n octects as above)
     comments       (4 octets, lsb first)
     comment 0 len  (4 octets, lsb first)
     comment 0 len  (n octets as above)
     ...
     comment n-1 len  (4 octets, lsb first)
     comment 0-1 len  (n octets as above)
  */

  _oggpack_reset(&opb);
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x01,8);

  if(vi->vendor){
    _oggpack_write(&opb,strlen(vi->vendor),32);
    _v_writestring(&opb,vi->vendor);
  }else{
    _oggpack_write(&opb,0,32);
  }
  
  _oggpack_write(&opb,vi->comments,32);
  if(vi->comments){
    int i;
    for(i=0;i<vi->comments;i++){
      if(vi->user_comments[i]){
	_oggpack_write(&opb,strlen(vi->user_comments[i]),32);
	_v_writestring(&opb,vi->user_comments[i]);
      }else{
	_oggpack_write(&opb,0,32);
      }
    }
  }
  
  if(vi->header1)free(vi->header1);
  vi->header1=malloc(_oggpack_bytes(&opb));
  memcpy(vi->header1,opb.buffer,_oggpack_bytes(&opb));
  op_comm->packet=vi->header1;
  op_comm->bytes=_oggpack_bytes(&opb);
  op_comm->b_o_s=0;
  op_comm->e_o_s=0;
  op_comm->frameno=0;

  /* codebook header:
     codec id       "vorbis"
     header id      2 (byte)
     nul so far; not encoded yet */

  _oggpack_reset(&opb);
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x02,8);

  if(vi->header2)free(vi->header2);
  vi->header2=malloc(_oggpack_bytes(&opb));
  memcpy(vi->header2,opb.buffer,_oggpack_bytes(&opb));
  op_code->packet=vi->header2;
  op_code->bytes=_oggpack_bytes(&opb);
  op_code->b_o_s=0;
  op_code->e_o_s=0;
  op_code->frameno=0;

  _oggpack_writefree(&opb);

  return(0);
}

void vorbis_info_clear(vorbis_info *vi){
  /* clear the non-flat storage before zeroing */

  /* comments */
  if(vi->user_comments){
    char **ptr=vi->user_comments;
    while(*ptr){
      free(*(ptr++));
    }
    free(vi->user_comments);
  }

  /* vendor string */
  if(vi->vendor)free(vi->vendor);

  /* local encoding storage */
  if(vi->header)free(vi->header);
  if(vi->header1)free(vi->header1);
  if(vi->header2)free(vi->header2);

  memset(vi,0,sizeof(vorbis_info));
}
  

