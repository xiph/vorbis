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

 function: modified discrete cosine transform
           power of two length transform only [16 <= n ]

 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Jul 29 1999

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

/* Undef the following if you want a normal MDCT */
#define VORBIS_SPECIFIC_MODIFICATIONS

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mdct.h"

/* build lookups for trig functions; also pre-figure scaling and
   some window function algebra. */

void mdct_init(mdct_lookup *lookup,int n){
  int    *bitrev=malloc(sizeof(int)*(n/4));
  double *trig=malloc(sizeof(double)*(n+n/4));
  double *AE=trig;
  double *AO=AE+n/4;
  double *BE=AO+n/4;
  double *BO=BE+n/4;
  double *CE=BO+n/4;
  double *CO=CE+n/8;
  
  int *bitA=bitrev;
  int *bitB=bitrev+n/8;

  int i;
  int log2n=lookup->log2n=rint(log(n)/log(2));
  lookup->n=n;
  lookup->trig=trig;
  lookup->bitrev=bitrev;

  /* trig lookups... */

  for(i=0;i<n/4;i++){
    AE[i]=cos((M_PI/n)*(4*i));
    AO[i]=-sin((M_PI/n)*(4*i));
    BE[i]=cos((M_PI/(2*n))*(2*i+1));
    BO[i]=sin((M_PI/(2*n))*(2*i+1));
  }
  for(i=0;i<n/8;i++){
    CE[i]=cos((M_PI/n)*(4*i+2));
    CO[i]=-sin((M_PI/n)*(4*i+2));
  }

  /* bitreverse lookup... */

  {
    int mask=(1<<(log2n-1))-1,i,j;
    int msb=1<<(log2n-2);
    for(i=0;i<n/8;i++){
      int acc=0;
      for(j=0;msb>>j;j++)
	if((msb>>j)&i)acc|=1<<j;
      bitA[i]=((~acc)&mask)*2;
      bitB[i]=acc*2;
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

static inline void _mdct_kernel(double *x, 
				int n, int n2, int n4, int n8,
				mdct_lookup *init){
  double *w=x+1; /* interleaved access improves cache locality */ 
  int i;
  /* step 2 */

  {
    double *xA=x+n2;
    double *xB=x;
    double *w2=w+n2;
    double *AE=init->trig+n4;
    double *AO=AE+n4;

    for(i=0;i<n2;){
      double x0=xA[i]-xB[i];
      double x1=xA[i+2]-xB[i+2];
      AE-=2;AO-=2;

      w[i] =x0 * *AE + x1 * *AO;
      w2[i]=xA[i]+xB[i];
      i+=2;
      w[i] =x1 * *AE - x0 * *AO;
      w2[i]=xA[i]+xB[i];
      i+=2;
    }
  }

  /* step 3 */

  {
    int r,s;
    for(i=0;i<init->log2n-3;i++){
      int k0=n>>(i+1);
      int k1=1<<(i+2);
      int wbase=n-4;
      double *AE=init->trig;
      double *AO=AE+n4;
      double *temp;

      for(r=0;r<(n4>>i);r+=4){
	int w1=wbase;
	int w2=wbase-(k0>>1);
	wbase-=4;

	for(s=0;s<(2<<i);s++){
	  x[w1+2]=w[w1+2]+w[w2+2];
	  x[w1]  =w[w1]+w[w2];
	  x[w2+2]=(w[w1+2]-w[w2+2])* *AE-(w[w1]-w[w2])* *AO;
	  x[w2]  =(w[w1]-w[w2])* *AE+(w[w1+2]-w[w2+2])* *AO;
	  w1-=k0;
	  w2-=k0;
	}
	AE+=k1;
	AO+=k1;
      }

      temp=w;
      w=x;
      x=temp;
    }
  }


  /* step 4, 5, 6, 7 */
  {
    double *CE=init->trig+n;
    double *CO=CE+n8;
    int *bitA=init->bitrev;
    int *bitB=bitA+n8;
    double *x1=x;
    double *x2=x+n-2;
    for(i=0;i<n8;i++){
      int t1=bitA[i];
      int t4=bitB[i];
      int t2=t4+2;
      int t3=t1-2;

      double wA=w[t1]-w[t2];
      double wB=w[t3]+w[t4];
      double wC=w[t1]+w[t2];
      double wD=w[t3]-w[t4];

      double wACO=wA* *CO;
      double wBCO=wB* *(CO++);
      double wACE=wA* *CE;
      double wBCE=wB* *(CE++);

      *x1    =( wC+wACO+wBCE)*.5;
      *(x2-2)=( wC-wACO-wBCE)*.5;
      *(x1+2)=( wD+wBCO-wACE)*.5; 
      *x2    =(-wD+wBCO-wACE)*.5;
      x1+=4;
      x2-=4;
    }
  }
}

void mdct_forward(mdct_lookup *init, double *in, double *out, double *window){
  int n=init->n;
  double *x=alloca(n*sizeof(double));
  int n2=n>>1;
  int n4=n>>2;
  int n8=n>>3;
  int i;

  /* window + rotate + step 1 */
  {
    double tempA,tempB;
    int in1=n2+n4-4;
    int in2=in1+5;
    double *AE=init->trig+n4;
    double *AO=AE+n4;

    i=0;

    for(i=0;i<n4;i+=4){
      tempA= in[in1+2]*window[in1+2] + in[in2]*window[in2];
      tempB= in[in1]*window[in1] + in[in2+2]*window[in2+2];       
      in1 -=4;in2 +=4;
      AE--;AO--;
      x[i]=   tempB* *AO + tempA* *AE;
      x[i+2]= tempB* *AE - tempA* *AO;
    }

    in2=1;

    for(;i<n-n4;i+=4){
      tempA= in[in1+2]*window[in1+2] - in[in2]*window[in2];
      tempB= in[in1]*window[in1] - in[in2+2]*window[in2+2];       
      in1 -=4;in2 +=4;
      AE--;AO--;
      x[i]=   tempB* *AO + tempA* *AE;
      x[i+2]= tempB* *AE - tempA* *AO;
    }

    in1=n-4;

    for(;i<n;i+=4){
      tempA= -in[in1+2]*window[in1+2] - in[in2]*window[in2];
      tempB= -in[in1]*window[in1] - in[in2+2]*window[in2+2];       
      in1 -=4;in2 +=4;
      AE--;AO--;
      x[i]=   tempB* *AO + tempA* *AE;
      x[i+2]= tempB* *AE - tempA* *AO;
    }
  }

  _mdct_kernel(x,n,n2,n4,n8,init);

  /* step 8 */

  {
    double *BE=init->trig+n2;
    double *BO=BE+n4;
    double *out2=out+n2;
    for(i=0;i<n4;i++){
      out[i]   =x[0]* *BE+x[2]* *BO;
      *(--out2)=x[0]* *BO-x[2]* *BE;
      x+=4;
      BO++;
      BE++;
    }
  }
}

void mdct_backward(mdct_lookup *init, double *in, double *out, double *window){
  int n=init->n;
  double *x=alloca(n*sizeof(double));
  int n2=n>>1;
  int n4=n>>2;
  int n8=n>>3;
  int i;

  /* window + rotate + step 1 */
  {
    double *inO=in+1;
    double  *xO= x;
    double  *AE=init->trig+n4;
    double  *AO=AE+n4;

    for(i=0;i<n8;i++){
      AE--;AO--;
      *xO=-*(inO+2)* *AO - *inO * *AE;
      xO+=2;
      *xO= *inO * *AO - *(inO+2)* *AE;
      xO+=2;
      inO+=4;
    }

    inO=in+n2-4;

    for(i=0;i<n8;i++){
      AE--;AO--;
      *xO=*inO * *AO + *(inO+2) * *AE;
      xO+=2;
      *xO=*inO * *AE - *(inO+2) * *AO;
      xO+=2;
      inO-=4;
    }

  }

  _mdct_kernel(x,n,n2,n4,n8,init);

  /* step 8 */

  {
    double *BE=init->trig+n2;
    double *BO=BE+n4;
    int o1=n4,o2=o1-1;
    int o3=n4+n2,o4=o3-1;
    double scale=n/4.;
    
    for(i=0;i<n4;i++){
#ifdef VORBIS_SPECIFIC_MODIFICATIONS
      double temp1= (*x * *BO - *(x+2) * *BE)/ scale;
      double temp2= (*x * *BE + *(x+2) * *BO)/ -scale;
    
      out[o1]=-temp1*window[o1];
      out[o2]=temp1*window[o2];
      out[o3]=out[o4]=temp2;
#else
      double temp1= (*x * *BO - *(x+2) * *BE)* scale;
      double temp2= (*x * *BE + *(x+2) * *BO)* -scale;
    
      out[o1]=-temp1*window[o1];
      out[o2]=temp1*window[o2];
      out[o3]=temp2*window[o3];
      out[o4]=temp2*window[o4];
#endif

      o1++;
      o2--;
      o3++;
      o4--;
      x+=4;
      BE++;BO++;
    }
  }
}













