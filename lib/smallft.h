/******************************************************************
 * CopyPolicy: GNU Public License 2 applies
 * Copyright (C) 1998 Monty xiphmont@mit.edu, monty@xiph.org
 *
 * FFT implementation from OggSquish, minus cosine transforms.
 * Only convenience functions exposed
 *
 ******************************************************************/

#ifndef _V_SMFT_H_
#define _V_SMFT_H_

extern void fft_forward(int n, double *buf, double *trigcache, int *sp);
extern void fft_backward(int n, double *buf, double *trigcache, int *sp);
extern void fft_i(int n, double **trigcache, int **splitcache);

#endif
