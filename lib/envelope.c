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

 function: PCM data envelope analysis and manipulation
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 06 1999

 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "codec.h"
#include "envelope.h"
#include "bitwise.h"

void _ve_envelope_init(envelope_lookup *e,int samples_per){
  int i;

  e->winlen=samples_per*2;
  e->window=malloc(e->winlen*sizeof(double));

  /* We just use a straight sin^2(x) window for this */
  for(i=0;i<e->winlen;i++){
    double temp=sin((i+.5)/e->winlen*M_PI);
    e->window[i]=temp*temp;
  }
}

void _ve_envelope_clear(envelope_lookup *e){
  if(e->window)free(e->window);
  memset(e,0,sizeof(envelope_lookup));
}

/* initial and final blocks are special cases. Eg:
   ______
         `--_            
   |_______|_`-.___|_______|_______|

              ___                     
          _--'   `--_     
   |___.-'_|_______|_`-.___|_______|

                      ___                     
                  _--'   `--_     
   |_______|___.-'_|_______|_`-.___|

                               _____
                           _--'
   |_______|_______|____.-'|_______|
 
   as we go block by block, we watch the collective metrics span. If we 
   span the threshhold (assuming the threshhold is active), we use an 
   abbreviated vector */

static int _ve_envelope_generate(double *mult,double *env,double *look,
				 int n,int step){
  int i,j,p;
  double m,mo;

  for(j=0;j<n;j++)if(mult[j]!=1)break;
  if(j==n)return(0);

  n*=step;
  /* first multiplier special case */
  m=ldexp(1,mult[0]-1);
  for(p=0;p<step/2;p++)env[p]=m;
  
  /* mid multipliers normal case */
  for(j=1;p<n-step/2;j++){
    mo=m;
    m=ldexp(1,mult[j]-1);
    if(mo==m)
      for(i=0;i<step;i++,p++)env[p]=m;
    else
      for(i=0;i<step;i++,p++)env[p]=m*look[i]+mo*look[i+step];
  }

  /* last multiplier special case */
  for(;p<n;p++)env[p]=m;
  return(1);
}

/* right now, we do things simple and dirty (read: our current preecho
   is a joke).  Should this prove inadequate, then we'll think of
   something different.  The details of the encoding format do not
   depend on the exact behavior, only the format of the bits that come
   out.

   Mark Taylor probably has much witter ways of doing this...  Let's
   see if simple delta analysis gives us acceptible results for now.  */

static void _ve_deltas(double *deltas,double *pcm,int n,double *win,
		       int winsize){
  int i,j,p=winsize/2;
  for(j=0;j<n;j++){
    p-=winsize/2;
    for(i=0;i<winsize-1;i++,p++){
      double temp=fabs(win[i]*pcm[p]-win[i+1]*pcm[p+1]);
      if(deltas[j]<temp)deltas[j]=temp;
    }
    p++;
  }
}

void _ve_envelope_multipliers(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  int step=vi->envelopesa;
  static int frame=0;

  /* we need a 1-1/4 envelope window overlap begin and 1/4 end */
  int dtotal=(v->pcm_current-step/2)/vi->envelopesa;
  int dcurr=v->envelope_current;
  double *window=v->ve.window;
  int winlen=v->ve.winlen;
  int pch,ech;
  
  if(dtotal>dcurr){
    for(ech=0;ech<vi->envelopech;ech++){
      double *mult=v->multipliers[ech]+dcurr;
      memset(mult,0,sizeof(double)*(dtotal-dcurr));
      
      for(pch=0;pch<vi->channels;pch++){
	
	/* does this channel contribute to the envelope analysis */
	/*if(vi->envelopemap[pch]==ech){ not mapping yet */
	if(pch==ech){

	  /* we need a 1/4 envelope window overlap front and back */
	  double *pcm=v->pcm[pch]+dcurr*step-step/2;
	  _ve_deltas(mult,pcm,dtotal-dcurr,window,winlen);

	}
      }
    }
    v->envelope_current=dtotal;
    frame++;
  }
}

/* This readies the multiplier vector for use/coding.  Clamp/adjust
   the multipliers to the allowed range and eliminate unneeded
   coefficients */

void _ve_envelope_sparsify(vorbis_block *vb){
  vorbis_info *vi=vb->vd->vi;
  int ch;
  for(ch=0;ch<vi->envelopech;ch++){
    int flag=0;
    double *mult=vb->mult[ch];
    int n=vb->multend;
    double first=mult[0];
    double last=first;
    double clamp;
    int i;

    /* are we going to multiply anything? */
    
    for(i=1;i<n;i++){
      if(mult[i]>=last*vi->preecho_thresh){
	flag=1;
	break;
      }
      if(i<n-1 && mult[i+1]>=last*vi->preecho_thresh){
	flag=1;
	break;
      }
      last=mult[i];
    }
    
    if(flag){
      /* we need to adjust, so we might as well go nuts */
      
      int begin=i;
      clamp=last?last:1;
      
      for(i=0;i<begin;i++)mult[i]=0;
      
      last=1;
      for(;i<n;i++){
	if(mult[i]/last>clamp*vi->preecho_thresh){
	  last=mult[i]/vi->preecho_clamp;
	  
	  mult[i]=floor(log(mult[i]/clamp/vi->preecho_clamp)/log(2))-1;
	  if(mult[i]>15)mult[i]=15;
	}else{
	  mult[i]=0;
	}
      }  
    }else
      memset(mult,0,sizeof(double)*n);
  }
}

void _ve_envelope_apply(vorbis_block *vb,int multp){
  vorbis_info *vi=vb->vd->vi;
  double env[vb->multend*vi->envelopesa];
  envelope_lookup *look=&vb->vd->ve;
  int i,j,k;
  
  for(i=0;i<vi->envelopech;i++){
    double *mult=vb->mult[i];
    double last=1.;
    
    /* fill in the multiplier placeholders */

    for(j=0;j<vb->multend;j++){
      if(mult[j]){
	last=mult[j];
      }else
	mult[j]=last;
    }

    /* compute the envelope curve */
    if(_ve_envelope_generate(mult,env,look->window,vb->multend,
			     vi->envelopesa)){
      
      /* apply the envelope curve */
      for(j=0;j<vi->channels;j++){
	
	/* check to see if the generated envelope applies to this channel */
	/*if(vi->envelopemap[j]==i){ not mapping yet */
	if(j==i){
	  
	  if(multp)
	    for(k=0;k<vb->multend*vi->envelopesa;k++)
	      vb->pcm[j][k]*=env[k];
	  else
	    for(k=0;k<vb->multend*vi->envelopesa;k++)
	      vb->pcm[j][k]/=env[k];
	}
      }
    }
  }
}

int _ve_envelope_encode(vorbis_block *vb){
  /* Not huffcoded yet. */

  vorbis_info *vi=vb->vd->vi;
  int scale=vb->W;
  int n=vb->multend;
  int i,j;

  /* range is 0-15 */

  for(i=0;i<vi->envelopech;i++){
    double *mult=vb->mult[i];
    for(j=0;j<n;j++)
      _oggpack_write(&vb->opb,(int)(mult[j]),4);
  }
  return(0);
}

/* synthesis expands the buffers in vb if needed.  We can assume we
   have enough storage handed to us. */
int _ve_envelope_decode(vorbis_block *vb){
  vorbis_info *vi=vb->vd->vi;
  int scale=vb->W;
  int n=vb->multend;
  int i,j;

  /* range is 0-15 */

  for(i=0;i<vi->envelopech;i++){
    double *mult=vb->mult[i];
    for(j=0;j<n;j++)
      mult[j]=_oggpack_read(&vb->opb,4);
  }
  return(0);
}


