/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: illustrate seeking, and test it too
 last mod: $Id: seeking_example.c,v 1.6 2001/02/02 03:51:53 xiphmont Exp $

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
  if(ov_open(stdin,&ov,NULL,-1)<0){
    printf("Could not open input as an OggVorbis file.\n\n");
    exit(1);
  }
  
  /* print details about each logical bitstream in the input */
  if(ov_seekable(&ov)){
    double length=ov_time_total(&ov,-1);
    printf("testing seeking to random places in %g seconds....\n",length);
    for(i=0;i<100;i++){
      double val=(double)rand()/RAND_MAX*length;
      ov_time_seek(&ov,val);
      printf("\r\t%d [%gs]...     ",i,val);
      fflush(stdout);
    }
    
    printf("\r                                   \nOK.\n\n");
  }else{
    printf("Standard input was not seekable.\n");
  }

  ov_clear(&ov);
  return 0;
}













