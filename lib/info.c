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

 function: maintain the info structure, info <-> header packets
 last mod: $Id: info.c,v 1.15 2000/01/19 08:57:55 xiphmont Exp $

 ********************************************************************/

/* general handling of the header and the vorbis_info structure (and
   substructures) */

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"

/* these modules were split out only to make this file more readable.
   I don't want to expose the symbols */
#include "infomap.c"

/* helpers */

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
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

/* convenience functions for the interface */
int vorbis_info_addcomment(vorbis_info *vi,char *comment){
  vi->user_comments=realloc(vi->user_comments,
			    (vi->comments+2)*sizeof(char *));
  vi->user_comments[vi->comments]=strdup(comment);
  vi->comments++;
  vi->user_comments[vi->comments]=NULL;
  return(0);
}

int vorbis_info_addvendor(vorbis_info *vi,char *vendor){
  if(vi->vendor)free(vi->vendor);
  vi->vendor=strdup(vendor);
  return(0);
}
  
/* libVorbis expects modes to be submitted in an already valid
   vorbis_info structure, but also expects some of the elements to be
   allocated storage.  To make this easier, the Vorbis distribution
   includes a number of modes in static storage as headers.  To
   allocate a copy, run it through vorbis_info_dup */

/* note that due to codebook size, codebooks are not fully duplicated.
   The info structure (aside from the embedded codebook elements) will
   be fully dupped */

int vorbis_info_dup(vorbis_info *dest,vorbis_info *source){
  int i;
  memcpy(dest,source,sizeof(vorbis_info));

  /* also dup individual pieces that need to be allocated */
  /* dup user comments (unlikely to have any, but for completeness */
  if(source->comments>0){
    dest->user_comments=calloc(source->comments+1,sizeof(char *));
    for(i=0;i<source->comments;i++)
      dest->user_comments[i]=strdup(source->user_comments[i]);
  }
  /* dup vendor */
  if(source->vendor)
    dest->vendor=strdup(source->vendor);

  /* dup mode maps, blockflags and map types */
  if(source->modes){
    dest->blockflags=malloc(source->modes*sizeof(int));
    dest->maptypes=malloc(source->modes*sizeof(int));
    dest->maplist=calloc(source->modes,sizeof(void *));

    memcpy(dest->blockflags,source->blockflags,sizeof(int)*dest->modes);
    memcpy(dest->maptypes,source->maptypes,sizeof(int)*dest->modes);
    for(i=0;i<source->modes;i++){
      void *dup;
      if(dest->maptypes[i]<0|| dest->maptypes[i]>=VI_MAPB)goto err_out;
      if(!(dup=vorbis_map_dup_P[dest->maptypes[i]](source->maplist[i])))
	goto err_out; 
      dest->maplist[i]=dup;
    }
  }

  /* dup (partially) books */
  if(source->books){
    dest->booklist=malloc(source->books*sizeof(codebook *));
    for(i=0;i<source->books;i++){
      dest->booklist[i]=calloc(1,sizeof(codebook));
      vorbis_book_dup(dest->booklist[i],source->booklist[i]);
    }
  }
  /* we do *not* dup local storage */
  dest->header=NULL;
  dest->header1=NULL;
  dest->header2=NULL;
  
  return(0);
err_out:
  vorbis_info_clear(dest);
  return(-1);
}

void vorbis_info_clear(vorbis_info *vi){
  int i;
  if(vi->comments){
    for(i=0;i<vi->comments;i++)
      if(vi->user_comments[i])free(vi->user_comments[i]);
    free(vi->user_comments);
  }
  if(vi->vendor)free(vi->vendor);
  if(vi->modes){
    for(i=0;i<vi->modes;i++)
      if(vi->maptypes[i]>=0 && vi->maptypes[i]<VI_MAPB)
	vorbis_map_free_P[vi->maptypes[i]](vi->maplist[i]);
    free(vi->maplist);
    free(vi->maptypes);
    free(vi->blockflags);
  }
  if(vi->books){
    for(i=0;i<vi->books;i++){
      if(vi->booklist[i]){
	vorbis_book_clear(vi->booklist[i]);
	free(vi->booklist[i]);
      }
    }
    free(vi->booklist);
  }
  
  if(vi->header)free(vi->header);
  if(vi->header1)free(vi->header1);
  if(vi->header2)free(vi->header2);

  memset(vi,0,sizeof(vorbis_info));
}

/* Header packing/unpacking ********************************************/

static int _vorbis_unpack_info(vorbis_info *vi,oggpack_buffer *opb){
  vi->version=_oggpack_read(opb,32);
  if(vi->version!=0)return(-1);

  vi->channels=_oggpack_read(opb,8);
  vi->rate=_oggpack_read(opb,32);

  vi->bitrate_upper=_oggpack_read(opb,32);
  vi->bitrate_nominal=_oggpack_read(opb,32);
  vi->bitrate_lower=_oggpack_read(opb,32);

  vi->blocksizes[0]=1<<_oggpack_read(opb,4);
  vi->blocksizes[1]=1<<_oggpack_read(opb,4);
  
  if(vi->rate<1)goto err_out;
  if(vi->channels<1)goto err_out;
  if(vi->blocksizes[0]<8)goto err_out; 
  if(vi->blocksizes[1]<vi->blocksizes[0])
    goto err_out; /* doubles as EOF check */

  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(-1);
}

static int _vorbis_unpack_comments(vorbis_info *vi,oggpack_buffer *opb){
  int i;
  int vendorlen=_oggpack_read(opb,32);
  if(vendorlen<0)goto err_out;
  vi->vendor=calloc(vendorlen+1,1);
  _v_readstring(opb,vi->vendor,vendorlen);
  vi->comments=_oggpack_read(opb,32);
  if(vi->comments<0)goto err_out;
  vi->user_comments=calloc(vi->comments+1,sizeof(char **));
	    
  for(i=0;i<vi->comments;i++){
    int len=_oggpack_read(opb,32);
    if(len<0)goto err_out;
    vi->user_comments[i]=calloc(len+1,1);
    _v_readstring(opb,vi->user_comments[i],len);
  }	  
  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(-1);
}

/* all of the real encoding details are here.  The modes, books,
   everything */
static int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb){
  int i;

  vi->modes=_oggpack_read(opb,16);
  vi->blockflags=malloc(vi->modes*sizeof(int));
  vi->maptypes=malloc(vi->modes*sizeof(int));
  vi->maplist=calloc(vi->modes,sizeof(void *));

  for(i=0;i<vi->modes;i++){
    vi->blockflags[i]=_oggpack_read(opb,1);
    vi->maptypes[i]=_oggpack_read(opb,8);
    if(vi->maptypes[i]<0 || vi->maptypes[i]>VI_MAPB)goto err_out;
    vi->maplist[i]=vorbis_map_unpack_P[vi->maptypes[i]](opb);
    if(!vi->maplist[i])goto err_out;
  }

  vi->books=_oggpack_read(opb,16);
  vi->booklist=calloc(vi->books,sizeof(codebook *));
  for(i=0;i<vi->books;i++){
    vi->booklist[i]=calloc(1,sizeof(codebook));
    if(vorbis_book_unpack(opb,vi->booklist[i]))goto err_out;
  }
  return(0);
err_out:
  vorbis_info_clear(vi);
  return(-1);
}

/* The Vorbis header is in three packets; the initial small packet in
   the first page that identifies basic parameters, a second packet
   with bitstream comments and a third packet that holds the
   codebook. */

/* call before header in, or just to zero out uninitialized mem */
void vorbis_info_init(vorbis_info *vi){
  memset(vi,0,sizeof(vorbis_info));
}

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
      case 0x80:
	if(!op->b_o_s){
	  /* Not the initial packet */
	  return(-1);
	}
	if(vi->rate!=0){
	  /* previously initialized info header */
	  return(-1);
	}

	return(_vorbis_unpack_info(vi,&opb));

      case 0x81:
	if(vi->rate==0){
	  /* um... we didn't get the initial header */
	  return(-1);
	}

	return(_vorbis_unpack_comments(vi,&opb));

      case 0x82:
	if(vi->rate==0 || vi->vendor==NULL){
	  /* um... we didn;t get the initial header or comments yet */
	  return(-1);
	}

	return(_vorbis_unpack_books(vi,&opb));

      default:
	/* Not a valid vorbis header type */
	return(-1);
	break;
      }
    }
  }
  return(-1);
}

/* pack side **********************************************************/

static int _vorbis_pack_info(oggpack_buffer *opb,vorbis_info *vi){
  /* preamble */  
  _v_writestring(opb,"vorbis");
  _oggpack_write(opb,0x80,8);

  /* basic information about the stream */
  _oggpack_write(opb,0x00,32);
  _oggpack_write(opb,vi->channels,8);
  _oggpack_write(opb,vi->rate,32);

  _oggpack_write(opb,vi->bitrate_upper,32);
  _oggpack_write(opb,vi->bitrate_nominal,32);
  _oggpack_write(opb,vi->bitrate_lower,32);

  _oggpack_write(opb,ilog2(vi->blocksizes[0]),4);
  _oggpack_write(opb,ilog2(vi->blocksizes[1]),4);
  
  return(0);
}

static int _vorbis_pack_comments(oggpack_buffer *opb,vorbis_info *vi){
  char temp[]="Xiphophorus libVorbis I 20000114";

  /* preamble */  
  _v_writestring(opb,"vorbis");
  _oggpack_write(opb,0x81,8);

  /* vendor */
  _oggpack_write(opb,strlen(temp),32);
  _v_writestring(opb,temp);
  
  /* comments */

  _oggpack_write(opb,vi->comments,32);
  if(vi->comments){
    int i;
    for(i=0;i<vi->comments;i++){
      if(vi->user_comments[i]){
	_oggpack_write(opb,strlen(vi->user_comments[i]),32);
	_v_writestring(opb,vi->user_comments[i]);
      }else{
	_oggpack_write(opb,0,32);
      }
    }
  }

  return(0);
}
 
static int _vorbis_pack_books(oggpack_buffer *opb,vorbis_info *vi){
  int i;
  _v_writestring(opb,"vorbis");
  _oggpack_write(opb,0x82,8);

  _oggpack_write(opb,vi->modes,16);
  for(i=0;i<vi->modes;i++){
    _oggpack_write(opb,vi->blockflags[i],1);
    _oggpack_write(opb,vi->maptypes[i],8);
    if(vi->maptypes[i]<0 || vi->maptypes[i]>VI_MAPB)goto err_out;
    vorbis_map_pack_P[vi->maptypes[i]](opb,vi->maplist[i]);
  }

  _oggpack_write(opb,vi->books,16);
  for(i=0;i<vi->books;i++)
    if(vorbis_book_pack(vi->booklist[i],opb))goto err_out;
  return(0);
err_out:
  return(-1);
} 

int vorbis_info_headerout(vorbis_info *vi,
			  ogg_packet *op,
			  ogg_packet *op_comm,
			  ogg_packet *op_code){

  oggpack_buffer opb;

  /* first header packet **********************************************/

  _oggpack_writeinit(&opb);
  if(_vorbis_pack_info(&opb,vi))goto err_out;

  /* build the packet */
  if(vi->header)free(vi->header);
  vi->header=malloc(_oggpack_bytes(&opb));
  memcpy(vi->header,opb.buffer,_oggpack_bytes(&opb));
  op->packet=vi->header;
  op->bytes=_oggpack_bytes(&opb);
  op->b_o_s=1;
  op->e_o_s=0;
  op->frameno=0;

  /* second header packet (comments) **********************************/

  _oggpack_reset(&opb);
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x81,8);
  if(_vorbis_pack_comments(&opb,vi))goto err_out;

  if(vi->header1)free(vi->header1);
  vi->header1=malloc(_oggpack_bytes(&opb));
  memcpy(vi->header1,opb.buffer,_oggpack_bytes(&opb));
  op_comm->packet=vi->header1;
  op_comm->bytes=_oggpack_bytes(&opb);
  op_comm->b_o_s=0;
  op_comm->e_o_s=0;
  op_comm->frameno=0;

  /* third header packet (modes/codebooks) ****************************/

  _oggpack_reset(&opb);
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x82,8);
  if(_vorbis_pack_books(&opb,vi))goto err_out;

  if(vi->header2)free(vi->header2);
  vi->header2=malloc(_oggpack_bytes(&opb));
  memcpy(vi->header2,opb.buffer,_oggpack_bytes(&opb));
  op_code->packet=vi->header2;
  op_code->bytes=_oggpack_bytes(&opb);
  op_code->b_o_s=0;
  op_code->e_o_s=0;
  op_code->frameno=0;

  _oggpack_writeclear(&opb);
  return(0);
 err_out:
  _oggpack_writeclear(&opb);
  memset(op,0,sizeof(ogg_packet));
  memset(op_comm,0,sizeof(ogg_packet));
  memset(op_code,0,sizeof(ogg_packet));
  return(-1);
}

