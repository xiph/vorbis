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

 function: illustrate seeking, and test it too
 last mod: $Id: seeking_example.c,v 1.2 2000/05/08 20:49:42 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include "vorbis/codec.h"
#include "vorbis/vorbisfile.h"
#include "../lib/misc.h"

int main(){
  OggVorbis_File ov;
  int i;

  /* open the file/pipe on stdin */
  if(ov_open(stdin,&ov,NULL,-1)==-1){
    printf("Could not open input as an OggVorbis file.\n\n");
    exit(1);
  }
  
  /* print details about each logical bitstream in the input */
  if(ov_seekable(&ov)){
    double length=ov_time_total(&ov,-1);
    printf("testing seeking to random places in %g seconds....\n",length);
    for(i=0;i<100;i++){
      ov_time_seek(&ov,drand48()*length);
      printf("\r\t%d...     ",i);
      fflush(stdout);
    }
    
    printf("\r                                   \nOK.\n\n");
  }else{
    printf("Standard input was not seekable.\n");
  }

  ov_clear(&ov);
  return 0;
}













