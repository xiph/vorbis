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

 function: spectrum envelope and residue code/decode
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 10 1999

 ********************************************************************/

#include <stdio.h>
#include <math.h>
#include "codec.h"
#include "bitwise.h"
#include "spectrum.h"

/* this code is still seriously abbreviated.  I'm filling in pieces as
   we go... --Monty 19991004 */

int _vs_spectrum_encode(vorbis_block *vb,double amp,double *lsp){
  /* no real coding yet.  Just write out full sized words for now
     because people need bitstreams to work with */

  int scale=vb->W;
  int n=vb->pcmend/2;
  int last=0;
  int bits=rint(log(n)/log(2));
  int i;
  
  _oggpack_write(&vb->opb,amp*32768,15);
  
  for(i=0;i<vb->vd->vi->floororder[scale];i++){
    int val=rint(lsp[i]/M_PI*n-last);
    lsp[i]=(last+=val)*M_PI/n;
    _oggpack_write(&vb->opb,val,bits);
  }
  return(0);
}

int _vs_spectrum_decode(vorbis_block *vb,double *amp,double *lsp){
  int scale=vb->W;
  int n=vb->pcmend/2;
  int last=0;
  int bits=rint(log(n)/log(2));
  int i;

  *amp=_oggpack_read(&vb->opb,15)/32768.;

  for(i=0;i<vb->vd->vi->floororder[scale];i++){
    int val=_oggpack_read(&vb->opb,bits);
    lsp[i]=(last+=val)*M_PI/n;
  }
  return(0);
}


void _vs_residue_quantize(double *data,double *curve,
				 vorbis_info *vi,int n){

  /* The following is temporary, hardwired bullshit */
  int i;

  for(i=0;i<n;i++){

    int val=rint(data[i]/curve[i]);
    if(val>16)val=16;
    if(val<-16)val=-16;

    /*if(val==0){
      if(data[i]<0){
	val=-1;
      }else{
	val=1;
      }
      }*/
    
    data[i]=val;
    /*if(val<0){
    }else{
      data[i]=val+15;
      }*/
  }
}

int _vs_residue_encode(vorbis_block *vb,double *data){
  /* no real coding yet.  Just write out full sized words for now
     because people need bitstreams to work with */

  int              n=vb->pcmend/2;
  int i;

  for(i=0;i<n;i++){
    _oggpack_write(&vb->opb,(int)(data[i]+16),6);
  }

  return(0);
}

int _vs_residue_decode(vorbis_block *vb,double *data){
  /* no real coding yet.  Just write out full sized words for now
     because people need bitstreams to work with */

  int              n=vb->pcmend/2;
  int i;

  for(i=0;i<n;i++){
    data[i]=_oggpack_read(&vb->opb,6)-16;
    /*if(data[i]>=0)data[i]+=1;*/
  }
  return(0);
}

