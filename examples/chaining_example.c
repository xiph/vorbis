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

 function: illustrate simple use of chained bitstream and vorbisfile.a
 last mod: $Id: chaining_example.c,v 1.5 2000/06/14 10:13:35 xiphmont Exp $

 ********************************************************************/

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
    printf("Input bitstream contained %ld logical bitstream section(s).\n",
	   ov_streams(&ov));
    printf("Total bitstream playing time: %ld seconds\n\n",
	   (long)ov_time_total(&ov,-1));

  }else{
    printf("Standard input was not seekable.\n"
	   "First logical bitstream information:\n\n");
  }

  for(i=0;i<ov_streams(&ov);i++){
    vorbis_info *vi=ov_info(&ov,i);
    printf("\tlogical bitstream section %d information:\n",i+1);
    printf("\t\t%ldHz %d channels bitrate %ldkbps serial number=%ld\n",
	   vi->rate,vi->channels,ov_bitrate(&ov,i)/1000,
	   ov_serialnumber(&ov,i));
    printf("\t\tcompressed length: %ld bytes ",(long)(ov_raw_total(&ov,i)));
    printf(" play time: %lds\n",(long)ov_time_total(&ov,i));
  }

  ov_clear(&ov);
  return 0;
}













