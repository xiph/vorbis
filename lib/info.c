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
 last modification date: Oct 03 1999

 ********************************************************************/

/* This fills in a vorbis_info structure with settings from a few
   pre-defined encoding modes.  Also handles choosing/blowing in the
   codebook */

#include <stdlib.h>
#include <string.h>
#include "modes.h"
#include "bitwise.h"

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
int vorbis_info_add_comment(vorbis_info *vi,char *comment){
  vi->user_comments=realloc(vi->user_comments,
			    (vi->max_comment+2)*sizeof(char *));
  vi->user_comments[vi->max_comment]=strdup(comment);
  vi->max_comment++;
  vi->user_comments[vi->max_comment]=NULL;
  return(0);
}

static int _v_writestring(oggpack_buffer *o,char *s){
  while(*s){
    _oggpack_write(o,*s++,8);
  }
}

static char *_v_readstring(oggpack_buffer *o,int bytes){
  char *ret=calloc(bytes+1,1);
  char *ptr=ret;
  while(bytes--){
    *ptr++=_oggpack_read(o,8);
  }
  return(ret);
}

/* The Vorbis header is in three packets; the initial small packet in
   the first page that identifies basic parameters, a second packet
   with bitstream comments and a third packet that holds the
   codebook. */

int vorbis_info_headerin(vorbis_info *vi,ogg_packet *op){

}

int vorbis_info_headerout(vorbis_info *vi,
			  ogg_packet *op,
			  ogg_packet *op_comm,
			  ogg_packet *op_code){

  oggpack_buffer opb;
  /* initial header:

     codec id     "vorbis"
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
  _v_writestring(&obp,"vorbis");
  _oggpack_write(&opb,0x00,32);

  _oggpack_write(&opb,vi->channels,32);
  _oggpack_write(&opb,vi->rate,32);
  _oggpack_write(&opb,vi->smallblock,32);
  _oggpack_write(&opb,vi->largeblock,32);
  _oggpack_write(&opb,vi->envelopesa,32);
  _oggpack_write(&opb,vi->envelopech,32);
  _oggpack_write(&opb,vi->floororder,8);
  _oggpack_write(&opb,vi->flooroctaves,8);
  _oggpack_write(&opb,vi->floorch,32);

  /* build the packet */
  if(vi->header)free(vi->header);
  vi->header=memcpy(opb.buffer,_oggpack_bytes(&opb));
  op->packet=vi->header;
  op->bytes=_oggpack_bytes(&opb);
  op->b_o_s=1;
  op->e_o_s=0;
  op->frameno=0;

  /* comment header:
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
  if(vi->vendor){
    _oggpack_write(&opb,strlen(vi->vendor),32);
    _oggpack_write(&opb,vi->vendor,strlen(vi->vendor));
  }else{
    _oggpack_write(&opb,0,32);
  }
  
  _oggpack_write(&opb,vi->max_comment,32);
  if(vi->max_comment){
    int i;
    for(i=0;i<=vi->max_comment;i++){
      if(vi->user_comments[i]){
	_oggpack_write(&opb,strlen(vi->user_comments[i]),32);
	_oggpack_write(&opb,vi->user_comments[i],strlen(vi->user_comments[i]));
      }else{
	_oggpack_write(&opb,0,32);
      }
    }
  }
  
  if(vi->header1)free(vi->header1);
  vi->header1=memcpy(opb.buffer,_oggpack_bytes(&opb));
  op_comm->packet=vi->header1;
  op_comm->bytes=_oggpack_bytes(&opb);
  op_comm->b_o_s=0;
  op_comm->e_o_s=0;
  op_comm->frameno=0;

  /* codebook header:
     nul so far; not encoded yet */

  if(vi->header2)free(vi->header2);
  vi->header2=NULL;
  op_code->packet=vi->header2;
  op_code->bytes=0;
  op_code->b_o_s=0;
  op_code->e_o_s=0;
  op_code->frameno=0;

  return(0);
}

int vorbis_info_clear(vorbis_info *vi){
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
  

