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

 function: coder backend; handles encoding into and decoding from 
           codewords, codebook generation, and codebook transmission
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 13 1999

 ********************************************************************/

#include <stdlib.h>

#include "codec.h"

/* several basic functions:
   
   create codebooks from representation
   create codebooks from data
   encode codebooks
   decode codebooks
   
   encode using codebook
   decode using codebook
   map an entry to a sparse codebook's (eg, VQ) codeword index
*/

typedef struct vorbis_codebook{

  int entries;
  
  /*** codebook side ****************************************************/

  /* encode side tree structure (indice->codeword) */
  int *codewords;   /* null if indice==codeword (ie fixed len codeword) */
  int *codelengths; /* null if indice==codeword */
  int len;          /* -1 if varlength */

  /* decode side tree structure (codeword->indice) */ 
  
  int *tree; /* null if codeword==indice */

  /*** mapping side *****************************************************/

  int wordsper;
  
  /* decode is easy */
  int remaining;
  int *mapping;

  /* encode in the VQ case is what's hard.  We need to find the
     'closest match' out of a large n-dimentional space */


} vorbis_codebook;
