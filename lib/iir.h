/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

  function: Direct Form I, II IIR filters, plus some specializations
  last mod: $Id: iir.h,v 1.2.2.1 2000/11/04 06:21:44 xiphmont Exp $

 ********************************************************************/

#ifndef _V_IIR_H_
#define _V_IIR_H_

typedef struct {
  int stages;
  float *coeff_A;
  float *coeff_B;
  float *z_A;
  float *z_B;
  int ring;
  float gain;
} IIR_state;

void IIR_init(IIR_state *s,int stages,float gain, float *A, float *B);
void IIR_clear(IIR_state *s);
float IIR_filter(IIR_state *s,float in);
float IIR_filter_ChebBand(IIR_state *s,float in);

#endif
