/******************************************************************
 * CopyPolicy: GNU Public License 2 applies
 * Copyright (C) 1998-1999 Monty xiphmont@mit.edu, monty@xiph.org
 *
 * Stripped down FFT implementation from OggSquish.
 *
 ******************************************************************/

#ifndef _V_SMFT_H_
#define _V_SMFT_H_

#include "codec.h"

extern void drft_forward(drft_lookup *l,double *data);
extern void drft_backward(drft_lookup *l,double *data);
extern void drft_init(drft_lookup *l,int n);
extern void drft_clear(drft_lookup *l);

#endif
