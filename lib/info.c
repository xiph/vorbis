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
 last mod: $Id: info.c,v 1.24 2000/05/08 20:49:48 xiphmont Exp $

 ********************************************************************/

/* general handling of the header and the vorbis_info structure (and
   substructures) */

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "vorbis/backends.h"
#include "bitwise.h"
#include "sharedbook.h"
#include "bookinternal.h"
#include "registry.h"
#include "window.h"
#include "psy.h"
#include "misc.h"

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

void vorbis_comment_init(vorbis_comment *vc){
  memset(vc,0,sizeof(vorbis_comment));
}

void vorbis_comment_add(vorbis_comment *vc,char *comment){
  vc->user_comments=realloc(vc->user_comments,
			    (vc->comments+2)*sizeof(char *));
  vc->user_comments[vc->comments]=strdup(comment);
  vc->comments++;
  vc->user_comments[vc->comments]=NULL;
}

void vorbis_comment_clear(vorbis_comment *vc){
  if(vc){
    long i;
    for(i=0;i<vc->comments;i++)
      if(vc->user_comments[i])free(vc->user_comments[i]);
    if(vc->user_comments)free(vc->user_comments);
    if(vc->vendor)free(vc->vendor);
  }
  memset(vc,0,sizeof(vorbis_comment));
}

/* used by synthesis, which has a full, alloced vi */
void vorbis_info_init(vorbis_info *vi){
  memset(vi,0,sizeof(vorbis_info));
}

void vorbis_info_clear(vorbis_info *vi){
  int i;

  for(i=0;i<vi->modes;i++)
    if(vi->mode_param[i])free(vi->mode_param[i]);
  /*if(vi->mode_param)free(vi->mode_param);*/
 
  for(i=0;i<vi->maps;i++) /* unpack does the range checking */
    _mapping_P[vi->map_type[i]]->free_info(vi->map_param[i]);
  /*if(vi->map_param)free(vi->map_param);*/
    
  for(i=0;i<vi->times;i++) /* unpack does the range checking */
    _time_P[vi->time_type[i]]->free_info(vi->time_param[i]);
  /*if(vi->time_param)free(vi->time_param);*/
    
  for(i=0;i<vi->floors;i++) /* unpack does the range checking */
    _floor_P[vi->floor_type[i]]->free_info(vi->floor_param[i]);
  /*if(vi->floor_param)free(vi->floor_param);*/
    
  for(i=0;i<vi->residues;i++) /* unpack does the range checking */
    _residue_P[vi->residue_type[i]]->free_info(vi->residue_param[i]);
  /*if(vi->residue_param)free(vi->residue_param);*/

  /* the static codebooks *are* freed if you call info_clear, because
     decode side does alloc a 'static' codebook. Calling clear on the
     full codebook does not clear the static codebook (that's our
     responsibility) */
  for(i=0;i<vi->books;i++){
    /* just in case the decoder pre-cleared to save space */
    if(vi->book_param[i]){
      vorbis_staticbook_clear(vi->book_param[i]);
      free(vi->book_param[i]);
    }
  }
  /*if(vi->book_param)free(vi->book_param);*/

  for(i=0;i<vi->psys;i++)
    _vi_psy_free(vi->psy_param[i]);
  /*if(vi->psy_param)free(vi->psy_param);*/
  
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
  if(vi->blocksizes[1]<vi->blocksizes[0])goto err_out;
  
  if(_oggpack_read(opb,1)!=1)goto err_out; /* EOP check */

  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(-1);
}

static int _vorbis_unpack_comment(vorbis_comment *vc,oggpack_buffer *opb){
  int i;
  int vendorlen=_oggpack_read(opb,32);
  if(vendorlen<0)goto err_out;
  vc->vendor=calloc(vendorlen+1,1);
  _v_readstring(opb,vc->vendor,vendorlen);
  vc->comments=_oggpack_read(opb,32);
  if(vc->comments<0)goto err_out;
  vc->user_comments=calloc(vc->comments+1,sizeof(char **));
	    
  for(i=0;i<vc->comments;i++){
    int len=_oggpack_read(opb,32);
    if(len<0)goto err_out;
    vc->user_comments[i]=calloc(len+1,1);
    _v_readstring(opb,vc->user_comments[i],len);
  }	  
  if(_oggpack_read(opb,1)!=1)goto err_out; /* EOP check */

  return(0);
 err_out:
  vorbis_comment_clear(vc);
  return(-1);
}

/* all of the real encoding details are here.  The modes, books,
   everything */
static int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb){
  int i;

  /* codebooks */
  vi->books=_oggpack_read(opb,8)+1;
  /*vi->book_param=calloc(vi->books,sizeof(static_codebook *));*/
  for(i=0;i<vi->books;i++){
    vi->book_param[i]=calloc(1,sizeof(static_codebook));
    if(vorbis_staticbook_unpack(opb,vi->book_param[i]))goto err_out;
  }

  /* time backend settings */
  vi->times=_oggpack_read(opb,6)+1;
  /*vi->time_type=malloc(vi->times*sizeof(int));*/
  /*vi->time_param=calloc(vi->times,sizeof(void *));*/
  for(i=0;i<vi->times;i++){
    vi->time_type[i]=_oggpack_read(opb,16);
    if(vi->time_type[i]<0 || vi->time_type[i]>=VI_TIMEB)goto err_out;
    vi->time_param[i]=_time_P[vi->time_type[i]]->unpack(vi,opb);
    if(!vi->time_param[i])goto err_out;
  }

  /* floor backend settings */
  vi->floors=_oggpack_read(opb,6)+1;
  /*vi->floor_type=malloc(vi->floors*sizeof(int));*/
  /*vi->floor_param=calloc(vi->floors,sizeof(void *));*/
  for(i=0;i<vi->floors;i++){
    vi->floor_type[i]=_oggpack_read(opb,16);
    if(vi->floor_type[i]<0 || vi->floor_type[i]>=VI_FLOORB)goto err_out;
    vi->floor_param[i]=_floor_P[vi->floor_type[i]]->unpack(vi,opb);
    if(!vi->floor_param[i])goto err_out;
  }

  /* residue backend settings */
  vi->residues=_oggpack_read(opb,6)+1;
  /*vi->residue_type=malloc(vi->residues*sizeof(int));*/
  /*vi->residue_param=calloc(vi->residues,sizeof(void *));*/
  for(i=0;i<vi->residues;i++){
    vi->residue_type[i]=_oggpack_read(opb,16);
    if(vi->residue_type[i]<0 || vi->residue_type[i]>=VI_RESB)goto err_out;
    vi->residue_param[i]=_residue_P[vi->residue_type[i]]->unpack(vi,opb);
    if(!vi->residue_param[i])goto err_out;
  }

  /* map backend settings */
  vi->maps=_oggpack_read(opb,6)+1;
  /*vi->map_type=malloc(vi->maps*sizeof(int));*/
  /*vi->map_param=calloc(vi->maps,sizeof(void *));*/
  for(i=0;i<vi->maps;i++){
    vi->map_type[i]=_oggpack_read(opb,16);
    if(vi->map_type[i]<0 || vi->map_type[i]>=VI_MAPB)goto err_out;
    vi->map_param[i]=_mapping_P[vi->map_type[i]]->unpack(vi,opb);
    if(!vi->map_param[i])goto err_out;
  }
  
  /* mode settings */
  vi->modes=_oggpack_read(opb,6)+1;
  /*vi->mode_param=calloc(vi->modes,sizeof(void *));*/
  for(i=0;i<vi->modes;i++){
    vi->mode_param[i]=calloc(1,sizeof(vorbis_info_mode));
    vi->mode_param[i]->blockflag=_oggpack_read(opb,1);
    vi->mode_param[i]->windowtype=_oggpack_read(opb,16);
    vi->mode_param[i]->transformtype=_oggpack_read(opb,16);
    vi->mode_param[i]->mapping=_oggpack_read(opb,8);

    if(vi->mode_param[i]->windowtype>=VI_WINDOWB)goto err_out;
    if(vi->mode_param[i]->transformtype>=VI_WINDOWB)goto err_out;
    if(vi->mode_param[i]->mapping>=vi->maps)goto err_out;
  }
  
  if(_oggpack_read(opb,1)!=1)goto err_out; /* top level EOP check */

  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(-1);
}

/* The Vorbis header is in three packets; the initial small packet in
   the first page that identifies basic parameters, a second packet
   with bitstream comments and a third packet that holds the
   codebook. */

int vorbis_synthesis_headerin(vorbis_info *vi,vorbis_comment *vc,ogg_packet *op){
  oggpack_buffer opb;
  
  if(op){
    _oggpack_readinit(&opb,op->packet,op->bytes);

    /* Which of the three types of header is this? */
    /* Also verify header-ness, vorbis */
    {
      char buffer[6];
      int packtype=_oggpack_read(&opb,8);
      memset(buffer,0,6);
      _v_readstring(&opb,buffer,6);
      if(memcmp(buffer,"vorbis",6)){
	/* not a vorbis header */
	return(-1);
      }
      switch(packtype){
      case 0x01: /* least significant *bit* is read first */
	if(!op->b_o_s){
	  /* Not the initial packet */
	  return(-1);
	}
	if(vi->rate!=0){
	  /* previously initialized info header */
	  return(-1);
	}

	return(_vorbis_unpack_info(vi,&opb));

      case 0x03: /* least significant *bit* is read first */
	if(vi->rate==0){
	  /* um... we didn't get the initial header */
	  return(-1);
	}

	return(_vorbis_unpack_comment(vc,&opb));

      case 0x05: /* least significant *bit* is read first */
	if(vi->rate==0 || vc->vendor==NULL){
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
  _oggpack_write(opb,0x01,8);
  _v_writestring(opb,"vorbis");

  /* basic information about the stream */
  _oggpack_write(opb,0x00,32);
  _oggpack_write(opb,vi->channels,8);
  _oggpack_write(opb,vi->rate,32);

  _oggpack_write(opb,vi->bitrate_upper,32);
  _oggpack_write(opb,vi->bitrate_nominal,32);
  _oggpack_write(opb,vi->bitrate_lower,32);

  _oggpack_write(opb,ilog2(vi->blocksizes[0]),4);
  _oggpack_write(opb,ilog2(vi->blocksizes[1]),4);
  _oggpack_write(opb,1,1);

  return(0);
}

static int _vorbis_pack_comment(oggpack_buffer *opb,vorbis_comment *vc){
  char temp[]="Xiphophorus libVorbis I 20000508";

  /* preamble */  
  _oggpack_write(opb,0x03,8);
  _v_writestring(opb,"vorbis");

  /* vendor */
  _oggpack_write(opb,strlen(temp),32);
  _v_writestring(opb,temp);
  
  /* comments */

  _oggpack_write(opb,vc->comments,32);
  if(vc->comments){
    int i;
    for(i=0;i<vc->comments;i++){
      if(vc->user_comments[i]){
	_oggpack_write(opb,strlen(vc->user_comments[i]),32);
	_v_writestring(opb,vc->user_comments[i]);
      }else{
	_oggpack_write(opb,0,32);
      }
    }
  }
  _oggpack_write(opb,1,1);

  return(0);
}
 
static int _vorbis_pack_books(oggpack_buffer *opb,vorbis_info *vi){
  int i;
  _oggpack_write(opb,0x05,8);
  _v_writestring(opb,"vorbis");

  /* books */
  _oggpack_write(opb,vi->books-1,8);
  for(i=0;i<vi->books;i++)
    if(vorbis_staticbook_pack(vi->book_param[i],opb))goto err_out;

  /* times */
  _oggpack_write(opb,vi->times-1,6);
  for(i=0;i<vi->times;i++){
    _oggpack_write(opb,vi->time_type[i],16);
    _time_P[vi->time_type[i]]->pack(vi->time_param[i],opb);
  }

  /* floors */
  _oggpack_write(opb,vi->floors-1,6);
  for(i=0;i<vi->floors;i++){
    _oggpack_write(opb,vi->floor_type[i],16);
    _floor_P[vi->floor_type[i]]->pack(vi->floor_param[i],opb);
  }

  /* residues */
  _oggpack_write(opb,vi->residues-1,6);
  for(i=0;i<vi->residues;i++){
    _oggpack_write(opb,vi->residue_type[i],16);
    _residue_P[vi->residue_type[i]]->pack(vi->residue_param[i],opb);
  }

  /* maps */
  _oggpack_write(opb,vi->maps-1,6);
  for(i=0;i<vi->maps;i++){
    _oggpack_write(opb,vi->map_type[i],16);
    _mapping_P[vi->map_type[i]]->pack(vi,vi->map_param[i],opb);
  }

  /* modes */
  _oggpack_write(opb,vi->modes-1,6);
  for(i=0;i<vi->modes;i++){
    _oggpack_write(opb,vi->mode_param[i]->blockflag,1);
    _oggpack_write(opb,vi->mode_param[i]->windowtype,16);
    _oggpack_write(opb,vi->mode_param[i]->transformtype,16);
    _oggpack_write(opb,vi->mode_param[i]->mapping,8);
  }
  _oggpack_write(opb,1,1);

  return(0);
err_out:
  return(-1);
} 

int vorbis_analysis_headerout(vorbis_dsp_state *v,
			      vorbis_comment *vc,
			      ogg_packet *op,
			      ogg_packet *op_comm,
			      ogg_packet *op_code){
  vorbis_info *vi=v->vi;
  oggpack_buffer opb;

  /* first header packet **********************************************/

  _oggpack_writeinit(&opb);
  if(_vorbis_pack_info(&opb,vi))goto err_out;

  /* build the packet */
  if(v->header)free(v->header);
  v->header=malloc(_oggpack_bytes(&opb));
  memcpy(v->header,opb.buffer,_oggpack_bytes(&opb));
  op->packet=v->header;
  op->bytes=_oggpack_bytes(&opb);
  op->b_o_s=1;
  op->e_o_s=0;
  op->frameno=0;

  /* second header packet (comments) **********************************/

  _oggpack_reset(&opb);
  if(_vorbis_pack_comment(&opb,vc))goto err_out;

  if(v->header1)free(v->header1);
  v->header1=malloc(_oggpack_bytes(&opb));
  memcpy(v->header1,opb.buffer,_oggpack_bytes(&opb));
  op_comm->packet=v->header1;
  op_comm->bytes=_oggpack_bytes(&opb);
  op_comm->b_o_s=0;
  op_comm->e_o_s=0;
  op_comm->frameno=0;

  /* third header packet (modes/codebooks) ****************************/

  _oggpack_reset(&opb);
  if(_vorbis_pack_books(&opb,vi))goto err_out;

  if(v->header2)free(v->header2);
  v->header2=malloc(_oggpack_bytes(&opb));
  memcpy(v->header2,opb.buffer,_oggpack_bytes(&opb));
  op_code->packet=v->header2;
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

  if(v->header)free(v->header);
  if(v->header1)free(v->header1);
  if(v->header2)free(v->header2);
  v->header=NULL;
  v->header1=NULL;
  v->header2=NULL;
  return(-1);
}

