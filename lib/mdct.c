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

 function: normalized modified discrete cosine transform
           power of two length transform only [16 <= n ]
 last mod: $Id: mdct.c,v 1.16 2000/03/10 13:21:18 xiphmont Exp $

 Algorithm adapted from _The use of multirate filter banks for coding
 of high quality digital audio_, by T. Sporer, K. Brandenburg and
 B. Edler, collection of the European Signal Processing Conference
 (EUSIPCO), Amsterdam, June 1992, Vol.1, pp 211-214 

 Note that the below code won't make much sense without the paper;
 The presented algorithm was already fairly polished, and the code
 once followed it closely.  The current code both corrects several
 typos in the paper and goes beyond the presented optimizations 
 (steps 4 through 6 are, for example, entirely eliminated).

 This module DOES NOT INCLUDE code to generate the window function.
 Everybody has their own weird favorite including me... I happen to
 like the properties of y=sin(2PI*sin^2(x)), but others may vehemently
 disagree.

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mdct.h"
#include "os.h"
#include "misc.h"

/* build lookups for trig functions; also pre-figure scaling and
   some window function algebra. */

void mdct_init(mdct_lookup *lookup,int n){
  int    *bitrev=malloc(sizeof(int)*(n/4));
  double *trig=malloc(sizeof(double)*(n+n/4));
  double *AE=trig;
  double *AO=trig+1;
  double *BE=AE+n/2;
  double *BO=BE+1;
  double *CE=BE+n/2;
  double *CO=CE+1;
  
  int i;
  int log2n=lookup->log2n=rint(log(n)/log(2));
  lookup->n=n;
  lookup->trig=trig;
  lookup->bitrev=bitrev;

  /* trig lookups... */

  for(i=0;i<n/4;i++){
    AE[i*2]=cos((M_PI/n)*(4*i));
    AO[i*2]=-sin((M_PI/n)*(4*i));
    BE[i*2]=cos((M_PI/(2*n))*(2*i+1));
    BO[i*2]=sin((M_PI/(2*n))*(2*i+1));
  }
  for(i=0;i<n/8;i++){
    CE[i*2]=cos((M_PI/n)*(4*i+2));
    CO[i*2]=-sin((M_PI/n)*(4*i+2));
  }

  /* bitreverse lookup... */

  {
    int mask=(1<<(log2n-1))-1,i,j;
    int msb=1<<(log2n-2);
    for(i=0;i<n/8;i++){
      int acc=0;
      for(j=0;msb>>j;j++)
	if((msb>>j)&i)acc|=1<<j;
      bitrev[i*2]=((~acc)&mask);
      bitrev[i*2+1]=acc;
    }
  }
}

void mdct_clear(mdct_lookup *l){
  if(l){
    if(l->trig)free(l->trig);
    if(l->bitrev)free(l->bitrev);
    memset(l,0,sizeof(mdct_lookup));
  }
}

static double *_mdct_kernel(double *x, double *w,
			    int n, int n2, int n4, int n8,
			    mdct_lookup *init){
  int i;
  /* step 2 */

  {
    double *xA=x+n4;
    double *xB=x;
    double *w2=w+n4;
    double *A=init->trig+n2;

    for(i=0;i<n4;){
      double x0=*xA - *xB;
      double x1;
      w2[i]=    *xA++ + *xB++;


      x1=       *xA - *xB;
      A-=4;

      w[i++]=   x0 * A[0] + x1 * A[1];
      w[i]=     x1 * A[0] - x0 * A[1];

      w2[i++]=  *xA++ + *xB++;

    }
  }

  /* step 3 */

  {
    int r,s;
    for(i=0;i<init->log2n-3;i++){
      int k0=n>>(i+2);
      int k1=1<<(i+3);
      int wbase=n2-2;
      double *A=init->trig;
      double *temp;

      for(r=0;r<(k0>>2);r++){
        int w1=wbase;
	int w2=w1-(k0>>1);
	double AEv= A[0],wA;
	double AOv= A[1],wB;
	wbase-=2;

	k0++;
	for(s=0;s<(2<<i);s++){
	  wB     =w[w1]   -w[w2];
	  x[w1]  =w[w1]   +w[w2];

	  wA     =w[++w1] -w[++w2];
	  x[w1]  =w[w1]   +w[w2];

	  x[w2]  =wA*AEv  - wB*AOv;
	  x[w2-1]=wB*AEv  + wA*AOv;

	  w1-=k0;
	  w2-=k0;
	}
	k0--;

	A+=k1;
      }

      temp=w;
      w=x;
      x=temp;
    }
  }

  /* step 4, 5, 6, 7 */
  {
    double *C=init->trig+n;
    int *bit=init->bitrev;
    double *x1=x;
    double *x2=x+n2-1;
    for(i=0;i<n8;i++){
      int t1=*bit++;
      int t2=*bit++;

      double wA=w[t1]-w[t2+1];
      double wB=w[t1-1]+w[t2];
      double wC=w[t1]+w[t2+1];
      double wD=w[t1-1]-w[t2];

      double wACE=wA* *C;
      double wBCE=wB* *C++;
      double wACO=wA* *C;
      double wBCO=wB* *C++;
      
      *x1++=( wC+wACO+wBCE)*.5;
      *x2--=(-wD+wBCO-wACE)*.5;
      *x1++=( wD+wBCO-wACE)*.5; 
      *x2--=( wC-wACO-wBCE)*.5;
    }
  }
  return(x);
}

void mdct_forward(mdct_lookup *init, double *in, double *out){
  int n=init->n;
  double *x=alloca(sizeof(double)*(n/2));
  double *w=alloca(sizeof(double)*(n/2));
  double *xx;
  int n2=n>>1;
  int n4=n>>2;
  int n8=n>>3;
  int i;

  /* window + rotate + step 1 */
  {
    double tempA,tempB;
    int in1=n2+n4-4;
    int in2=in1+5;
    double *A=init->trig+n2;

    i=0;
    
    for(i=0;i<n8;i+=2){
      A-=2;
      tempA= in[in1+2] + in[in2];
      tempB= in[in1] + in[in2+2];       
      in1 -=4;in2 +=4;
      x[i]=   tempB*A[1] + tempA*A[0];
      x[i+1]= tempB*A[0] - tempA*A[1];
    }

    in2=1;

    for(;i<n2-n8;i+=2){
      A-=2;
      tempA= in[in1+2] - in[in2];
      tempB= in[in1] - in[in2+2];       
      in1 -=4;in2 +=4;
      x[i]=   tempB*A[1] + tempA*A[0];
      x[i+1]= tempB*A[0] - tempA*A[1];
    }
    
    in1=n-4;

    for(;i<n2;i+=2){
      A-=2;
      tempA= -in[in1+2] - in[in2];
      tempB= -in[in1] - in[in2+2];       
      in1 -=4;in2 +=4;
      x[i]=   tempB*A[1] + tempA*A[0];
      x[i+1]= tempB*A[0] - tempA*A[1];
    }
  }

  xx=_mdct_kernel(x,w,n,n2,n4,n8,init);

  /* step 8 */

  {
    double *B=init->trig+n2;
    double *out2=out+n2;
    double scale=4./n;
    for(i=0;i<n4;i++){
      out[i]   =(xx[0]*B[0]+xx[1]*B[1])*scale;
      *(--out2)=(xx[0]*B[1]-xx[1]*B[0])*scale;

      xx+=2;
      B+=2;
    }
  }
}

void mdct_backward(mdct_lookup *init, double *in, double *out){
  int n=init->n;
  double *x=alloca(sizeof(double)*(n/2));
  double *w=alloca(sizeof(double)*(n/2));
  double *xx;
  int n2=n>>1;
  int n4=n>>2;
  int n8=n>>3;
  int i;

  /* rotate + step 1 */
  {
    double *inO=in+1;
    double  *xO= x;
    double  *A=init->trig+n2;

    for(i=0;i<n8;i++){
      A-=2;
      *xO++=-*(inO+2)*A[1] - *inO*A[0];
      *xO++= *inO*A[1] - *(inO+2)*A[0];
      inO+=4;
    }

    inO=in+n2-4;

    for(i=0;i<n8;i++){
      A-=2;
      *xO++=*inO*A[1] + *(inO+2)*A[0];
      *xO++=*inO*A[0] - *(inO+2)*A[1];
      inO-=4;
    }

  }

  xx=_mdct_kernel(x,w,n,n2,n4,n8,init);

  /* step 8 */

  {
    double *B=init->trig+n2;
    int o1=n4,o2=o1-1;
    int o3=n4+n2,o4=o3-1;
    
    for(i=0;i<n4;i++){
      double temp1= (*xx * B[1] - *(xx+1) * B[0]);
      double temp2=-(*xx * B[0] + *(xx+1) * B[1]);
    
      out[o1]=-temp1;
      out[o2]= temp1;
      out[o3]= temp2;
      out[o4]= temp2;

      o1++;
      o2--;
      o3++;
      o4--;
      xx+=2;
      B+=2;
    }
  }
}
