/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE Ogg Vorbis SOFTWARE CODEC SOURCE CODE.  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU PUBLIC LICENSE 2, WHICH IS INCLUDED WITH THIS SOURCE.    *
 * PLEASE READ THESE TERMS DISTRIBUTING.                            *
 *                                                                  *
 * THE OggSQUISH SOURCE CODE IS (C) COPYRIGHT 1994-1999             *
 * by 1999 Monty and The XIPHOPHORUS Company                        *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

 function: modified discrete cosine transform prototypes

 ********************************************************************/

#ifndef _OGG_MDCT_H_
#define _OGG_MDCT_H_

typedef struct {
  int n;
  int log2n;
  
  double *trig;
  int    *bitrev;

} MDCT_lookup;

extern MDCT_lookup *MDCT_init(int n);
extern void MDCT_free(MDCT_lookup *l);
extern void MDCT(double *in, double *out, MDCT_lookup *init, double *window);
extern void iMDCT(double *in, double *out, MDCT_lookup *init, double *window);

#endif












