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

 function: PCM data envelope analysis and manipulation
 last mod: $Id: envelope.c,v 1.16 2000/03/10 13:21:18 xiphmont Exp $

 Preecho calculation.

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "vorbis/codec.h"

#include "os.h"
#include "mdct.h"
#include "envelope.h"
#include "bitwise.h"
#include "window.h"
#include "misc.h"

void _ve_envelope_init(envelope_lookup *e,int samples_per){
  int i;

  e->winlen=samples_per;
  e->window=malloc(e->winlen*sizeof(double));
  mdct_init(&e->mdct,e->winlen);

  /* We just use a straight sin(x) window for this */
  for(i=0;i<e->winlen;i++)
    e->window[i]=sin((i+.5)/e->winlen*M_PI);

}

void _ve_envelope_clear(envelope_lookup *e){
  if(e->window)free(e->window);
  mdct_clear(&e->mdct);
  memset(e,0,sizeof(envelope_lookup));
}

/* use MDCT for spectral power estimation */

static void _ve_deltas(double *deltas,double *pcm,int n,double *window,
		       int winsize,mdct_lookup *m){
  int i,j;
  double *out=alloca(sizeof(double)*winsize);
  
  for(j=0;j<n;j++){
    double acc=0.;
 
    memcpy(out,pcm+j*winsize,winsize*sizeof(double));
    for(i=0;i<winsize;i++)
      out[i]*=window[i];

    mdct_forward(m,out,out);

    for(i=winsize/10;i<winsize/2;i++)
      acc+=fabs(out[i]);
    if(deltas[j]<acc)deltas[j]=acc;
  }
}

void _ve_envelope_deltas(vorbis_dsp_state *v){
  vorbis_info *vi=v->vi;
  int step=vi->envelopesa;
  
  int dtotal=v->pcm_current/vi->envelopesa;
  int dcurr=v->envelope_current;
  int pch;
  
  if(dtotal>dcurr){
    double *mult=v->multipliers+dcurr;
    memset(mult,0,sizeof(double)*(dtotal-dcurr));
      
    for(pch=0;pch<vi->channels;pch++){
      double *pcm=v->pcm[pch]+dcurr*step;
      _ve_deltas(mult,pcm,dtotal-dcurr,v->ve.window,v->ve.winlen,&v->ve.mdct);
    }
    v->envelope_current=dtotal;

#ifdef ANALYSIS
    {
      static int frameno=0;
      FILE *out;
      char buffer[80];
      int j,k;
      
      sprintf(buffer,"delt%d.m",frameno);
      out=fopen(buffer,"w+");
      for(j=0;j<v->envelope_current;j++)
	fprintf(out,"%d %g\n",j*vi->envelopesa+vi->envelopesa/2,v->multipliers[j]);
      fclose(out);

      sprintf(buffer,"deltpcm%d.m",frameno++);
      out=fopen(buffer,"w+");
      for(k=0;k<vi->channels;k++){
	for(j=0;j<v->pcm_current;j++)
	  fprintf(out,"%d %g\n",j,v->pcm[k][j]);
	fprintf(out,"\n");
      }
      fclose(out);
    }
#endif

  }
}






