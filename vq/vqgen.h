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

  int    elements;
  int    quantbits;

  /* point cache */
  double *pointlist; 
  long   points;
  long   allocated;

  /* entries */
  double *entrylist;
  long   *assigned;
  double *bias;
  long   entries;

  double (*metric_func)   (struct vqgen *v,double *a,double *b);
} vqgen;

typedef struct vqbook{
  long elements;
  long entries;
  double *valuelist;
  long   *codelist;
  long   *lengthlist;

  /* auxiliary encoding/decoding information */
  long   *ptr0;
  long   *ptr1;

  /* auxiliary encoding information */
  double *n;
  double *c;
  long   aux;
  long   alloc;

} vqbook;

extern void vqgen_init(vqgen *v,int elements,int entries,
		       double (*metric)(vqgen *,double *, double *),
		       int quant);
extern void vqgen_addpoint(vqgen *v, double *p);
extern double *vqgen_midpoint(vqgen *v);
extern double vqgen_iterate(vqgen *v);
extern int vqenc_entry(vqbook *b,double *val);
extern void vqgen_book(vqgen *v,vqbook *b);

#endif





