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
 last modification date: Oct 17 1999

 Vorbis manipulates the dynamic range of the incoming PCM data
 envelope to minimise time-domain energy leakage from percussive and
 plosive waveforms being quantized in the MDCT domain.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "codec.h"
#include "mdct.h"
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

/* use MDCT for spectral power estimation */

static void _ve_deltas(double *deltas,double *pcm,int n,double *win,
		       int winsize){
  int i,j;
  mdct_lookup m;
  double out[winsize/2];

  mdct_init(&m,winsize);

  for(j=0;j<n;j++){
    double acc=0.;

    mdct_forward(&m,pcm+j*winsize/2,out,win);
    for(i=0;i<winsize/2;i++)
      acc+=fabs(out[i]);
    if(deltas[j]<acc)deltas[j]=acc;
  }

  mdct_clear(&m);

#ifdef ANALYSIS
  {
    static int frameno=0;
    FILE *out;
    char buffer[80];
    
    sprintf(buffer,"delta%d.m",frameno++);
    out=fopen(buffer,"w+");
    for(j=0;j<n;j++)
      fprintf(out,"%g\n",deltas[j]);
    fclose(out);
  }
#endif
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

#ifdef ANALYSIS
    {
      static int frameno=0.;
      FILE *out;
      int j;
      char buffer[80];
      
      sprintf(buffer,"del%d.m",frameno++);
      out=fopen(buffer,"w+");
      for(j=0;j<n;j++)
	fprintf(out,"%g\n",mult[j]);
      fclose(out);
    }
#endif

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
      
      for(;i<n;i++){
	if(mult[i]>=last*vi->preecho_thresh){
	  last=mult[i];
	  
	  mult[i]=floor(log(mult[i]/clamp)/log(2));
	  if(mult[i]>15)mult[i]=15;
	  if(mult[i]<1)mult[i]=1;

	}else{
	  mult[i]=0;
	}
      }  
    }else
      memset(mult,0,sizeof(double)*n);

#ifdef ANALYSIS
    {
      static int frameno=0.;
      FILE *out;
      int j;
      char buffer[80];
      
      sprintf(buffer,"sparse%d.m",frameno++);
      out=fopen(buffer,"w+");
      for(j=0;j<n;j++)
	fprintf(out,"%g\n",mult[j]);
      fclose(out);
    }
#endif


  }
}

void _ve_envelope_apply(vorbis_block *vb,int multp){
  static int frameno=0;
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
      }else{
	mult[j]=last;
      }
    }

    /* compute the envelope curve */
    if(_ve_envelope_generate(mult,env,look->window,vb->multend,
			     vi->envelopesa)){
#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"env%d.m",frameno);
	out=fopen(buffer,"w+");
	for(j=0;j<vb->multend*vi->envelopesa;j++)
	  fprintf(out,"%g\n",env[j]);
	fclose(out);
	sprintf(buffer,"prepcm%d.m",frameno);
	out=fopen(buffer,"w+");
	for(j=0;j<vb->multend*vi->envelopesa;j++)
	  fprintf(out,"%g\n",vb->pcm[i][j]);
	fclose(out);
      }
#endif

      for(k=0;k<vi->channels;k++){

	if(multp)
	  for(j=0;j<vb->multend*vi->envelopesa;j++)
	    vb->pcm[k][j]*=env[j];
	else
	  for(j=0;j<vb->multend*vi->envelopesa;j++)
	    vb->pcm[k][j]/=env[j];

      }

#ifdef ANALYSIS
      {
	FILE *out;
	char buffer[80];
	
	sprintf(buffer,"pcm%d.m",frameno);
	out=fopen(buffer,"w+");
	for(j=0;j<vb->multend*vi->envelopesa;j++)
	  fprintf(out,"%g\n",vb->pcm[i][j]);
	fclose(out);
      }
#endif
    }
    frameno++;
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
    for(j=0;j<n;j++){
      _oggpack_write(&vb->opb,(int)(mult[j]),4);
    }
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





