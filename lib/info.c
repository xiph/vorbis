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
 last modification date: Oct 02 1999

 ********************************************************************/

/* This fills in a vorbis_info structure with settings from a few
   pre-defined encoding modes.  Also handles choosing/blowing in the
   codebook */

#include <stdlib.h>
#include <string.h>
#include "modes.h"

/* one test mode for now; temporary of course */
int vorbis_info_modeset(vorbis_info *vi, int mode){
  if(mode<0 || mode>predef_mode_max)return(-1);

  /* handle the flat settings first */
  memcpy(vi,&(predef_modes[mode]),sizeof(vorbis_info));
  vi->user_comments=calloc(1,sizeof(char *));

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

/* The Vorbis header is in three packets; the initial small packet in
   the first page that identifies basic parameters, a second
   [optional] packet with bitstream comments and a third packet that
   holds the codebook. */

int vorbis_info_headerin(vorbis_info *vi,ogg_packet *op){

}

int vorbis_info_headerout(vorbis_info *vi,ogg_packet *op){


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

  memset(vi,0,sizeof(vorbis_info));
}
  

