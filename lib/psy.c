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

 function: random psychoacoustics (not including preecho)
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Aug 08 1999

 ********************************************************************/

#include <math.h>
#include "codec.h"
#include "psy.h"

#define NOISEdB 6

#define MASKdB  12
#define HROLL   60
#define LROLL   90
#define MASKBIAS  40

#define LNOISE  .8
#define HNOISE  1.01
#define NOISEBIAS  20

/* Find the mean log energy of a given 'band'; used to evaluate tones
   against background noise */

/* This is faster than a real convolution, gives us roughly the log f
   scale we seek, and gives OK results.  So, that means it's a good
   hack */

/* To add: f scale noise attenuation curve */

void _vp_noise_floor(double *f, double *m,int n){
  long lo=0,hi=0;
  double acc=0,div=0;
  int i,j;

  for(i=0;i<n;i++){
    long newlo=i*LNOISE-NOISEBIAS;
    long newhi=i*HNOISE+NOISEBIAS;
    double temp;
    
    if(newhi>n)newhi=n;
    if(newlo<0)newlo=0;

    for(j=hi;j<newhi;j++){
      acc+=todB(f[j]);
      div++;
    }
    for(j=lo;j<newlo;j++){
      acc-=todB(f[j]);
      div--;
    }

    hi=newhi;
    lo=newlo;

    temp=fromdB(acc/div+NOISEdB); /* The NOISEdB constant should be an
				     attenuation curve */
    if(m[i]<temp)m[i]=temp;
  }
}

/* figure the masking curve.  linear rolloff on a dB scale, adjusted
   by octave */
void _vp_mask_floor(double *f, double *m,int n){
  double ocSCALE=1./log(2);
  double curmask=-9.e40;
  double curoc=log(MASKBIAS)*ocSCALE;
  long i;

  /* run mask forward then backward */
  for(i=0;i<n;i++){
    double newmask=todB(f[i])-MASKdB;
    double newoc=log(i+MASKBIAS)*ocSCALE;
    double roll=curmask-(newoc-curoc)*HROLL;
    double lroll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    lroll=fromdB(roll);
    if(m[i]<lroll)m[i]=lroll;
  }

  curmask=-9.e40;
  curoc=log(n+MASKBIAS)*ocSCALE;
  for(i=n-1;i>=0;i--){
    double newmask=todB(f[i])-MASKdB;
    double newoc=log(i+MASKBIAS)*ocSCALE;
    double roll=curmask-(curoc-newoc)*LROLL;
    double lroll;
    if(newmask>roll){
      roll=curmask=newmask;
      curoc=newoc;
    }
    lroll=fromdB(roll);
    if(m[i]<lroll)m[i]=lroll;
  }
}

void _vp_psy_quantize(double *f, double *m,int n){
  int i;
  for(i=0;i<n;i++){
    int val=rint(f[i]/m[i]);
    if(val>16)val=16;
    if(val<-16)val=-16;
    f[i]=val*m[i];
  }
}

void _vp_psy_sparsify(double *f, double *m,int n){
  int i;
  for(i=0;i<n;i++)
    if(fabs(f[i])<m[i])f[i]=0;
}

void  _vp_psy_make_lsp(vorbis_block *vb){




}

