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

 function: libvorbis codec internal types.  These structures are 
           'visible', but generally uninteresting to the developer
 last mod: $Id: internal.h,v 1.2 2000/01/20 04:43:51 xiphmont Exp $

 ********************************************************************/

#ifndef _vorbis_internal_h_
#define _vorbis_internal_h_

/* lookup structures for various simple transforms *****************/

typedef struct {
  int n;
  int log2n;
  
  double *trig;
  int    *bitrev;

} mdct_lookup;

typedef struct {
  int n;
  double *trigcache;
  int *splitcache;
} drft_lookup;

typedef struct {
  int winlen;
  double *window;
  mdct_lookup mdct;
} envelope_lookup;

typedef struct lpclook{
  /* en/decode lookups */
  int *linearmap;
  double *barknorm;
  drft_lookup fft;

  int n;
  int ln;
  int m;

} lpc_lookup;

/* structures for various internal data abstractions ********************/

typedef struct {
  long endbyte;     
  int  endbit;      

  unsigned char *buffer;
  unsigned char *ptr;
  long storage;
  
} oggpack_buffer;

#endif





