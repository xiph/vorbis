/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: bitrate tracking and management
 last mod: $Id: bitrate.c,v 1.3 2001/12/14 07:21:16 xiphmont Exp $

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "os.h"
#include "bitrate.h"

#define BINBITS(pos,bin) ((bin)>0?bm->queue_binned[(pos)*bins+(bin)-1]:0)
#define LIMITBITS(pos,bin) ((bin)>-bins?\
                 bm->minmax_binstack[(pos)*bins*2+((bin)+bins)-1]:0)

static long LACING_ADJUST(long bits){
  int addto=((bits+7)/8+1)/256+1;
  return( ((bits+7)/8+addto)*8 );
}

static double floater_interpolate(bitrate_manager_state *bm,vorbis_info *vi,
				  double desired_rate){
  int bin=bm->avgfloat*BITTRACK_DIVISOR-1.;
  double lobitrate;
  double hibitrate;
  
  lobitrate=(double)(bin==0?0:bm->avg_binacc[bin-1])/bm->avg_sampleacc*vi->rate;
  while(lobitrate>desired_rate && bin>0){
    bin--;
    lobitrate=(double)(bin==0?0:bm->avg_binacc[bin-1])/bm->avg_sampleacc*vi->rate;
  }

  hibitrate=(double)(bin>=bm->queue_bins?bm->avg_binacc[bm->queue_bins-1]:
		     bm->avg_binacc[bin])/bm->avg_sampleacc*vi->rate;
  while(hibitrate<desired_rate && bin<bm->queue_bins){
    bin++;
    if(bin<bm->queue_bins)
      hibitrate=(double)bm->avg_binacc[bin]/bm->avg_sampleacc*vi->rate;
  }

  /* interpolate */
  if(bin==bm->queue_bins){
    return bin/(double)BITTRACK_DIVISOR;
  }else{
    double delta=(desired_rate-lobitrate)/(hibitrate-lobitrate);
    return (bin+delta)/(double)BITTRACK_DIVISOR;
  }
}

/* try out a new limit */
static long limit_sum(bitrate_manager_state *bm,int limit){
  int i=bm->minmax_stackptr;
  long acc=bm->minmax_acctotal;
  long bins=bm->queue_bins;
  
  acc-=LIMITBITS(i,0);
  acc+=LIMITBITS(i,limit);

  while(i-->0){
    if(bm->minmax_limitstack[i]<=limit)break;
    acc-=LIMITBITS(i,bm->minmax_limitstack[i]);
    acc+=LIMITBITS(i,limit);
  }
  return(acc);
}

/* compute bitrate tracking setup, allocate circular packet size queue */
void vorbis_bitrate_init(vorbis_info *vi,bitrate_manager_state *bm){
  int i;
  codec_setup_info *ci=vi->codec_setup;
  bitrate_manager_info *bi=&ci->bi;
  long maxlatency;

  memset(bm,0,sizeof(*bm));
  
  if(bi){
    
    bm->avg_sampledesired=bi->queue_avg_time*vi->rate;
    bm->avg_centerdesired=bi->queue_avg_time*vi->rate*bi->queue_avg_center;
    bm->minmax_sampledesired=bi->queue_minmax_time*vi->rate;
    
    /* first find the max possible needed queue size */
    maxlatency=max(bm->avg_sampledesired-bm->avg_centerdesired,
		   bm->minmax_sampledesired)+bm->avg_centerdesired;
    
    if(maxlatency>0 &&
       (bi->queue_avgmin>0 || bi->queue_avgmax>0 || bi->queue_hardmax>0 ||
	bi->queue_hardmin>0)){
      long maxpackets=maxlatency/(ci->blocksizes[0]>>1)+3;
      long bins=BITTRACK_DIVISOR*ci->passlimit[ci->coupling_passes-1];
      
      bm->queue_size=maxpackets;
      bm->queue_bins=bins;
      bm->queue_binned=_ogg_malloc(maxpackets*bins*sizeof(*bm->queue_binned));
      bm->queue_actual=_ogg_malloc(maxpackets*sizeof(*bm->queue_actual));
      
      if((bi->queue_avgmin>0 || bi->queue_avgmax>0) &&
	 bi->queue_avg_time>0){
	
	bm->avg_binacc=_ogg_malloc(bins*sizeof(*bm->avg_binacc));
	bm->avgfloat=bi->avgfloat_initial;
	
	
      }else{
	bm->avg_tail= -1;
      }
      
      if((bi->queue_hardmin>0 || bi->queue_hardmax>0) &&
	 bi->queue_minmax_time>0){
	
	bm->minmax_binstack=_ogg_malloc((bins+1)*bins*2*
					sizeof(bm->minmax_binstack));
	bm->minmax_posstack=_ogg_malloc((bins+1)*
				      sizeof(bm->minmax_posstack));
	bm->minmax_limitstack=_ogg_malloc((bins+1)*
					  sizeof(bm->minmax_limitstack));
      }else{
	bm->minmax_tail= -1;
      }
      
      /* space for the packet queueing */
      bm->queue_packet_buffers=calloc(maxpackets,sizeof(*bm->queue_packet_buffers));
      bm->queue_packets=calloc(maxpackets,sizeof(*bm->queue_packets));
      for(i=0;i<maxpackets;i++)
	oggpack_writeinit(bm->queue_packet_buffers+i);
      
    }else{
      bm->queue_packet_buffers=calloc(1,sizeof(*bm->queue_packet_buffers));
      bm->queue_packets=calloc(1,sizeof(*bm->queue_packets));
      oggpack_writeinit(bm->queue_packet_buffers);
    }      
  }
}

void vorbis_bitrate_clear(bitrate_manager_state *bm){
  int i;
  if(bm){
    if(bm->queue_binned)_ogg_free(bm->queue_binned);
    if(bm->queue_actual)_ogg_free(bm->queue_actual);
    if(bm->avg_binacc)_ogg_free(bm->avg_binacc);
    if(bm->minmax_binstack)_ogg_free(bm->minmax_binstack);
    if(bm->minmax_posstack)_ogg_free(bm->minmax_posstack);
    if(bm->minmax_limitstack)_ogg_free(bm->minmax_limitstack);
    if(bm->queue_packet_buffers){
      if(bm->queue_size==0){
	oggpack_writeclear(bm->queue_packet_buffers);
	_ogg_free(bm->queue_packet_buffers);
      }else{
	for(i=0;i<bm->queue_size;i++)
	  oggpack_writeclear(bm->queue_packet_buffers+i);
	_ogg_free(bm->queue_packet_buffers);
      }
    }
    if(bm->queue_packets)_ogg_free(bm->queue_packets);
    memset(bm,0,sizeof(*bm));
  }
}

int vorbis_bitrate_managed(vorbis_block *vb){
  vorbis_dsp_state      *vd=vb->vd;
  backend_lookup_state  *b=vd->backend_state; 
  bitrate_manager_state *bm=&b->bms;

  if(bm->queue_binned)return(1);
  return(0);
}

int vorbis_bitrate_maxmarkers(void){
  return 8*BITTRACK_DIVISOR;
}

/* finish taking in the block we just processed */
int vorbis_bitrate_addblock(vorbis_block *vb){
  int i; 
  vorbis_block_internal *vbi=vb->internal;
  vorbis_dsp_state      *vd=vb->vd;
  backend_lookup_state  *b=vd->backend_state; 
  bitrate_manager_state *bm=&b->bms;
  vorbis_info           *vi=vd->vi;
  codec_setup_info      *ci=vi->codec_setup;
  bitrate_manager_info  *bi=&ci->bi;
  int                    eofflag=vb->eofflag;
  int                    head=bm->queue_head;
  int                    next_head=head+1;
  int                    bins=bm->queue_bins;
  int                    minmax_head,new_minmax_head;
  
  ogg_uint32_t           *head_ptr;
  oggpack_buffer          temp;

  if(!bm->queue_binned){
    oggpack_buffer temp;
    /* not a bitrate managed stream, but for API simplicity, we'll
       buffer one packet to keep the code path clean */
    
    if(bm->queue_head)return(-1); /* one has been submitted without
                                     being claimed */
    bm->queue_head++;

    bm->queue_packets[0].packet=oggpack_get_buffer(&vb->opb);
    bm->queue_packets[0].bytes=oggpack_bytes(&vb->opb);
    bm->queue_packets[0].b_o_s=0;
    bm->queue_packets[0].e_o_s=vb->eofflag;
    bm->queue_packets[0].granulepos=vb->granulepos;
    bm->queue_packets[0].packetno=vb->sequence; /* for sake of completeness */

    memcpy(&temp,bm->queue_packet_buffers,sizeof(vb->opb));
    memcpy(bm->queue_packet_buffers,&vb->opb,sizeof(vb->opb));
    memcpy(&vb->opb,&temp,sizeof(vb->opb));

    return(0);
  }

  /* add encoded packet to head */
  if(next_head>=bm->queue_size)next_head=0;
  head_ptr=bm->queue_binned+bins*head;

  /* is there room to add a block? In proper use of the API, this will
     never come up... but guard it anyway */
  if(next_head==bm->avg_tail || next_head==bm->minmax_tail)return(-1);

  /* add the block to the toplevel queue */
  bm->queue_head=next_head;
  bm->queue_actual[head]=(vb->W?0x80000000UL:0);

  /* buffer packet fields */
  bm->queue_packets[head].packet=oggpack_get_buffer(&vb->opb);
  bm->queue_packets[head].bytes=oggpack_bytes(&vb->opb);
  bm->queue_packets[head].b_o_s=0;
  bm->queue_packets[head].e_o_s=vb->eofflag;
  bm->queue_packets[head].granulepos=vb->granulepos;
  bm->queue_packets[head].packetno=vb->sequence; /* for sake of completeness */

  /* swap packet buffers */
  memcpy(&temp,bm->queue_packet_buffers+head,sizeof(vb->opb));
  memcpy(bm->queue_packet_buffers+head,&vb->opb,sizeof(vb->opb));
  memcpy(&vb->opb,&temp,sizeof(vb->opb));

  /* save markers */
  memcpy(head_ptr,vbi->packet_markers,sizeof(*head_ptr)*bins);

  if(bm->avg_binacc)
    new_minmax_head=minmax_head=bm->avg_center;
  else
    new_minmax_head=minmax_head=head;

  /* the average tracking queue is updated first; its results (if it's
     in use) are taken into account by the min/max limiter (if min/max
     is in use) */
  if(bm->avg_binacc){
    long desired_center=bm->avg_centerdesired;
    if(eofflag)desired_center=0;

    /* update the avg head */
    for(i=0;i<bins;i++)
      bm->avg_binacc[i]+=LACING_ADJUST(head_ptr[i]);
    bm->avg_sampleacc+=ci->blocksizes[vb->W]>>1;
    bm->avg_centeracc+=ci->blocksizes[vb->W]>>1;

    /* update the avg tail if needed */
    while(bm->avg_sampleacc>bm->avg_sampledesired){
      int samples=
	ci->blocksizes[bm->queue_actual[bm->avg_tail]&0x80000000UL?1:0]>>1;
      for(i=0;i<bm->queue_bins;i++)
	bm->avg_binacc[i]-=LACING_ADJUST(bm->queue_binned[bins*bm->avg_tail+i]);
      bm->avg_sampleacc-=samples;
      bm->avg_tail++;
      if(bm->avg_tail>=bm->queue_size)bm->avg_tail=0;
    }

    /* update the avg center */
    if(bm->avg_centeracc>desired_center){
      /* choose the new average floater */
      double upper=floater_interpolate(bm,vi,bi->queue_avgmax);
      double lower=floater_interpolate(bm,vi,bi->queue_avgmin);
      double new=bi->avgfloat_initial,slew;
      int bin;

      if(upper>0. && upper<new)new=upper;
      if(lower<bi->avgfloat_minimum)
        lower=bi->avgfloat_minimum;
      if(lower>new)new=lower;

      slew=new-bm->avgfloat;

      if(slew<bi->avgfloat_downhyst || slew>bi->avgfloat_uphyst){
        if(slew<bi->avgfloat_downslew_max)
          new=bm->avgfloat+bi->avgfloat_downslew_max;
        if(slew>bi->avgfloat_upslew_max)
          new=bm->avgfloat+bi->avgfloat_upslew_max;
        
        bm->avgfloat=new;
      }
      
      /* apply the average floater to new blocks */
      bin=bm->avgfloat*BITTRACK_DIVISOR; /* truncate on purpose */
      fprintf(stderr,"float:%d ",bin);
      while(bm->avg_centeracc>desired_center){
	int samples=
	  samples=ci->blocksizes[bm->queue_actual[bm->avg_center]&
				0x80000000UL?1:0]>>1;
	
	bm->queue_actual[bm->avg_center]|=bin;
	
	bm->avg_centeracc-=samples;
	bm->avg_center++;
	if(bm->noisetrigger_postpone)bm->noisetrigger_postpone-=samples;
	if(bm->avg_center>=bm->queue_size)bm->avg_center=0;
      }
      new_minmax_head=bm->avg_center;
      
      /* track noise bias triggers and noise bias */
      if(bm->avgfloat<bi->avgfloat_noise_lowtrigger)
	bm->noisetrigger_request+=1.f;
      
      if(bm->avgfloat>bi->avgfloat_noise_hightrigger)
	bm->noisetrigger_request-=1.f;
      
      if(bm->noisetrigger_postpone<=0){
	if(bm->noisetrigger_request<0.){
	  bm->avgnoise-=1.f;
	  if(bm->noisetrigger_request<bm->avg_sampleacc/2)
            bm->avgnoise-=1.f;
	  bm->noisetrigger_postpone=bm->avg_sampleacc/2;
	}
	if(bm->noisetrigger_request>0.){
	  bm->avgnoise+=1.f;
	  if(bm->noisetrigger_request>bm->avg_sampleacc/2)
	    bm->avgnoise+=1.f;
	  bm->noisetrigger_postpone=bm->avg_sampleacc/2;
	}

	/* we generally want the noise bias to drift back to zero */
	bm->noisetrigger_request=0.f;
	if(bm->avgnoise>0)
	  bm->noisetrigger_request= -1.;
	if(bm->avgnoise<0)
	  bm->noisetrigger_request= +1.;

	if(bm->avgnoise<bi->avgfloat_noise_minval)
	  bm->avgnoise=bi->avgfloat_noise_minval;
	if(bm->avgnoise>bi->avgfloat_noise_maxval)
	  bm->avgnoise=bi->avgfloat_noise_maxval;
      }
      fprintf(stderr,"noise:%f req:%ld trigger:%ld\n",bm->avgnoise,
	      bm->noisetrigger_request,bm->noisetrigger_postpone);

    }
  }else{
    /* if we're not using an average tracker, the 'float' is nailed to
       the avgfloat_initial value.  It needs to be set for the min/max
       to deal properly */
    long bin=bi->avgfloat_initial*BITTRACK_DIVISOR; /* truncate on purpose */
    bm->queue_actual[head]|=bin;
    new_minmax_head=next_head;
  }	
  
  /* update the min/max queues and enforce limits */
  if(bm->minmax_binstack){
    long sampledesired=eofflag?0:bm->minmax_sampledesired;

    /* add to stack recent */
    while(minmax_head!=new_minmax_head){
      int samples=ci->blocksizes[bm->queue_actual[minmax_head]&
				0x80000000UL?1:0]>>1;

	/* the construction here is not parallel to the floater's
	   stack.  

	   floater[bin-1]  <-> floater supported at bin
	   ...
	   floater[0]      <-> floater supported at 1
	   supported at zero is implicit.  
	   the BINBITS macro performs offsetting

     
      bin  minmax[bin*2-1] <-> floater supported at bin
	   ...
	1  minmax[bin]     <-> floater supported at 1
	0  minmax[bin-1]   <-> no limit/support (limited to/supported at bin 0,
	                                         ie, no effect)
       -1  minmax[bin-2]   <-> floater limited to bin-1
	   ...
    -bin+1  minmax[0]       <-> floater limited to 1
	    limited to zero (val= -bin) is implicit
	*/
      for(i=0;i<bins;i++){
	bm->minmax_binstack[bm->minmax_stackptr*bins*2+bins+i]+=
	  LACING_ADJUST(
	  BINBITS(minmax_head,
		  (bm->queue_actual[minmax_head]&0x7fffffffUL)>i+1?
		  (bm->queue_actual[minmax_head]&0x7fffffffUL):i+1));
	
	bm->minmax_binstack[bm->minmax_stackptr*bins*2+i]+=
	  LACING_ADJUST(
	  BINBITS(minmax_head,
		  (bm->queue_actual[minmax_head]&0x7fffffffUL)<i+1?
		  (bm->queue_actual[minmax_head]&0x7fffffffUL):i+1));
      }
      
      bm->minmax_posstack[bm->minmax_stackptr]=minmax_head; /* not one
							       past
							       like
							       typical */
      bm->minmax_limitstack[bm->minmax_stackptr]=0;
      bm->minmax_sampleacc+=samples;
      bm->minmax_acctotal+=
	LACING_ADJUST(
	BINBITS(minmax_head,(bm->queue_actual[minmax_head]&0x7fffffffUL)));
      
      minmax_head++;
      if(minmax_head>=bm->queue_size)minmax_head=0;
    }

    /* check limits, enforce changes */
    if(bm->minmax_sampleacc>sampledesired){
      double bitrate=(double)bm->minmax_acctotal/bm->minmax_sampleacc*vi->rate;
      int limit=0;
      
      fprintf(stderr,"prelimit:%dkbps ",(int)bitrate/1000);
      if(bitrate>bi->queue_hardmax || bitrate<bi->queue_hardmin){
	int newstack;
	int stackctr;
	long bitsum=limit_sum(bm,0);
	bitrate=(double)bitsum/bm->minmax_sampleacc*vi->rate;
	
	/* we're off rate.  Iteratively try out new hard floater
           limits until we find one that brings us inside.  Here's
           where we see the whole point of the limit stacks.  */
	if(bitrate>bi->queue_hardmax){
	  for(limit=-1;limit>-bins;limit--){
	    long bitsum=limit_sum(bm,limit);
	    bitrate=(double)bitsum/bm->minmax_sampleacc*vi->rate;
	    if(bitrate<=bi->queue_hardmax)break;
	  }
	}else if(bitrate<bi->queue_hardmin){
	  for(limit=1;limit<bins;limit++){
	    long bitsum=limit_sum(bm,limit);
	    bitrate=(double)bitsum/bm->minmax_sampleacc*vi->rate;
	    if(bitrate>=bi->queue_hardmin)break;
	  }
	  if(bitrate>bi->queue_hardmax)limit--;
	}

	bitsum=limit_sum(bm,limit);
	bitrate=(double)bitsum/bm->minmax_sampleacc*vi->rate;
	fprintf(stderr,"postlimit:%dkbps ",(int)bitrate/1000);

	/* trace the limit backward, stop when we see a lower limit */
	newstack=bm->minmax_stackptr-1;
	while(newstack>=0){
	  if(bm->minmax_limitstack[newstack]<limit)break;
	  newstack--;
	}
	
	/* update bit counter with new limit and replace any stack
           limits that have been replaced by our new lower limit */
	stackctr=bm->minmax_stackptr;
	while(stackctr>newstack){
	  bm->minmax_acctotal-=
	    LIMITBITS(stackctr,bm->minmax_limitstack[stackctr]);
	  bm->minmax_acctotal+=LIMITBITS(stackctr,limit);

	  if(stackctr<bm->minmax_stackptr)
	    for(i=0;i<bins*2;i++)
	      bm->minmax_binstack[stackctr*bins*2+i]+=
	      bm->minmax_binstack[(stackctr+1)*bins*2+i];

	  stackctr--;
	}
	stackctr++;
	bm->minmax_posstack[stackctr]=bm->minmax_posstack[bm->minmax_stackptr];
	bm->minmax_limitstack[stackctr]=limit;
	fprintf(stderr,"limit:%d\n",limit);

	/* set up new blank stack entry */
	stackctr++;
	bm->minmax_stackptr=stackctr;
	memset(&bm->minmax_binstack[stackctr*bins*2],
	       0,
	       sizeof(*bm->minmax_binstack)*bins*2);
	bm->minmax_limitstack[stackctr]=0;
	bm->minmax_posstack[stackctr]=-1;
	
      }
    }
    
    /* remove from tail */
    while(bm->minmax_sampleacc>sampledesired){
      int samples=
	ci->blocksizes[bm->queue_actual[bm->minmax_tail]&0x80000000UL?1:0]>>1;
      int actual=bm->queue_actual[bm->minmax_tail]&0x7fffffffUL;

      for(i=0;i<bins;i++){
	bm->minmax_binstack[bins+i]-= /* always comes off the stack bottom */
	  LACING_ADJUST(BINBITS(bm->minmax_tail,actual>i+1?actual:i+1));
	bm->minmax_binstack[i]-= 
	  LACING_ADJUST(BINBITS(bm->minmax_tail,actual<i+1?actual:i+1));
      }

      /* always perform in this order; max overrules min */
      if(bm->minmax_limitstack[0]>actual)
	actual=bm->minmax_limitstack[0];
      if(bins+bm->minmax_limitstack[0]<actual)
	actual=bins+bm->minmax_limitstack[0];

      bm->minmax_acctotal-=LACING_ADJUST(BINBITS(bm->minmax_tail,actual));
      bm->minmax_sampleacc-=samples;
     
      /* revise queue_actual to reflect the limit */
      bm->queue_actual[bm->minmax_tail]=actual;
      
      if(bm->minmax_tail==bm->minmax_posstack[0]){
	/* the stack becomes a FIFO; the first data has fallen off */
	memmove(bm->minmax_binstack,bm->minmax_binstack+bins*2,
		sizeof(*bm->minmax_binstack)*bins*2*bm->minmax_stackptr);
	memmove(bm->minmax_posstack,bm->minmax_posstack+1,
		sizeof(*bm->minmax_posstack)*bm->minmax_stackptr);
	memmove(bm->minmax_limitstack,bm->minmax_limitstack+1,
		sizeof(*bm->minmax_limitstack)*bm->minmax_stackptr);
	bm->minmax_stackptr--;
      }

      bm->minmax_tail++;
      if(bm->minmax_tail>=bm->queue_size)bm->minmax_tail=0;
    }
    
    
    bm->last_to_flush=bm->minmax_tail;
  }else{
    bm->last_to_flush=bm->avg_center;
  }
  if(eofflag)
    bm->last_to_flush=bm->queue_head;
  return(0);
}

int vorbis_bitrate_flushpacket(vorbis_dsp_state *vd,ogg_packet *op){
  backend_lookup_state  *b=vd->backend_state;
  bitrate_manager_state *bm=&b->bms;

  if(bm->queue_size==0){
    if(bm->queue_head==0)return(0);

    memcpy(op,bm->queue_packets,sizeof(*op));
    bm->queue_head=0;

  }else{
    long bins=bm->queue_bins;
    long bin;
    long bytes;

    if(bm->next_to_flush==bm->last_to_flush)return(0);

    bin=bm->queue_actual[bm->next_to_flush]&0x7fffffffUL;
    bytes=(BINBITS(bm->next_to_flush,bin)+7)/8;
    
    memcpy(op,bm->queue_packets+bm->next_to_flush,sizeof(*op));
    if(bytes<op->bytes)op->bytes=bytes;

    bm->next_to_flush++;
    if(bm->next_to_flush>=bm->queue_size)bm->next_to_flush=0;

  }

  return(1);
}
