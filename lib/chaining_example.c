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

 function: simple example of opening/seeking chained bitstreams
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Nov 02 1999

 ********************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <math.h>
#include "codec.h"

/* A 'chained bitstream' is a Vorbis bitstream that contains more than
   one logical bitstream arranged end to end (the only form of Ogg
   multiplexing allowed in a Vorbis bitstream; grouping [parallel
   multiplexing] is not allowed in Vorbis) */

/* A Vorbis file can be played beginning to end (streamed) without
   worrying ahead of time about chaining (see decoder_example.c).  If
   we have the whole file, however, and want random access
   (seeking/scrubbing) or desire to know the total length/time of a
   file, we need to account for the possibility of chaining. */

/* We can handle things a number of ways; we can determine the entire
   bitstream structure right off the bat, or find pieces on demand.
   This example determines and caches structure for the entire
   bitstream, but builds a virtual decoder on the fly when moving
   between links in the chain */

/* There are also different ways to implement seeking.  Enough
   information exists in an Ogg bitstream to seek to
   sample-granularity positions in the output.  Or, one can seek by
   picking some portion of the stream roughtly in the area if we only
   want course navigation through the stream. */

typedef struct {
  FILE             *f;
  int              seekable;
  long             offset;
  long             end;
  ogg_sync_state   oy; 

  /* If the FILE handle isn't seekable (eg, a pipe), only the current
     stream appears */
  int              links;
  long             *offsets;
  long             *serialnos;
  size64           *pcmlengths;
  vorbis_info      *vi;

  /* Decoding working state local storage */
  int ready;
  ogg_stream_state os; /* take physical pages, weld into a logical
                          stream of packets */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */

} OggVorbis_File;


/*************************************************************************
 * Many, many internal helpers.  The intention is not to be confusing; 
 * rampant duplication and monolithic function implementation would be 
 * harder to understand anyway.  The high level functions are last.  Begin
 * grokking near the end of the file */

/* read a little more data from the file/pipe into the ogg_sync framer */
#define CHUNKSIZE 4096
static long _get_data(OggVorbis_File *vf){
  char *buffer=ogg_sync_buffer(&vf->oy,4096);
  long bytes=fread(buffer,1,4096,vf->f);
  ogg_sync_wrote(&vf->oy,bytes);
  return(bytes);
}

/* save a tiny smidge of verbosity to make the code more readable */
static void _seek_helper(OggVorbis_File *vf,long offset){
  fseek(vf->f,offset,SEEK_SET);
  vf->offset=offset;
  ogg_sync_reset(&vf->oy);
}

/* The read/seek functions track absolute position within the stream */

/* from the head of the stream, get the next page.  boundary specifies
   if the function is allowed to fetch more data from the stream (and
   how much) or only use internally buffered data.

   boundary: -1) unbounded search
              0) read no additional data; use cached only
	      n) search for a new page beginning for n bytes

   return:   -1) did not find a page 
              n) found a page at absolute offset n */

static long _get_next_page(OggVorbis_File *vf,ogg_page *og,int boundary){
  if(boundary>0)boundary+=vf->offset;
  while(1){
    long more;

    if(boundary>0 && vf->offset>=boundary)return(-1);
    more=ogg_sync_pageseek(&vf->oy,og);
    
    if(more<0){
      /* skipped n bytes */
      vf->offset-=more;
    }else{
      if(more==0){
	/* send more paramedics */
	if(!boundary)return(-1);
	if(_get_data(vf)<=0)return(-1);
      }else{
	/* got a page.  Return the offset at the page beginning,
           advance the internal offset past the page end */
	long ret=vf->offset;
	vf->offset+=more;
	return(ret);
	
      }
    }
  }
}

/* find the latest page beginning before the current stream cursor
   position. Much dirtier than the above as Ogg doesn't have any
   backward search linkage.  no 'readp' as it will certainly have to
   read. */
static long _get_prev_page(OggVorbis_File *vf,ogg_page *og){
  long begin=vf->offset;
  long ret;
  int offset=-1;

  while(offset==-1){
    begin-=CHUNKSIZE;
    _seek_helper(vf,begin);
    while(vf->offset<begin+CHUNKSIZE){
      ret=_get_next_page(vf,og,begin+CHUNKSIZE-vf->offset);
      if(ret==-1){
	break;
      }else{
	offset=ret;
      }
    }
  }

  /* we have the offset.  Actually snork and hold the page now */
  _seek_helper(vf,offset);
  ret=_get_next_page(vf,og,CHUNKSIZE);
  if(ret==-1){
    /* this shouldn't be possible */
    fprintf(stderr,"Missed page fencepost at end of logical bitstream. "
	    "Exiting.\n");
    exit(1);
  }
  return(offset);
}

/* finds each bitstream link one at a time using a bisection search
   (has to begin by knowing the offset of the lb's initial page).
   Recurses for each link so it can alloc the link storage after
   finding them all, then unroll and fill the cache at the same time */
static void _bisect_forward_serialno(OggVorbis_File *vf,
				     long begin,
				     long searched,
				     long end,
				     long currentno,
				     long m){
  long endsearched=end;
  long next=end;
  ogg_page og;
  
  /* the below guards against garbage seperating the last and
     first pages of two links. */
  while(searched<endsearched){
    long bisect;
    long ret;
    
    if(endsearched-searched<CHUNKSIZE){
      bisect=searched;
    }else{
      bisect=(searched+endsearched)/2;
    }
    
    _seek_helper(vf,bisect);
    ret=_get_next_page(vf,&og,-1);
    if(ret<0 || ogg_page_serialno(&og)!=currentno){
      endsearched=bisect;
      if(ret>=0)next=ret;
    }else{
      searched=ret+og.header_len+og.body_len;
    }
  }
  if(searched>=end){
    vf->links=m+1;
    vf->offsets=malloc((m+2)*sizeof(long));
    vf->offsets[m+1]=searched;
  }else{
    _bisect_forward_serialno(vf,next,next,
			     end,ogg_page_serialno(&og),m+1);
  }
  
  vf->offsets[m]=begin;
}

/* uses the local ogg_stream storage in vf; this is important for
   non-streaming input sources */
static int _fetch_headers(OggVorbis_File *vf,vorbis_info *vi,long *serialno){
  ogg_page og;
  ogg_packet op;
  int i,ret;

  ret=_get_next_page(vf,&og,CHUNKSIZE);
  if(ret==-1){
    fprintf(stderr,"Did not find initial header for bitstream.\n");
    return -1;
  }
  
  if(serialno)*serialno=ogg_page_serialno(&og);
  ogg_stream_init(&vf->os,ogg_page_serialno(&og));
  
  /* extract the initial header from the first page and verify that the
     Ogg bitstream is in fact Vorbis data */
  
  vorbis_info_init(vi);
  
  i=0;
  while(i<3){
    ogg_stream_pagein(&vf->os,&og);
    while(i<3){
      int result=ogg_stream_packetout(&vf->os,&op);
      if(result==0)break;
      if(result==-1){
	fprintf(stderr,"Corrupt header in logical bitstream.  "
		"Exiting.\n");
	goto bail_header;
      }
      if(vorbis_info_headerin(vi,&op)){
	fprintf(stderr,"Illegal header in logical bitstream.  "
		"Exiting.\n");
	goto bail_header;
      }
      i++;
    }
    if(i<3)
      if(_get_next_page(vf,&og,1)<0){
	fprintf(stderr,"Missing header in logical bitstream.  "
		"Exiting.\n");
	goto bail_header;
      }
  }
  return 0; 

 bail_header:
  vorbis_info_clear(vi);
  ogg_stream_clear(&vf->os);
  return -1;
}

/* last step of the OggVorbis_File initialization; get all the
   vorbis_info structs and PCM positions.  Only called by the seekable
   initialization (local stream storage is hacked slightly; pay
   attention to how that's done) */
static void _prefetch_all_headers(OggVorbis_File *vf,vorbis_info *first){
  ogg_page og;
  int i,ret;
  
  vf->vi=malloc(vf->links*sizeof(vorbis_info));
  vf->pcmlengths=malloc(vf->links*sizeof(size64));
  vf->serialnos=malloc(vf->links*sizeof(long));
  
  for(i=0;i<vf->links;i++){
    if(first && i==0){
      /* we already grabbed the initial header earlier.  This just
         saves the waste of grabbing it again */
      memcpy(vf->vi+i,first,sizeof(vorbis_info));
      memset(first,0,sizeof(vorbis_info));
    }else{

      /* seek to the location of the initial header */

      _seek_helper(vf,vf->offsets[i]);
      if(_fetch_headers(vf,vf->vi+i,NULL)==-1){
	vorbis_info_clear(vf->vi+i);
	fprintf(stderr,"Error opening logical bitstream #%d.\n\n",i+1);
    
	ogg_stream_clear(&vf->os); /* clear local storage.  This is not
				      done in _fetch_headers, as that may
				      be called in a non-seekable stream
				      (in which case, we need to preserve
				      the stream local storage) */
      }
    }

    /* get the serial number and PCM length of this link. To do this,
       get the last page of the stream */
    {
      long end=vf->offsets[i+1];
      _seek_helper(vf,end);

      while(1){
	ret=_get_prev_page(vf,&og);
	if(ret==-1){
	  /* this should not be possible */
	  fprintf(stderr,"Could not find last page of logical "
		  "bitstream #%d\n\n",i);
	  vorbis_info_clear(vf->vi+i);
	  break;
	}
	if(ogg_page_frameno(&og)!=-1){
	  vf->serialnos[i]=ogg_page_serialno(&og);
	  vf->pcmlengths[i]=ogg_page_frameno(&og);
	  break;
	}
      }
    }
  }
}

static int _open_seekable(OggVorbis_File *vf){
  vorbis_info initial;
  long serialno,end;
  int ret;
  ogg_page og;
  
  /* is this even vorbis...? */
  ret=_fetch_headers(vf,&initial,&serialno);
  ogg_stream_clear(&vf->os);
  if(ret==-1)return(-1);
  
  /* we can seek, so set out learning all about this file */
  vf->seekable=1;
  fseek(vf->f,0,SEEK_END); /* Yes, I know I used lseek earlier. */
  vf->offset=vf->end=ftell(vf->f);
  
  /* We get the offset for the last page of the physical bitstream.
     Most OggVorbis files will contain a single logical bitstream */
  end=_get_prev_page(vf,&og);

  /* moer than one logical bitstream? */
  if(ogg_page_serialno(&og)!=serialno){

    /* Chained bitstream. Bisect-search each logical bitstream
       section.  Do so based on serial number only */
    _bisect_forward_serialno(vf,0,0,end+1,serialno,0);

  }else{

    /* Only one logical bitstream */
    _bisect_forward_serialno(vf,0,end,end+1,serialno,0);

  }

  _prefetch_all_headers(vf,&initial);

  return(0);
}

static int _open_nonseekable(OggVorbis_File *vf){
  vorbis_info vi;
  long serialno;

  /* Try to fetch the headers, maintaining all the storage */
  if(_fetch_headers(vf,&vi,&serialno)==-1)return(-1);
  
  /* we cannot seek. Set up a 'single' (current) logical bitstream entry  */
  vf->links=1;
  vf->vi=malloc(sizeof(vorbis_info));
  vf->serialnos=malloc(sizeof(long));
  
  memcpy(vf->vi,&vi,sizeof(vorbis_info));
  vf->serialnos[0]=serialno;
  return 0;
}
 
/**********************************************************************
 * The helpers are over; it's all toplevel interface from here on out */
 
/* clear out the OggVorbis_File struct */
int ov_clear(OggVorbis_File *vf){
  if(vf){
    vorbis_block_clear(&vf->vb);
    vorbis_dsp_clear(&vf->vd);
    ogg_stream_clear(&vf->os);
    
    if(vf->vi && vf->links){
      int i;
      for(i=0;i<vf->links;i++)
	vorbis_info_clear(vf->vi+i);
      free(vf->vi);
    }
    if(vf->pcmlengths)free(vf->pcmlengths);
    if(vf->serialnos)free(vf->serialnos);
    if(vf->offsets)free(vf->offsets);
    ogg_sync_clear(&vf->oy);
    if(vf->f)fclose(vf->f);
    memset(vf,0,sizeof(OggVorbis_File));
  }
  return(0);
}

/* inspects the OggVorbis file and finds/documents all the logical
   bitstreams contained in it.  Tries to be tolerant of logical
   bitstream sections that are truncated/woogie. */
int ov_open(FILE *f,OggVorbis_File *vf,char *initial,long ibytes){
  long offset=lseek(fileno(f),0,SEEK_CUR);
  int ret;

  memset(vf,0,sizeof(OggVorbis_File));
  vf->f=f;

  /* init the framing state */
  ogg_sync_init(&vf->oy);

  /* perhaps some data was previously read into a buffer for testing
     against other stream types.  Allow initialization from this
     previously read data (as we may be reading from a non-seekable
     stream) */
  if(initial){
    char *buffer=ogg_sync_buffer(&vf->oy,ibytes);
    memcpy(buffer,initial,ibytes);
    ogg_sync_wrote(&vf->oy,ibytes);
  }

  /* can we seek? Stevens suggests the seek test was portable */
  if(offset!=-1){
    ret=_open_seekable(vf);
  }else{
    ret=_open_nonseekable(vf);
  }
  if(ret){
    vf->f=NULL;
    ov_clear(vf);
  }
  return(ret);
}

double ov_lbtime(OggVorbis_File *vf,int i){
  if(i>=0 && i<vf->links)
    return((float)(vf->pcmlengths[i])/vf->vi[i].rate);
  return(0);
}

long ov_totaltime(OggVorbis_File *vf){
  double acc=0;
  int i;
  for(i=0;i<vf->links;i++)
    acc+=ov_lbtime(vf,i);
  return(acc);
}

/* seek to an offset relative to the *compressed* data */
int ov_seek_stream;


/* seek to an offset relative to the decompressed *output* stream */
int ov_seek_pcm;


int main(){
  OggVorbis_File ov;
  int i;

  /* open the file/pipe on stdin */
  if(ov_open(stdin,&ov,NULL,-1)==-1){
    printf("Could not open input as an OggVorbis file.\n\n");
    exit(1);
  }
  
  /* print details about each logical bitstream in the input */
  if(ov.seekable){
    printf("Input bitstream contained %d logical bitstream section(s).\n",
	   ov.links);
    printf("Total bitstream playing time: %ld seconds\n\n",
	   (long)ov_totaltime(&ov));

  }else{
    printf("Standard input was not seekable.\n"
	   "First logical bitstream information:\n\n");
  }

  for(i=0;i<ov.links;i++){
    printf("\tlogical bitstream section %d information:\n",i+1);
    printf("\t\tsample rate: %ldHz\n",ov.vi[i].rate);
    printf("\t\tchannels: %d\n",ov.vi[i].channels);
    printf("\t\tserial number: %ld\n",ov.serialnos[i]);
    printf("\t\traw length: %ldbytes\n",(ov.offsets?
				     ov.offsets[i+1]-ov.offsets[i]:
				     -1));
    printf("\t\tplay time: %lds\n\n",(long)ov_lbtime(&ov,i));
  }
  
  ov_clear(&ov);
  return 0;
}





