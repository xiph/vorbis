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

  function: LSP (also called LSF) conversion routines
  last mod: $Id: lsp.c,v 1.4.4.3 2000/04/21 16:35:39 xiphmont Exp $

  The LSP generation code is taken (with minimal modification) from
  "On the Computation of the LSP Frequencies" by Joseph Rothweiler
  <rothwlr@altavista.net>, available at:
  
  http://www2.xtdl.com/~rothwlr/lsfpaper/lsfpage.html 

 ********************************************************************/

/* Note that the lpc-lsp conversion finds the roots of polynomial with
   an iterative root polisher (CACM algorithm 283).  It *is* possible
   to confuse this algorithm into not converging; that should only
   happen with absurdly closely spaced roots (very sharp peaks in the
   LPC f response) which in turn should be impossible in our use of
   the code.  If this *does* happen anyway, it's a bug in the floor
   finder; find the cause of the confusion (probably a single bin
   spike or accidental near-double-limit resolution problems) and
   correct it. */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "lsp.h"
#include "os.h"

void vorbis_lsp_to_lpc(double *lsp,double *lpc,int m){ 
  int i,j,m2=m/2;
  double *O=alloca(sizeof(double)*m2);
  double *E=alloca(sizeof(double)*m2);
  double A;
  double *Ae=alloca(sizeof(double)*(m2+1));
  double *Ao=alloca(sizeof(double)*(m2+1));
  double B;
  double *Be=alloca(sizeof(double)*(m2));
  double *Bo=alloca(sizeof(double)*(m2));
  double temp;

  /* even/odd roots setup */
  for(i=0;i<m2;i++){
    O[i]=-2.*cos(lsp[i*2]);
    E[i]=-2.*cos(lsp[i*2+1]);
  }

  /* set up impulse response */
  for(j=0;j<m2;j++){
    Ae[j]=0.;
    Ao[j]=1.;
    Be[j]=0.;
    Bo[j]=1.;
  }
  Ao[j]=1.;
  Ae[j]=1.;

  /* run impulse response */
  for(i=1;i<m+1;i++){
    A=B=0.;
    for(j=0;j<m2;j++){
      temp=O[j]*Ao[j]+Ae[j];
      Ae[j]=Ao[j];
      Ao[j]=A;
      A+=temp;

      temp=E[j]*Bo[j]+Be[j];
      Be[j]=Bo[j];
      Bo[j]=B;
      B+=temp;
    }
    lpc[i-1]=(A+Ao[j]+B-Ae[j])/2;
    Ao[j]=A;
    Ae[j]=B;
  }
}

static void cheby(double *g, int ord) {
  int i, j;

  g[0] *= 0.5;
  for(i=2; i<= ord; i++) {
    for(j=ord; j >= i; j--) {
      g[j-2] -= g[j];
      g[j] += g[j]; 
    }
  }
}

static int comp(const void *a,const void *b){
  if(*(double *)a<*(double *)b)
    return(1);
  else
    return(-1);
}

/* CACM algorithm 283. */
static void cacm283(double *a,int ord,double *r){
  int i, k;
  double val, p, delta, error;
  double rooti;

  for(i=0; i<ord;i++) r[i] = 2.0 * (i+0.5) / ord - 1.0;
  
  for(error=1 ; error > 1.e-12; ) {
    error = 0;
    for( i=0; i<ord; i++) {  /* Update each point. */
      rooti = r[i];
      val = a[ord];
      p = a[ord];
      for(k=ord-1; k>= 0; k--) {
	val = val * rooti + a[k];
	if (k != i) p *= rooti - r[k];
      }
      delta = val/p;
      r[i] -= delta;

      error += delta*delta;
    }
  }
  
  /* Replaced the original bubble sort with a real sort.  With your
     help, we can eliminate the bubble sort in our lifetime. --Monty */
  
  qsort(r,ord,sizeof(double),comp);

}

/* Convert lpc coefficients to lsp coefficients */
void vorbis_lpc_to_lsp(double *lpc,double *lsp,int m){
  int order2=m/2;
  double *g1=alloca(sizeof(double)*(order2+1));
  double *g2=alloca(sizeof(double)*(order2+1));
  double *g1r=alloca(sizeof(double)*(order2+1));
  double *g2r=alloca(sizeof(double)*(order2+1));
  int i;

  /* Compute the lengths of the x polynomials. */
  /* Compute the first half of K & R F1 & F2 polynomials. */
  /* Compute half of the symmetric and antisymmetric polynomials. */
  /* Remove the roots at +1 and -1. */
  
  g1[order2] = 1.0;
  for(i=0;i<order2;i++) g1[order2-i-1] = lpc[i]+lpc[m-i-1];
  g2[order2] = 1.0;
  for(i=0;i<order2;i++) g2[order2-i-1] = lpc[i]-lpc[m-i-1];
  
  for(i=0; i<order2;i++) g1[order2-i-1] -= g1[order2-i];
  for(i=0; i<order2;i++) g2[order2-i-1] += g2[order2-i];

  /* Convert into polynomials in cos(alpha) */
  cheby(g1,order2);
  cheby(g2,order2);

  /* Find the roots of the 2 even polynomials.*/
  
  cacm283(g1,order2,g1r);
  cacm283(g2,order2,g2r);
  
  for(i=0;i<m;i+=2){
    lsp[i] = acos(g1r[i/2]);
    lsp[i+1] = acos(g2r[i/2]);
  }
}
