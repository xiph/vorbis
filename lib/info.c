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
 last mod: $Id: info.c,v 1.17 2000/01/22 13:28:21 xiphmont Exp $

 ********************************************************************/

/* general handling of the header and the vorbis_info structure (and
   substructures) */

#include <stdlib.h>
#include <string.h>
#include "vorbis/codec.h"
#include "bitwise.h"
#include "bookinternal.h"
#include "registry.h"
#include "window.h"
#include "psy.h"

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
int vorbis_dsp_addcomment(vorbis_dsp_state *v,char *comment){
  vorbis_info *vi=v->vi;
  vi->user_comments=realloc(vi->user_comments,
			    (vi->comments+2)*sizeof(char *));
  vi->user_comments[vi->comments]=strdup(comment);
  vi->comments++;
  vi->user_comments[vi->comments]=NULL;
  return(0);
}

/* used by analysis, which has a minimally replicated vi */
void _vorbis_info_anrep(vorbis_info *dest, vorbis_info *source){

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
  dest->vendor=NULL;

  /* modes, maps, times, floors, residues, psychoacoustics are not
     dupped; the pointer is just replicated by the above copy */

  /* dup (partially) books */
  if(source->books){
    dest->booklist=malloc(source->books*sizeof(codebook *));
    for(i=0;i<source->books;i++){
      dest->booklist[i]=calloc(1,sizeof(codebook));
      vorbis_book_dup(dest->booklist[i],source->booklist[i]);
    }
  }

  return(0);
err_out:
  vorbis_info_clear(dest);
  return(-1);
}


  memset(vi,0,sizeof(vorbis_info));
}

void vorbis_info_anclear(vorbis_info *vi){
  memset(vi,0,sizeof(vorbis_info));
}



/* used by synthesis, which has a full, alloced vi */
void vorbis_info_init(vorbis_info *vi){
  memset(vi,0,sizeof(vorbis_info));
}

void vorbis_info_clear(vorbis_info *vi){
  int i;
  if(vi->comments){
    for(i=0;i<vi->comments;i++)
      if(vi->user_comments[i])free(vi->user_comments[i]);
    free(vi->user_comments);
  }
  if(vi->vendor)free(vi->vendor);
  if(vi->windowtypes)free(vi->windowtypes);
  if(vi->transformtypes)free(vi->transformtypes);
  if(vi->modes){
    for(i=0;i<vi->modes;i++)
      if(vi->mappingtypes[i]>=0 && vi->mappingtypes[i]<VI_MAPB)
	vorbis_map_free_P[vi->mappingtypes[i]](vi->modelist[i]);
    free(vi->modelist);
    free(vi->mappingtypes);
    free(vi->blockflags);
  }
  if(vi->times){
    for(i=0;i<vi->times;i++)
      if(vi->timetypes[i]>=0 && vi->timetypes[i]<VI_TIMEB)
	vorbis_time_free_P[vi->timetypes[i]](vi->timelist[i]);
    free(vi->timelist);
    free(vi->timetypes);
  }
  if(vi->floors){
    for(i=0;i<vi->floors;i++)
      if(vi->floortypes[i]>=0 && vi->floortypes[i]<VI_FLOORB)
	vorbis_floor_free_P[vi->floortypes[i]](vi->floorlist[i]);
    free(vi->floorlist);
    free(vi->floortypes);
  }
  if(vi->residues){
    for(i=0;i<vi->residues;i++)
      if(vi->residuetypes[i]>=0 && vi->residuetypes[i]<VI_RESB)
	vorbis_res_free_P[vi->residuetypes[i]](vi->residuelist[i]);
    free(vi->residuelist);
    free(vi->residuetypes);
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
  if(vi->psys){
    for(i=0;i<vi->psys;i++)
      _vi_psy_free(vi->psylist[i]);
    free(vi->psylist);
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
  if(vi->blocksizes[1]<vi->blocksizes[0])goto err_out;
  
  if(_oggpack_read(opb,1)!=1)goto err_out; /* EOP check */

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
  if(_oggpack_read(opb,1)!=1)goto err_out; /* EOP check */

  return(0);
 err_out:
  vorbis_info_clear(vi);
  return(-1);
}

/* all of the real encoding details are here.  The modes, books,
   everything */
static int _vorbis_unpack_books(vorbis_info *vi,oggpack_buffer *opb){
  int i;

  /* time backend settings */
  vi->times=_oggpack_read(opb,8);
  vi->timetypes=malloc(vi->times*sizeof(int));
  vi->timelist=calloc(vi->times,sizeof(void *));
  for(i=0;i<vi->times;i++){
    vi->timetypes[i]=_oggpack_read(opb,16);
    if(vi->timetypes[i]<0 || vi->timetypes[i]>VI_TIMEB)goto err_out;
    vi->timelist[i]=vorbis_time_unpack_P[vi->timetypes[i]](vi,opb);
    if(!vi->timelist[i])goto err_out;
  }

  /* floor backend settings */
  vi->floors=_oggpack_read(opb,8);
  vi->floortypes=malloc(vi->floors*sizeof(int));
  vi->floorlist=calloc(vi->floors,sizeof(void *));
  for(i=0;i<vi->floors;i++){
    vi->floortypes[i]=_oggpack_read(opb,16);
    if(vi->floortypes[i]<0 || vi->floortypes[i]>VI_FLOORB)goto err_out;
    vi->floorlist[i]=vorbis_floor_unpack_P[vi->floortypes[i]](vi,opb);
    if(!vi->floorlist[i])goto err_out;
  }

  /* residue backend settings */
  vi->residues=_oggpack_read(opb,8);
  vi->residuetypes=malloc(vi->residues*sizeof(int));
  vi->residuelist=calloc(vi->residues,sizeof(void *));
  for(i=0;i<vi->residues;i++){
    vi->residuetypes[i]=_oggpack_read(opb,16);
    if(vi->residuetypes[i]<0 || vi->residuetypes[i]>VI_RESB)goto err_out;
    vi->residuelist[i]=vorbis_res_unpack_P[vi->residuetypes[i]](vi,opb);
    if(!vi->residuelist[i])goto err_out;
  }

  /* codebooks */
  vi->books=_oggpack_read(opb,16);
  vi->booklist=calloc(vi->books,sizeof(codebook *));
  for(i=0;i<vi->books;i++){
    vi->booklist[i]=calloc(1,sizeof(codebook));
    if(vorbis_book_unpack(opb,vi->booklist[i]))goto err_out;
  }

  /* modes/mappings; these are loaded last in order that the mappings
     can range-check their time/floor/res/book settings */
  vi->modes=_oggpack_read(opb,8);
  vi->blockflags=malloc(vi->modes*sizeof(int));
  vi->windowtypes=malloc(vi->modes*sizeof(int));
  vi->transformtypes=malloc(vi->modes*sizeof(int));
  vi->mappingtypes=malloc(vi->modes*sizeof(int));
  vi->modelist=calloc(vi->modes,sizeof(void *));
  for(i=0;i<vi->modes;i++){
    vi->blockflags[i]=_oggpack_read(opb,1);
    vi->windowtypes[i]=_oggpack_read(opb,16);
    vi->transformtypes[i]=_oggpack_read(opb,16);
    vi->mappingtypes[i]=_oggpack_read(opb,16);
    if(vi->windowtypes[i]<0 || vi->windowtypes[i]>VI_WINDOWB)goto err_out;
    if(vi->transformtypes[i]<0 || vi->transformtypes[i]>VI_TRANSFORMB)goto err_out;
    if(vi->mappingtypes[i]<0 || vi->mappingtypes[i]>VI_MAPB)goto err_out;
    vi->modelist[i]=vorbis_map_unpack_P[vi->mappingtypes[i]](vi,opb);
    if(!vi->modelist[i])goto err_out;
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
  _oggpack_write(opb,1,1);

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
  _oggpack_write(opb,1,1);

  return(0);
}
 
static int _vorbis_pack_books(oggpack_buffer *opb,vorbis_info *vi){
  int i;
  _v_writestring(opb,"vorbis");
  _oggpack_write(opb,0x82,8);

  /* times */
  _oggpack_write(opb,vi->times,8);
  for(i=0;i<vi->times;i++){
    _oggpack_write(opb,vi->timetypes[i],16);
    vorbis_time_pack_P[vi->timetypes[i]](opb,vi->timelist[i]);
  }

  /* floors */
  _oggpack_write(opb,vi->floors,8);
  for(i=0;i<vi->floors;i++){
    _oggpack_write(opb,vi->floortypes[i],16);
    vorbis_floor_pack_P[vi->floortypes[i]](opb,vi->floorlist[i]);
  }

  /* residues */
  _oggpack_write(opb,vi->residues,8);
  for(i=0;i<vi->residues;i++){
    _oggpack_write(opb,vi->residuetypes[i],16);
    vorbis_res_pack_P[vi->residuetypes[i]](opb,vi->residuelist[i]);
  }

  /* books */
  _oggpack_write(opb,vi->books,16);
  for(i=0;i<vi->books;i++)
    if(vorbis_book_pack(vi->booklist[i],opb))goto err_out;

  /* mode mappings */

  _oggpack_write(opb,vi->modes,8);
  for(i=0;i<vi->modes;i++){
    _oggpack_write(opb,vi->blockflags[i],1);
    _oggpack_write(opb,vi->windowtypes[i],16);
    _oggpack_write(opb,vi->transformtypes[i],16);
    _oggpack_write(opb,vi->mappingtypes[i],16);
    vorbis_map_pack_P[vi->mappingtypes[i]](vi,opb,vi->modelist[i]);
  }

  _oggpack_write(opb,1,1);

  return(0);
err_out:
  return(-1);
} 

int vorbis_analysis_headerout(vorbis_dsp_state *v,
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
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x81,8);
  if(_vorbis_pack_comments(&opb,vi))goto err_out;

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
  _v_writestring(&opb,"vorbis");
  _oggpack_write(&opb,0x82,8);
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
  return(-1);
}

