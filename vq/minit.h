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

  function: linear programming (dual simplex) solver includes

 ********************************************************************/

#ifndef _V_MINIT_H_
#define _V_MINIT_H_

typedef struct {
  long m;
  long n;
  long p;
  long l;
  
  long *ind1;
  long *chk;
  long *ind;
  double **e;

} minit;

extern void minit_init(minit *l,long n,long m,long p);
extern void minit_clear(minit *l);
extern int minit_solve(minit *l,double **A,double *b,double *c,double td,
		       double *x,double *w,double *z);

#endif 
