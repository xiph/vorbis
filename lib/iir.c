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

  function: Direct Form I, II IIR filters, plus some specializations
  last mod: $Id: iir.c,v 1.7 2001/02/02 03:51:56 xiphmont Exp $

 ********************************************************************/

/* LPC is actually a degenerate case of form I/II filters, but we need
   both */

#include <ogg/ogg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "iir.h"

void IIR_init(IIR_state *s,int stages,float gain, float *A, float *B){
  memset(s,0,sizeof(IIR_state));
  s->stages=stages;
  s->gain=gain;
  s->coeff_A=_ogg_malloc(stages*sizeof(float));
  s->coeff_B=_ogg_malloc((stages+1)*sizeof(float));
  s->z_A=_ogg_calloc(stages*2,sizeof(float));

  memcpy(s->coeff_A,A,stages*sizeof(float));
  memcpy(s->coeff_B,B,(stages+1)*sizeof(float));
}

void IIR_clear(IIR_state *s){
  if(s){
    _ogg_free(s->coeff_A);
    _ogg_free(s->coeff_B);
    _ogg_free(s->z_A);
    memset(s,0,sizeof(IIR_state));
  }
}

void IIR_reset(IIR_state *s){
  memset(s->z_A,0,sizeof(float)*s->stages*2);
}

float IIR_filter(IIR_state *s,float in){
  int stages=s->stages,i;
  float newA;
  float newB=0;
  float *zA=s->z_A+s->ring;

  newA=in/=s->gain;
  for(i=0;i<stages;i++){
    newA+= s->coeff_A[i] * zA[i];
    newB+= s->coeff_B[i] * zA[i];
  }
  newB+=newA*s->coeff_B[stages];

  zA[0]=zA[stages]=newA;
  if(++s->ring>=stages)s->ring=0;

  return(newB);
}

/* this assumes the symmetrical structure of the feed-forward stage of
   a Chebyshev bandpass to save multiplies */
float IIR_filter_ChebBand(IIR_state *s,float in){
  int stages=s->stages,i;
  float newA;
  float newB=0;
  float *zA=s->z_A+s->ring;

  newA=in/=s->gain;

  newA+= s->coeff_A[0] * zA[0];
  for(i=1;i<(stages>>1);i++){
    newA+= s->coeff_A[i] * zA[i];
    newB+= s->coeff_B[i] * (zA[i]-zA[stages-i]);
  }
  newB+= s->coeff_B[i] * zA[i];
  for(;i<stages;i++)
    newA+= s->coeff_A[i] * zA[i];

  newB+= newA-zA[0];

  zA[0]=zA[stages]=newA;
  if(++s->ring>=stages)s->ring=0;

  return(newB);
}

#ifdef _V_SELFTEST

/* z^-stage, z^-stage+1... */
static float cheb_bandpass_B[]={-1.f,0.f,5.f,0.f,-10.f,0.f,10.f,0.f,-5.f,0.f,1f};
static float cheb_bandpass_A[]={-0.6665900311f,
				  1.0070146601f,
				 -3.1262875409f,
			 	  3.5017171569f,
				 -6.2779211945f,
				  5.2966481740f,
				 -6.7570216587f,
				  4.0760335768f,
				 -3.9134284363f,
				  1.3997338886f};

static float data[128]={  
  0.0426331f,
  0.0384521f,
  0.0345764f,
  0.0346069f,
  0.0314636f,
  0.0310059f,
  0.0318604f,
  0.0336304f,
  0.036438f,
  0.0348511f,
  0.0354919f,
  0.0343628f,
  0.0325623f,
  0.0318909f,
  0.0263367f,
  0.0225525f,
  0.0195618f,
  0.0160828f,
  0.0168762f,
  0.0145569f,
  0.0126343f,
  0.0127258f,
  0.00820923f,
  0.00787354f,
  0.00558472f,
  0.00204468f,
  3.05176e-05f,
  -0.00357056f,
  -0.00570679f,
  -0.00991821f,
  -0.0101013f,
  -0.00881958f,
  -0.0108948f,
  -0.0110168f,
  -0.0119324f,
  -0.0161438f,
  -0.0194702f,
  -0.0229187f,
  -0.0260315f,
  -0.0282288f,
  -0.0306091f,
  -0.0330505f,
  -0.0364685f,
  -0.0385742f,
  -0.0428772f,
  -0.043457f,
  -0.0425415f,
  -0.0462341f,
  -0.0467529f,
  -0.0489807f,
  -0.0520325f,
  -0.0558167f,
  -0.0596924f,
  -0.0591431f,
  -0.0612793f,
  -0.0618591f,
  -0.0615845f,
  -0.0634155f,
  -0.0639648f,
  -0.0683594f,
  -0.0718079f,
  -0.0729675f,
  -0.0791931f,
  -0.0860901f,
  -0.0885315f,
  -0.088623f,
  -0.089386f,
  -0.0899353f,
  -0.0886841f,
  -0.0910645f,
  -0.0948181f,
  -0.0919495f,
  -0.0891418f,
  -0.0916443f,
  -0.096344f,
  -0.100464f,
  -0.105499f,
  -0.108612f,
  -0.112213f,
  -0.117676f,
  -0.120911f,
  -0.124329f,
  -0.122162f,
  -0.120605f,
  -0.12326f,
  -0.12619f,
  -0.128998f,
  -0.13205f,
  -0.134247f,
  -0.137939f,
  -0.143555f,
  -0.14389f,
  -0.14859f,
  -0.153717f,
  -0.159851f,
  -0.164551f,
  -0.162811f,
  -0.164276f,
  -0.156952f,
  -0.140564f,
  -0.123291f,
  -0.10321f,
  -0.0827637f,
  -0.0652466f,
  -0.053772f,
  -0.0509949f,
  -0.0577698f,
  -0.0818176f,
  -0.114929f,
  -0.148895f,
  -0.181122f,
  -0.200714f,
  -0.21048f,
  -0.203644f,
  -0.179413f,
  -0.145325f,
  -0.104492f,
  -0.0658264f,
  -0.0332031f,
  -0.0106201f,
  -0.00363159f,
  -0.00909424f,
  -0.0244141f,
  -0.0422058f,
  -0.0537415f,
  -0.0610046f,
  -0.0609741f,
  -0.0547791f};

/* comparison test code from http://www-users.cs.york.ac.uk/~fisher/mkfilter/
   (the above page kicks ass, BTW)*/

#define NZEROS 10
#define NPOLES 10
#define GAIN   4.599477515e+02f

static float xv[NZEROS+1], yv[NPOLES+1];

static float filterloop(float next){ 
  xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; 
  xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; xv[8] = xv[9]; xv[9] = xv[10]; 
  xv[10] = next / GAIN;
  yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; 
  yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; yv[8] = yv[9]; yv[9] = yv[10]; 
  yv[10] =   (xv[10] - xv[0]) + 5 * (xv[2] - xv[8]) + 10 * (xv[6] - xv[4])
    + ( -0.6665900311f * yv[0]) + (  1.0070146601f * yv[1])
    + ( -3.1262875409f * yv[2]) + (  3.5017171569f * yv[3])
    + ( -6.2779211945f * yv[4]) + (  5.2966481740f * yv[5])
    + ( -6.7570216587f * yv[6]) + (  4.0760335768f * yv[7])
    + ( -3.9134284363f * yv[8]) + (  1.3997338886f * yv[9]);
  return(yv[10]);
}

#include <stdio.h>
int main(){

  /* run the pregenerated Chebyshev filter, then our own distillation
     through the generic and specialized code */
  float *work=_ogg_malloc(128*sizeof(float));
  IIR_state iir;
  int i;

  for(i=0;i<128;i++)work[i]=filterloop(data[i]);
  {
    FILE *out=fopen("IIR_ref.m","w");
    for(i=0;i<128;i++)fprintf(out,"%g\n",work[i]);
    fclose(out);
  }

  IIR_init(&iir,NPOLES,GAIN,cheb_bandpass_A,cheb_bandpass_B);
  for(i=0;i<128;i++)work[i]=IIR_filter(&iir,data[i]);
  {
    FILE *out=fopen("IIR_gen.m","w");
    for(i=0;i<128;i++)fprintf(out,"%g\n",work[i]);
    fclose(out);
  }
  IIR_clear(&iir);  

  IIR_init(&iir,NPOLES,GAIN,cheb_bandpass_A,cheb_bandpass_B);
  for(i=0;i<128;i++)work[i]=IIR_filter_ChebBand(&iir,data[i]);
  {
    FILE *out=fopen("IIR_cheb.m","w");
    for(i=0;i<128;i++)fprintf(out,"%g\n",work[i]);
    fclose(out);
  }
  IIR_clear(&iir);  

  return(0);
}

#endif
