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

#ifndef _OGG_mdct_H_
#define _OGG_mdct_H_

typedef struct {
  int n;
  int log2n;
  
  double *trig;
  int    *bitrev;

} mdct_lookup;

extern void mdct_init(mdct_lookup *lookup,int n);
extern void mdct_clear(mdct_lookup *l);
extern void mdct_forward(mdct_lookup *init, double *in, 
			 double *out, double *window);
extern void mdct_backward(mdct_lookup *init, double *in, 
			  double *out, double *window);

#endif












