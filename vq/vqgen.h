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
 ********************************************************************/

#ifndef _VQGEN_H_
#define _VQGEN_H_

typedef struct vqgen{
  int it;
  int elements;
  int aux;

  /* point cache */
  double *pointlist; 
  long   points;
  long   allocated;

  /* entries */
  double *entrylist;
  long   *assigned;
  double *bias;
  long   entries;

  double  (*metric_func) (struct vqgen *v,double *entry,double *point);
  double *(*weight_func) (struct vqgen *v,double *point);
} vqgen;

typedef struct vqbook{
  long dim;           /* codebook dimensions (elements per vector) */
  long entries;       /* codebook entries */

  long   min;         /* packed 24 bit float; quant value 0 maps to minval */
  long   delta;       /* packed 24 bit float; val 1 - val 0 == delta */       
  int    quant;       /* 0 < quant <= 16 */
  int    sequencep;   /* bitflag */

  double *valuelist;  /* list of dim*entries actual entry values */
  long   *quantlist;  /* list of dim*entries quantized entry values */
  long   *codelist;   /* list of bitstream codewords for each entry */
  long   *lengthlist; /* codeword lengths in bits */

  /* auxiliary encoding/decoding information */
  /* encode: provided pre-calculated partitioning tree */
  /* decode: hufftree */
  long   *ptr0;
  long   *ptr1;

  /* auxiliary encoding information. Not used in decode */
  double *n;         /* decision hyperplanes: sum(x_i*n_i)[0<=i<dim]=c */ 
  double *c;
  long   aux;
  long   alloc;

} vqbook;

typedef struct {
  long   min;       /* packed 24 bit float */       
  long   delta;     /* packed 24 bit float */       
  int    quant;     /* 0 < quant <= 16 */
  int    sequencep; /* bitflag */
} quant_meta;

static inline double *_point(vqgen *v,long ptr){
  return v->pointlist+((v->elements+v->aux)*ptr);
}

static inline double *_aux(vqgen *v,long ptr){
  return _point(v,ptr)+v->aux;
}

static inline double *_now(vqgen *v,long ptr){
  return v->entrylist+(v->elements*ptr);
}

extern void vqgen_init(vqgen *v,int elements,int aux,int entries,
		       double  (*metric)(vqgen *,double *, double *),
		       double *(*weight)(vqgen *,double *));
extern void vqgen_addpoint(vqgen *v, double *p,double *aux);

extern double vqgen_iterate(vqgen *v);
extern void vqgen_unquantize(vqgen *v,quant_meta *q);
extern void vqgen_quantize(vqgen *v,quant_meta *q);
extern long float24_pack(double val);
extern double float24_unpack(long val);

extern void vqsp_book(vqgen *v,vqbook *b,long *quantlist);
extern int vqenc_entry(vqbook *b,double *val);

#endif





