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

 function: simple example decoder using vorbisfile
 last mod: $Id: vorbisfile_example.c,v 1.1 2000/06/19 10:05:57 xiphmont Exp $

 ********************************************************************/

/* Takes a vorbis bitstream from stdin and writes raw stereo PCM to
   stdout using vorbisfile. Using vorbisfile is much simpler than
   dealing with libvorbis. */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"

#ifdef _WIN32 /* We need the following two to set stdin/stdout to binary */
#include <io.h>
#include <fcntl.h>
#endif

char pcmout[4096]; /* take 4k out of the data segment, not the stack */

int main(int argc, char **argv){
  OggVorbis_File vf;
  int eof=0;
  int current_section;

#ifdef _WIN32 /* We need to set stdin/stdout to binary mode. Damn windows. */
  /* Beware the evil ifdef. We avoid these where we can, but this one we 
     cannot. Don't add any more, you'll probably go to hell if you do. */
  _setmode( _fileno( stdin ), _O_BINARY );
  _setmode( _fileno( stdout ), _O_BINARY );
#endif

  if(ov_open(stdin, &vf, NULL, 0) < 0) {
      fprintf(stderr,"Input does not appear to be an Ogg bitstream.\n");
      exit(1);
  }

  /* Throw the comments plus a few lines about the bitstream we're
     decoding */
  {
    char **ptr=ov_comment(&vf,-1)->user_comments;
    vorbis_info *vi=ov_info(&vf,-1);
    while(*ptr){
      fprintf(stderr,"%s\n",*ptr);
      ++ptr;
    }
    fprintf(stderr,"\nBitstream is %d channel, %ldHz\n",vi->channels,vi->rate);
    fprintf(stderr,"Encoded by: %s\n\n",ov_comment(&vf,-1)->vendor);
  }
  
  while(!eof){
    long ret=ov_read(&vf,pcmout,sizeof(pcmout),0,2,1,&current_section);
    switch(ret){
    case 0:
      /* EOF */
      eof=1;
      break;
    case -1:
      /* error in the stream.  Not a problem, just reporting it in
	 case we (the app) cares.  In this case, we don't. */
      break;
    default:
      /* we don't bother dealing with sample rate changes, etc, but
	 you'll have to*/
      fwrite(pcmout,1,ret,stdout);
      break;
    }
  }

  /* cleanup */
  ov_clear(&vf);
    
  fprintf(stderr,"Done.\n");
  return(0);
}

