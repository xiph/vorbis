/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS SOURCE IS GOVERNED BY *
 * THE GNU LESSER/LIBRARY PUBLIC LICENSE, WHICH IS INCLUDED WITH    *
 * THIS SOURCE. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.        *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2000             *
 * by Monty <monty@xiph.org> and the XIPHOPHORUS Company            *
 * http://www.xiph.org/                                             *
 *                                                                  *
 ********************************************************************

  function: LSP (also called LSF) conversion routines
  last mod: $Id: lsp.c,v 1.12 2000/11/06 00:07:01 xiphmont Exp $

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
   spike or accidental near-float-limit resolution problems) and
   correct it. */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "lsp.h"
#include "os.h"
#include "misc.h"
#include "lookup.h"
#include "scales.h"

/* three possible LSP to f curve functions; the exact computation
   (float), a lookup based float implementation, and an integer
   implementation.  The float lookup is likely the optimal choice on
   any machine with an FPU.  The integer implementation is *not* fixed
   point (due to the need for a large dynamic range and thus a
   seperately tracked exponent) and thus much more complex than the
   relatively simple float implementations. It's mostly for future
   work on a fully fixed point implementation for processors like the
   ARM family. */

/* undefine both for the 'old' but more precise implementation */
#define  FLOAT_LOOKUP
#undef   INT_LOOKUP

#ifdef FLOAT_LOOKUP
#include "lookup.c" /* catch this in the build system; we #include for
                       compilers (like gcc) that can't inline across
                       modules */

/* side effect: changes *lsp to cosines of lsp */
void vorbis_lsp_to_curve(float *curve,int *map,int n,int ln,float *lsp,int m,
			    float amp,float ampoffset){
  int i;
  float wdel=M_PI/ln;
  vorbis_fpu_control fpu;
  
  vorbis_fpu_setround(&fpu);
  for(i=0;i<m;i++)lsp[i]=vorbis_coslook(lsp[i]);

  i=0;
  while(i<n){
    int k=map[i];
    int qexp;
    float p=.7071067812;
    float q=.7071067812;
    float w=vorbis_coslook(wdel*k);
    float *ftmp=lsp;
    int c=m>>1;

    do{
      p*=ftmp[0]-w;
      q*=ftmp[1]-w;
      ftmp+=2;
    }while(--c);

    q=frexp(p*p*(1.+w)+q*q*(1.-w),&qexp);
    q=vorbis_fromdBlook(amp*             
			vorbis_invsqlook(q)*
			vorbis_invsq2explook(qexp+m)- 
			ampoffset);

    do{
      curve[i++]=q;
    }while(map[i]==k);
  }
  vorbis_fpu_restore(fpu);
}

#else

#ifdef INT_LOOKUP
#include "lookup.c" /* catch this in the build system; we #include for
                       compilers (like gcc) that can't inline across
                       modules */

static int MLOOP_1[64]={
   0,10,11,11, 12,12,12,12, 13,13,13,13, 13,13,13,13,
  14,14,14,14, 14,14,14,14, 14,14,14,14, 14,14,14,14,
  15,15,15,15, 15,15,15,15, 15,15,15,15, 15,15,15,15,
  15,15,15,15, 15,15,15,15, 15,15,15,15, 15,15,15,15,
};

static int MLOOP_2[64]={
  0,4,5,5, 6,6,6,6, 7,7,7,7, 7,7,7,7,
  8,8,8,8, 8,8,8,8, 8,8,8,8, 8,8,8,8,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
  9,9,9,9, 9,9,9,9, 9,9,9,9, 9,9,9,9,
};

static int MLOOP_3[8]={0,1,2,2,3,3,3,3};


/* side effect: changes *lsp to cosines of lsp */
void vorbis_lsp_to_curve(float *curve,int *map,int n,int ln,float *lsp,int m,
			    float amp,float ampoffset){

  /* 0 <= m < 256 */

  /* set up for using all int later */
  int i;
  int ampoffseti=rint(ampoffset*4096.);
  int ampi=rint(amp*16.);
  long *ilsp=alloca(m*sizeof(long));
  for(i=0;i<m;i++)ilsp[i]=vorbis_coslook_i(lsp[i]/M_PI*65536.+.5);

  i=0;
  while(i<n){
    int j,k=map[i];
    unsigned long pi=46341; /* 2**-.5 in 0.16 */
    unsigned long qi=46341;
    int qexp=0,shift;
    long wi=vorbis_coslook_i(k*65536/ln);

    pi*=labs(ilsp[0]-wi);
    qi*=labs(ilsp[1]-wi);

    for(j=2;j<m;j+=2){
      if(!(shift=MLOOP_1[(pi|qi)>>25]))
	if(!(shift=MLOOP_2[(pi|qi)>>19]))
	  shift=MLOOP_3[(pi|qi)>>16];
      pi=(pi>>shift)*labs(ilsp[j]-wi);
      qi=(qi>>shift)*labs(ilsp[j+1]-wi);
      qexp+=shift;
    }
    if(!(shift=MLOOP_1[(pi|qi)>>25]))
      if(!(shift=MLOOP_2[(pi|qi)>>19]))
	shift=MLOOP_3[(pi|qi)>>16];
    pi>>=shift;
    qi>>=shift;
    qexp+=shift-7*m;

    /* pi,qi normalized collectively, both tracked using qexp */

    /* p*=p(1-w), q*=q(1+w), let normalization drift because it isn't
       worth tracking step by step */

    pi=((pi*pi)>>16);
    qi=((qi*qi)>>16);
    qexp=qexp*2+m;

    qi*=(1<<14)-wi;
    pi*=(1<<14)+wi;
    
    qi=(qi+pi)>>14;

    /* we've let the normalization drift because it wasn't important;
       however, for the lookup, things must be normalized again.  We
       need at most one right shift or a number of left shifts */

    if(qi&0xffff0000){ /* checks for 1.xxxxxxxxxxxxxxxx */
      qi>>=1; qexp++; 
    }else
      while(qi && !(qi&0x8000)){ /* checks for 0.0xxxxxxxxxxxxxxx or less*/
	qi<<=1; qexp--; 
      }

    amp=vorbis_fromdBlook_i(ampi*                     /*  n.4         */
			    vorbis_invsqlook_i(qi,qexp)- 
			                              /*  m.8, m+n<=8 */
			    ampoffseti);              /*  8.12[0]     */

    curve[i]=amp;
    while(map[++i]==k)curve[i]=amp;
  }
}

#else 

/* old, nonoptimized but simple version for any poor sap who needs to
   figure out what the hell this code does, or wants the other tiny
   fraction of a dB precision */

/* side effect: changes *lsp to cosines of lsp */
void vorbis_lsp_to_curve(float *curve,int *map,int n,int ln,float *lsp,int m,
			    float amp,float ampoffset){
  int i;
  float wdel=M_PI/ln;
  for(i=0;i<m;i++)lsp[i]=2*cos(lsp[i]);

  i=0;
  while(i<n){
    int j,k=map[i];
    float p=.5;
    float q=.5;
    float w=2*cos(wdel*k);
    for(j=0;j<m;j+=2){
      p *= w-lsp[j];
      q *= w-lsp[j+1];
    }
    p*=p*(2.+w);
    q*=q*(2.-w);
    q=fromdB(amp/sqrt(p+q)-ampoffset);

    curve[i]=q;
    while(map[++i]==k)curve[i]=q;
  }
}

#endif
#endif

static void cheby(float *g, int ord) {
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
  if(*(float *)a<*(float *)b)
    return(1);
  else
    return(-1);
}

/* This is one of those 'mathemeticians should not write code' kind of
   cases.  Newton's method of polishing roots is straightforward
   enough... except in those cases where it just fails in the real
   world.  In our case below, we're worried about a local mini/maxima
   shooting a root estimation off to infinity, or the new estimation
   chaotically oscillating about convergence (shouldn't actually be a
   problem in our usage.

   Maehly's modification (zero suppression, to prevent two tenative
   roots from collapsing to the same actual root) similarly can
   temporarily shoot a root off toward infinity.  It would come
   back... if it were not for the fact that machine representation has
   limited dynamic range and resolution.  This too is guarded by
   limiting delta.

   Last problem is convergence criteria; we don't know what a 'double'
   is on our hardware/compiler, and the convergence limit is bounded
   by roundoff noise.  So, we hack convergence:

   Require at most 1e-6 mean squared error for all zeroes.  When
   converging, start the clock ticking at 1e-6; limit our polishing to
   as many more iterations as took us to get this far, 100 max.

   Past max iters, quit when MSE is no longer decreasing *or* we go
   below ~1e-20 MSE, whichever happens first. */

static void Newton_Raphson_Maehly(float *a,int ord,float *r){
  int i, k, count=0, maxiter=0;
  double error=1.,besterror=1.;
  double *root=alloca(ord*sizeof(double));

  for(i=0; i<ord;i++) root[i] = 2.0 * (i+0.5) / ord - 1.0;
  
  while(error>1.e-20){
    error=0;
    
    for(i=0; i<ord; i++) { /* Update each point. */
      double ac=0.,pp=0.,delta;
      double rooti=root[i];
      double p=a[ord];
      for(k=ord-1; k>= 0; k--) {

	pp= pp* rooti + p;
	p = p * rooti+ a[k];
	if (k != i) ac += 1./(rooti - root[k]);
      }
      ac=p*ac;

      delta = p/(pp-ac);

      /* don't allow the correction to scream off into infinity if we
         happened to polish right at a local mini/maximum */

      if(delta<-3)delta=-3;
      if(delta>3.)delta=3.; /* 3 is not a random choice; it's large
                               enough to make sure the first pass
                               can't accidentally limit two poles to
                               the same value in a fatal nonelastic
                               collision.  */

      root[i] -= delta;
      error += delta*delta;
    }
    
    if(maxiter && count>maxiter && error>=besterror)break;

    /* anything to help out the polisher; converge using doubles */
    if(!count || error<besterror){
      for(i=0; i<ord; i++) r[i]=root[i]; 
      besterror=error;
      if(error<1.e-6){ /* rough minimum criteria */
	maxiter=count*2+10;
	if(maxiter>100)maxiter=100;
      }
    }

    count++;
  }

  /* Replaced the original bubble sort with a real sort.  With your
     help, we can eliminate the bubble sort in our lifetime. --Monty */
  
  qsort(r,ord,sizeof(float),comp);

}

/* Convert lpc coefficients to lsp coefficients */
void vorbis_lpc_to_lsp(float *lpc,float *lsp,int m){
  int order2=m/2;
  float *g1=alloca(sizeof(float)*(order2+1));
  float *g2=alloca(sizeof(float)*(order2+1));
  float *g1r=alloca(sizeof(float)*(order2+1));
  float *g2r=alloca(sizeof(float)*(order2+1));
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
  
  Newton_Raphson_Maehly(g1,order2,g1r);
  Newton_Raphson_Maehly(g2,order2,g2r);
  
  for(i=0;i<m;i+=2){
    lsp[i] = acos(g1r[i/2]);
    lsp[i+1] = acos(g2r[i/2]);
  }
}
