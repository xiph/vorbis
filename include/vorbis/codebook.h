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

 function: codebook types
 last mod: $Id: codebook.h,v 1.4 2000/01/28 09:05:00 xiphmont Exp $

 ********************************************************************/

#ifndef _V_CODEBOOK_H_
#define _V_CODEBOOK_H_

/* This structure encapsulates huffman and VQ style encoding books; it
   doesn't do anything specific to either.

   valuelist/quantlist are nonNULL (and q_* significant) only if
   there's entry->value mapping to be done.

   If encode-side mapping must be done (and thus the entry needs to be
   hunted), the auxiliary encode pointer will point to a decision
   tree.  This is true of both VQ and huffman, but is mostly useful
   with VQ.

*/

typedef struct static_codebook{
  long dim;           /* codebook dimensions (elements per vector) */
  long entries;       /* codebook entries */

  /* mapping */
  long   q_min;       /* packed 24 bit float; quant value 0 maps to minval */
  long   q_delta;     /* packed 24 bit float; val 1 - val 0 == delta */
  int    q_quant;     /* 0 < quant <= 16 */
  int    q_sequencep; /* bitflag */

  long   *quantlist;  /* list of dim*entries quantized entry values */

  long   *lengthlist; /* codeword lengths in bits */

  struct encode_aux *encode_tree;
} static_codebook;

typedef struct encode_aux{
  /* pre-calculated partitioning tree */
  long   *ptr0;
  long   *ptr1;

  long   *p;         /* decision points (each is an entry) */
  long   *q;         /* decision points (each is an entry) */
  long   aux;        /* number of tree entries */
  long   alloc;       
} encode_aux;

typedef struct decode_aux{
  long   *ptr0;
  long   *ptr1;
  long   aux;        /* number of tree entries */
} decode_aux;

typedef struct codebook{
  long dim;           /* codebook dimensions (elements per vector) */
  long entries;       /* codebook entries */
  const static_codebook *c;

  double *valuelist;  /* list of dim*entries actual entry values */
  long   *codelist;   /* list of bitstream codewords for each entry */
  struct decode_aux *decode_tree;

} codebook;

#define VQ_FEXP_BIAS 20 /* bias toward values smaller than 1. */

#endif





