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

 function: psychoacoustics not including preecho
 last mod: $Id: psy.c,v 1.17 2000/03/10 13:21:18 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "vorbis/codec.h"

#include "psy.h"
#include "lpc.h"
#include "smallft.h"
#include "scales.h"
#include "misc.h"

/* Set up decibel threshhold slopes on a Bark frequency scale */

static void set_curve(double *ref,double *c,int n, double crate){
  int i,j=0;

  for(i=0;i<MAX_BARK-1;i++){
    int endpos=rint(fromBARK(i+1)*2*n/crate);
    double base=ref[i];
    double delta=(ref[i+1]-base)/(endpos-j);
    for(;j<endpos && j<n;j++){
      c[j]=base;
      base+=delta;
    }
  }
}

void _vp_psy_init(vorbis_look_psy *p,vorbis_info_psy *vi,int n,long rate){
  long i;
  memset(p,0,sizeof(vorbis_look_psy));
  p->maskthresh=malloc(n*sizeof(double));
  p->barknum=malloc(n*sizeof(double));
  p->vi=vi;
  p->n=n;

  /* set up the lookups for a given blocksize and sample rate */
  /* Vorbis max sample rate is limited by 26 Bark (54kHz) */
  set_curve(vi->maskthresh, p->maskthresh, n,rate);

  for(i=0;i<n;i++)
    p->barknum[i]=toBARK(rate/2.*i/n);

#ifdef ANALYSIS
  {
    int j;
    FILE *out;
    char buffer[80];
    
    sprintf(buffer,"mask_threshhold_%d.m",n);
    out=fopen(buffer,"w+");
    for(j=0;j<n;j++)
      fprintf(out,"%g\n",p->maskthresh[j]);
    fclose(out);
  }
#endif

}

void _vp_psy_clear(vorbis_look_psy *p){
  if(p){
    if(p->maskthresh)free(p->maskthresh);
    if(p->barknum)free(p->barknum);
    memset(p,0,sizeof(vorbis_look_psy));
  }
}

/* Masking curve: linear rolloff on a Bark/dB scale, attenuated by
   maskthresh */

void _vp_mask_floor(vorbis_look_psy *p,double *f, double *floor,int attp){
  int n=p->n;
  double hroll=p->vi->hrolldB;
  double lroll=p->vi->lrolldB;
  double curmask=todB(f[0])+(attp?p->maskthresh[0]:0);
  double curoc=0.;
  long i;

  /* run mask forward then backward */
  for(i=0;i<n;i++){
    double newmask=todB(f[i])+(attp?p->maskthresh[i]:0);
    double newoc=p->barknum[i];
    double roll=curmask-(newoc-curoc)*hroll;
    double troll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    troll=fromdB(roll);
    if(floor[i]<troll)floor[i]=troll;
  }

  curmask=todB(f[n-1])+(attp?p->maskthresh[n-1]:0);
  curoc=p->barknum[n-1];
  for(i=n-1;i>=0;i--){
    double newmask=todB(f[i])+(attp?p->maskthresh[i]:0);
    double newoc=p->barknum[i];
    double roll=curmask-(curoc-newoc)*lroll;
    double troll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    troll=fromdB(roll);
    if(floor[i]<troll)floor[i]=troll;
  }
}

/* s must be padded at the end with m-1 zeroes */
static void time_convolve(double *s,double *r,int n,int m){
  int i;
  
  for(i=0;i<n;i++){
    int j;
    double acc=0;

    for(j=0;j<m;j++)
      acc+=s[i+j]*r[m-j-1];

    s[i]=acc;
  }
}

void _vi_psy_free(vorbis_info_psy *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_psy));
    free(i);
  }
}
