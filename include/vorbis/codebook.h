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
 last mod: $Id: codebook.h,v 1.4.4.3 2000/04/06 15:59:36 xiphmont Exp $

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
  int    q_log;       /* 0 == linear, 1 == log (dB) mapping */

  /* The below does a linear, single monotonic sequence mapping.  
     The log mapping uses this, but extends it */
  long   q_min;       /* packed 24 bit float; quant value 0 maps to minval */
  long   q_delta;     /* packed 24 bit float; val 1 - val 0 == delta */
  int    q_quant;     /* bits: 0 < quant <= 16 */
  int    q_sequencep; /* bitflag */

  /* additional information for log (dB) mapping; the linear mapping
     is assumed to actually be values in dB.  encodebias is used to
     assign an error weight to 0 dB. We have two additional flags:
     zeroflag indicates if entry zero is to represent -Inf dB; negflag
     indicates if we're to represent negative linear values in a
     mirror of the positive mapping. */
  int    q_zeroflag;  
  int    q_negflag;

  /* encode only values that provide log encoding error parameters */
  double q_encodebias; /* encode only */
  double q_entropy;    /* encode only */


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
  double *logdist;    /* list of dim*entries metric vals for log encode */
  long   *codelist;   /* list of bitstream codewords for each entry */
  struct decode_aux *decode_tree;

} codebook;

#endif





