/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *

 ********************************************************************

 function: flexible, delayed bitpacking abstraction
 last mod: $Id: bitbuffer.c,v 1.6 2001/10/02 00:14:30 segher Exp $

 ********************************************************************/

#include <string.h>
#include <ogg/ogg.h>
#include <vorbis/codec.h>
#include "misc.h"
#include "bitbuffer.h"

/* done carefully to do two things:
   1) no realloc
   2) draws from our exact-size vorbis_block pool
*/

void bitbuf_init(vorbis_bitbuffer *vbb,vorbis_block *vb){
  memset(vbb,0,sizeof(*vbb));
  vbb->vb=vb;
  vbb->first=vbb->last=_vorbis_block_alloc(vb,sizeof(*vbb->first));
  vbb->first->next=0; /* overengineering */
}

void bitbuf_write(vorbis_bitbuffer *vbb,unsigned long word,int length){
  vorbis_block *vb=vbb->vb;
  if(vbb->ptr>=_VBB_ALLOCSIZE){
    vbb->last->next=_vorbis_block_alloc(vb,sizeof(*vbb->last->next));
    vbb->last=vbb->last->next;
    vbb->last->next=0; /* overengineering */
    vbb->ptr=0;
  }
  vbb->last->words[vbb->ptr]=word;
  vbb->last->bits[vbb->ptr++]=length;
}

void bitbuf_pack(oggpack_buffer *dest,vorbis_bitbuffer *vbb){
  vorbis_bitbuffer_chain *vbc=vbb->first;
  int i;
  
  while(vbc->next){
    for(i=0;i<_VBB_ALLOCSIZE;i++)
      oggpack_write(dest,vbc->words[i],vbc->bits[i]);
    vbc=vbc->next;
  }
  for(i=0;i<vbb->ptr;i++)
    oggpack_write(dest,vbc->words[i],vbc->bits[i]);
}

/* codebook variants for encoding to the bitbuffer */

int vorbis_book_bufencode(codebook *book, int a, vorbis_bitbuffer *b){
  bitbuf_write(b,book->codelist[a],book->c->lengthlist[a]);
  return(book->c->lengthlist[a]);
}

