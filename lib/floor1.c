/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2001             *
 * by the XIPHOPHORUS Company http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: floor backend 1 implementation
 last mod: $Id: floor1.c,v 1.3 2001/06/05 23:54:25 xiphmont Exp $

 ********************************************************************/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ogg/ogg.h>
#include "vorbis/codec.h"
#include "codec_internal.h"
#include "registry.h"
#include "codebook.h"
#include "misc.h"
#include "scales.h"

#include <stdio.h>

#define floor1_rangedB 140 /* floor 1 fixed at -140dB to 0dB range */

typedef struct {
  int sorted_index[VIF_POSIT+2];
  int forward_index[VIF_POSIT+2];
  int reverse_index[VIF_POSIT+2];
  
  int hineighbor[VIF_POSIT];
  int loneighbor[VIF_POSIT];
  int posts;

  int n;
  int quant_q;
  vorbis_info_floor1 *vi;


  long seq;
  long postbits;
  long classbits;
  long subbits;
  float mse;
} vorbis_look_floor1;

typedef struct lsfit_acc{
  long x0;
  long x1;

  long xa;
  long ya;
  long x2a;
  long y2a;
  long xya; 
  long n;
  long an;
  long un;
  long edgey0;
  long edgey1;
} lsfit_acc;

/***********************************************/
 
static vorbis_info_floor *floor1_copy_info (vorbis_info_floor *i){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  vorbis_info_floor1 *ret=_ogg_malloc(sizeof(vorbis_info_floor1));
  memcpy(ret,info,sizeof(vorbis_info_floor1));
  return(ret);
}

static void floor1_free_info(vorbis_info_floor *i){
  if(i){
    memset(i,0,sizeof(vorbis_info_floor1));
    _ogg_free(i);
  }
}

static void floor1_free_look(vorbis_look_floor *i){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)i;
  if(i){
    fprintf(stderr,"floor 1 bit usage: %ld:%ld:%ld (%ld/frame), mse:%gdB\n",
	    look->postbits/look->seq,look->classbits/look->seq,look->subbits/look->seq,
	    (look->postbits+look->subbits+look->classbits)/look->seq,
	    sqrt(look->mse/look->seq));

    memset(look,0,sizeof(vorbis_look_floor1));
    free(i);
  }
}

static int ilog(unsigned int v){
  int ret=0;
  while(v){
    ret++;
    v>>=1;
  }
  return(ret);
}

static int ilog2(unsigned int v){
  int ret=0;
  while(v>1){
    ret++;
    v>>=1;
  }
  return(ret);
}

static void floor1_pack (vorbis_info_floor *i,oggpack_buffer *opb){
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)i;
  int j,k;
  int count=0;
  int rangebits;
  int maxposit=info->postlist[1];
  int maxclass=-1;

  /* save out partitions */
  oggpack_write(opb,info->partitions,5); /* only 0 to 31 legal */
  for(j=0;j<info->partitions;j++){
    oggpack_write(opb,info->partitionclass[j],4); /* only 0 to 15 legal */
    if(maxclass<info->partitionclass[j])maxclass=info->partitionclass[j];
  }

  /* save out partition classes */
  for(j=0;j<maxclass+1;j++){
    oggpack_write(opb,info->class_dim[j]-1,3); /* 1 to 8 */
    oggpack_write(opb,info->class_subs[j],2); /* 0 to 3 */
    if(info->class_subs[j])oggpack_write(opb,info->class_book[j],8);
    for(k=0;k<(1<<info->class_subs[j]);k++)
      oggpack_write(opb,info->class_subbook[j][k]+1,8);
  }

  /* save out the post list */
  oggpack_write(opb,info->mult-1,2);     /* only 1,2,3,4 legal now */ 
  oggpack_write(opb,ilog2(maxposit),4);
  rangebits=ilog2(maxposit);

  for(j=0,k=0;j<info->partitions;j++){
    count+=info->class_dim[info->partitionclass[j]]; 
    for(;k<count;k++)
      oggpack_write(opb,info->postlist[k+2],rangebits);
  }
}


static vorbis_info_floor *floor1_unpack (vorbis_info *vi,oggpack_buffer *opb){
  codec_setup_info     *ci=vi->codec_setup;
  int j,k,count=0,maxclass=-1,rangebits;

  vorbis_info_floor1 *info=_ogg_calloc(1,sizeof(vorbis_info_floor1));
  /* read partitions */
  info->partitions=oggpack_read(opb,5); /* only 0 to 31 legal */
  for(j=0;j<info->partitions;j++){
    info->partitionclass[j]=oggpack_read(opb,4); /* only 0 to 15 legal */
    if(maxclass<info->partitionclass[j])maxclass=info->partitionclass[j];
  }

  /* read partition classes */
  for(j=0;j<maxclass+1;j++){
    info->class_dim[j]=oggpack_read(opb,3)+1; /* 1 to 8 */
    info->class_subs[j]=oggpack_read(opb,2); /* 0,1,2,3 bits */
    if(info->class_subs[j]<0)
      goto err_out;
    if(info->class_subs[j])info->class_book[j]=oggpack_read(opb,8);
    if(info->class_book[j]<0 || info->class_book[j]>=ci->books)
      goto err_out;
    for(k=0;k<(1<<info->class_subs[j]);k++){
      info->class_subbook[j][k]=oggpack_read(opb,8)-1;
      if(info->class_subbook[j][k]<-1 || info->class_subbook[j][k]>=ci->books)
	goto err_out;
    }
  }

  /* read the post list */
  info->mult=oggpack_read(opb,2)+1;     /* only 1,2,3,4 legal now */ 
  rangebits=oggpack_read(opb,4);

  for(j=0,k=0;j<info->partitions;j++){
    count+=info->class_dim[info->partitionclass[j]]; 
    for(;k<count;k++){
      int t=info->postlist[k+2]=oggpack_read(opb,rangebits);
      if(t<0 || t>=(1<<rangebits))
	goto err_out;
    }
  }
  info->postlist[0]=0;
  info->postlist[1]=1<<rangebits;

  return(info);
  
 err_out:
  floor1_free_info(info);
  return(NULL);
}

static int icomp(const void *a,const void *b){
  return(**(int **)a-**(int **)b);
}

static vorbis_look_floor *floor1_look(vorbis_dsp_state *vd,vorbis_info_mode *mi,
                              vorbis_info_floor *in){

  int *sortpointer[VIF_POSIT+2];
  vorbis_info_floor1 *info=(vorbis_info_floor1 *)in;
  vorbis_look_floor1 *look=_ogg_calloc(1,sizeof(vorbis_look_floor1));
  int i,j,n=0;

  look->vi=info;
  look->n=info->postlist[1];
 
  /* we drop each position value in-between already decoded values,
     and use linear interpolation to predict each new value past the
     edges.  The positions are read in the order of the position
     list... we precompute the bounding positions in the lookup.  Of
     course, the neighbors can change (if a position is declined), but
     this is an initial mapping */

  for(i=0;i<info->partitions;i++)n+=info->class_dim[info->partitionclass[i]];
  n+=2;
  look->posts=n;

  /* also store a sorted position index */
  for(i=0;i<n;i++)sortpointer[i]=info->postlist+i;
  qsort(sortpointer,n,sizeof(int),icomp);

  /* points from sort order back to range number */
  for(i=0;i<n;i++)look->forward_index[i]=sortpointer[i]-info->postlist;
  /* points from range order to sorted position */
  for(i=0;i<n;i++)look->reverse_index[look->forward_index[i]]=i;
  /* we actually need the post values too */
  for(i=0;i<n;i++)look->sorted_index[i]=info->postlist[look->forward_index[i]];
  
  /* quantize values to multiplier spec */
  switch(info->mult){
  case 1: /* 1024 -> 256 */
    look->quant_q=256;
    break;
  case 2: /* 1024 -> 128 */
    look->quant_q=128;
    break;
  case 3: /* 1024 -> 86 */
    look->quant_q=86;
    break;
  case 4: /* 1024 -> 64 */
    look->quant_q=64;
    break;
  }

  /* discover our neighbors for decode where we don't use fit flags
     (that would push the neighbors outward) */
  for(i=0;i<n-2;i++){
    int lo=0;
    int hi=1;
    int lx=0;
    int hx=look->n;
    int currentx=info->postlist[i+2];
    for(j=0;j<i+2;j++){
      int x=info->postlist[j];
      if(x>lx && x<currentx){
	lo=j;
	lx=x;
      }
      if(x<hx && x>currentx){
	hi=j;
	hx=x;
      }
    }
    look->loneighbor[i]=lo;
    look->hineighbor[i]=hi;
  }

  return(look);
}

static int render_point(int x0,int x1,int y0,int y1,int x){
  y0&=0x7fff; /* mask off flag */
  y1&=0x7fff;
    
  {
    int dy=y1-y0;
    int adx=x1-x0;
    int ady=abs(dy);
    int err=ady*(x-x0);
    
    int off=err/adx;
    if(dy<0)return(y0-off);
    return(y0+off);
  }
}

static int vorbis_dBquant(const float *x){
  int i= *x*7.3142857f+1023.5f;
  if(i>1023)return(1023);
  if(i<0)return(0);
  return i;
}

static float FLOOR_fromdB_LOOKUP[256]={
	1.0649863e-07F, 1.1341951e-07F, 1.2079015e-07F, 1.2863978e-07F, 
	1.3699951e-07F, 1.4590251e-07F, 1.5538408e-07F, 1.6548181e-07F, 
	1.7623575e-07F, 1.8768855e-07F, 1.9988561e-07F, 2.128753e-07F, 
	2.2670913e-07F, 2.4144197e-07F, 2.5713223e-07F, 2.7384213e-07F, 
	2.9163793e-07F, 3.1059021e-07F, 3.3077411e-07F, 3.5226968e-07F, 
	3.7516214e-07F, 3.9954229e-07F, 4.2550680e-07F, 4.5315863e-07F, 
	4.8260743e-07F, 5.1396998e-07F, 5.4737065e-07F, 5.8294187e-07F, 
	6.2082472e-07F, 6.6116941e-07F, 7.0413592e-07F, 7.4989464e-07F, 
	7.9862701e-07F, 8.5052630e-07F, 9.0579828e-07F, 9.6466216e-07F, 
	1.0273513e-06F, 1.0941144e-06F, 1.1652161e-06F, 1.2409384e-06F, 
	1.3215816e-06F, 1.4074654e-06F, 1.4989305e-06F, 1.5963394e-06F, 
	1.7000785e-06F, 1.8105592e-06F, 1.9282195e-06F, 2.0535261e-06F, 
	2.1869758e-06F, 2.3290978e-06F, 2.4804557e-06F, 2.6416497e-06F, 
	2.8133190e-06F, 2.9961443e-06F, 3.1908506e-06F, 3.3982101e-06F, 
	3.6190449e-06F, 3.8542308e-06F, 4.1047004e-06F, 4.3714470e-06F, 
	4.6555282e-06F, 4.9580707e-06F, 5.2802740e-06F, 5.6234160e-06F, 
	5.9888572e-06F, 6.3780469e-06F, 6.7925283e-06F, 7.2339451e-06F, 
	7.7040476e-06F, 8.2047000e-06F, 8.7378876e-06F, 9.3057248e-06F, 
	9.9104632e-06F, 1.0554501e-05F, 1.1240392e-05F, 1.1970856e-05F, 
	1.2748789e-05F, 1.3577278e-05F, 1.4459606e-05F, 1.5399272e-05F, 
	1.6400004e-05F, 1.7465768e-05F, 1.8600792e-05F, 1.9809576e-05F, 
	2.1096914e-05F, 2.2467911e-05F, 2.3928002e-05F, 2.5482978e-05F, 
	2.7139006e-05F, 2.8902651e-05F, 3.0780908e-05F, 3.2781225e-05F, 
	3.4911534e-05F, 3.7180282e-05F, 3.9596466e-05F, 4.2169667e-05F, 
	4.4910090e-05F, 4.7828601e-05F, 5.0936773e-05F, 5.4246931e-05F, 
	5.7772202e-05F, 6.1526565e-05F, 6.5524908e-05F, 6.9783085e-05F, 
	7.4317983e-05F, 7.9147585e-05F, 8.4291040e-05F, 8.9768747e-05F, 
	9.5602426e-05F, 0.00010181521F, 0.00010843174F, 0.00011547824F, 
	0.00012298267F, 0.00013097477F, 0.00013948625F, 0.00014855085F, 
	0.00015820453F, 0.00016848555F, 0.00017943469F, 0.00019109536F, 
	0.00020351382F, 0.00021673929F, 0.00023082423F, 0.00024582449F, 
	0.00026179955F, 0.00027881276F, 0.00029693158F, 0.00031622787F, 
	0.00033677814F, 0.00035866388F, 0.00038197188F, 0.00040679456F, 
	0.00043323036F, 0.00046138411F, 0.00049136745F, 0.00052329927F, 
	0.00055730621F, 0.00059352311F, 0.00063209358F, 0.00067317058F, 
	0.00071691700F, 0.00076350630F, 0.00081312324F, 0.00086596457F, 
	0.00092223983F, 0.00098217216F, 0.0010459992F, 0.0011139742F, 
	0.0011863665F, 0.0012634633F, 0.0013455702F, 0.0014330129F, 
	0.0015261382F, 0.0016253153F, 0.0017309374F, 0.0018434235F, 
	0.0019632195F, 0.0020908006F, 0.0022266726F, 0.0023713743F, 
	0.0025254795F, 0.0026895994F, 0.0028643847F, 0.0030505286F, 
	0.0032487691F, 0.0034598925F, 0.0036847358F, 0.0039241906F, 
	0.0041792066F, 0.0044507950F, 0.0047400328F, 0.0050480668F, 
	0.0053761186F, 0.0057254891F, 0.0060975636F, 0.0064938176F, 
	0.0069158225F, 0.0073652516F, 0.0078438871F, 0.0083536271F, 
	0.0088964928F, 0.009474637F, 0.010090352F, 0.010746080F, 
	0.011444421F, 0.012188144F, 0.012980198F, 0.013823725F, 
	0.014722068F, 0.015678791F, 0.016697687F, 0.017782797F, 
	0.018938423F, 0.020169149F, 0.021479854F, 0.022875735F, 
	0.024362330F, 0.025945531F, 0.027631618F, 0.029427276F, 
	0.031339626F, 0.033376252F, 0.035545228F, 0.037855157F, 
	0.040315199F, 0.042935108F, 0.045725273F, 0.048696758F, 
	0.051861348F, 0.055231591F, 0.058820850F, 0.062643361F, 
	0.066714279F, 0.071049749F, 0.075666962F, 0.080584227F, 
	0.085821044F, 0.091398179F, 0.097337747F, 0.10366330F, 
	0.11039993F, 0.11757434F, 0.12521498F, 0.13335215F, 
	0.14201813F, 0.15124727F, 0.16107617F, 0.17154380F, 
	0.18269168F, 0.19456402F, 0.20720788F, 0.22067342F, 
	0.23501402F, 0.25028656F, 0.26655159F, 0.28387361F, 
	0.30232132F, 0.32196786F, 0.34289114F, 0.36517414F, 
	0.38890521F, 0.41417847F, 0.44109412F, 0.46975890F, 
	0.50028648F, 0.53279791F, 0.56742212F, 0.60429640F, 
	0.64356699F, 0.68538959F, 0.72993007F, 0.77736504F, 
	0.82788260F, 0.88168307F, 0.9389798F, 1.F, 
};

static void render_line(int x0,int x1,int y0,int y1,float *d){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;

  ady-=abs(base*adx);

  d[x]=FLOOR_fromdB_LOOKUP[y];
  while(++x<x1){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }
    d[x]=FLOOR_fromdB_LOOKUP[y];
  }
}

/* the floor has already been filtered to only include relevant sections */
static int accumulate_fit(const float *flr,const float *mdct,
			  int x0, int x1,lsfit_acc *a,
			  int n,vorbis_info_floor1 *info){
  long i;
  int quantized=vorbis_dBquant(flr);

  long xa=0,ya=0,x2a=0,y2a=0,xya=0,na=0, xb=0,yb=0,x2b=0,y2b=0,xyb=0,nb=0;

  memset(a,0,sizeof(lsfit_acc));
  a->x0=x0;
  a->x1=x1;
  a->edgey0=quantized;
  if(x1>n)x1=n;

  for(i=x0;i<x1;i++){
    int quantized=vorbis_dBquant(flr+i);
    if(quantized){
      if(mdct[i]+info->twofitatten>=flr[i]){
	xa  += i;
	ya  += quantized;
	x2a += i*i;
	y2a += quantized*quantized;
	xya += i*quantized;
	na++;
      }else{
	xb  += i;
	yb  += quantized;
	x2b += i*i;
	y2b += quantized*quantized;
	xyb += i*quantized;
	nb++;
      }
    }
  }

  xb+=xa;
  yb+=ya;
  x2b+=x2a;
  y2b+=y2a;
  xyb+=xya;
  nb+=na;

  /* weight toward the actually used frequencies if we meet the threshhold */
  {
    int weight;
    if(nb<info->twofitminsize || na<info->twofitminused){
      weight=0;
    }else{
      weight=nb*info->twofitweight/na;
    }
    a->xa=xa*weight+xb;
    a->ya=ya*weight+yb;
    a->x2a=x2a*weight+x2b;
    a->y2a=y2a*weight+y2b;
    a->xya=xya*weight+xyb;
    a->an=na*weight+nb;
    a->n=nb;
    a->un=na;
    if(nb>=info->unusedminsize)a->un++;
  }

  a->edgey1=-200;
  if(x1<n){
    int quantized=vorbis_dBquant(flr+i);
    a->edgey1=quantized;
  }
  return(a->n);
}

/* returns < 0 on too few points to fit, >=0 (meansq error) on success */
static int fit_line(lsfit_acc *a,int fits,int *y0,int *y1){
  long x=0,y=0,x2=0,y2=0,xy=0,n=0,an=0,i;
  long x0=a[0].x0;
  long x1=a[fits-1].x1;

  for(i=0;i<fits;i++){
    if(a[i].un){
      x+=a[i].xa;
      y+=a[i].ya;
      x2+=a[i].x2a;
      y2+=a[i].y2a;
      xy+=a[i].xya;
      n+=a[i].n;
      an+=a[i].an;
    }
  }

  if(*y0>=0){  /* hint used to break degenerate cases */
    x+=   x0;
    y+=  *y0;
    x2+=  x0 *  x0;
    y2+= *y0 * *y0;
    xy+= *y0 *  x0;
    n++;
    an++;
  }

  if(*y1>=0){  /* hint used to break degenerate cases */
    x+=   x1;
    y+=  *y1;
    x2+=  x1 *  x1;
    y2+= *y1 * *y1;
    xy+= *y1 *  x1;
    n++;
    an++;
  }

  if(n<2)return(n-2);
  
  {
    /* need 64 bit multiplies, which C doesn't give portably as int */
    double fx=x;
    double fy=y;
    double fx2=x2;
    double fy2=y2;
    double fxy=xy;
    double denom=1./(an*fx2-fx*fx);
    double a=(fy*fx2-fxy*fx)*denom;
    double b=(an*fxy-fx*fy)*denom;
    *y0=rint(a+b*x0);
    *y1=rint(a+b*x1);

    /* limit to our range! */
    if(*y0>1023)*y0=1023;
    if(*y1>1023)*y1=1023;
    if(*y0<0)*y0=0;
    if(*y1<0)*y1=0;

    return(0);
  }
}

static void fit_line_point(lsfit_acc *a,int fits,int *y0,int *y1){
  long y=0;
  int i;

  for(i=0;i<fits && y==0;i++)
    y+=a[i].ya;
  
  *y0=*y1=y;
}

static int inspect_error(int x0,int x1,int y0,int y1,const float *mask,
			 const float *mdct,
			 vorbis_info_floor1 *info){
  int dy=y1-y0;
  int adx=x1-x0;
  int ady=abs(dy);
  int base=dy/adx;
  int sy=(dy<0?base-1:base+1);
  int x=x0;
  int y=y0;
  int err=0;
  int val=vorbis_dBquant(mask+x);
  int mse=0;
  int n=0;

  ady-=abs(base*adx);
  
  if(mdct[x]+info->twofitatten>=mask[x]){
    if(y+info->maxover<val)return(1);
    if(y-info->maxunder>val)return(1);
    mse=(y-val);
    mse*=mse;
    n++;
  }

  while(++x<x1){
    err=err+ady;
    if(err>=adx){
      err-=adx;
      y+=sy;
    }else{
      y+=base;
    }

    if(mdct[x]+info->twofitatten>=mask[x]){
      val=vorbis_dBquant(mask+x);
      if(val){
	if(y+info->maxover<val)return(1);
	if(y-info->maxunder>val)return(1);
	mse+=((y-val)*(y-val));
	n++;
      }
    }
  }
  
  if(n){
    if(info->maxover*info->maxover/n>info->maxerr)return(0);
    if(info->maxunder*info->maxunder/n>info->maxerr)return(0);
    if(mse/n>info->maxerr)return(1);
  }
  return(0);
}

static int post_Y(int *A,int *B,int pos){
  if(A[pos]<0)
    return B[pos];
  if(B[pos]<0)
    return A[pos];
  return (A[pos]+B[pos])>>1;
}

static int floor1_forward(vorbis_block *vb,vorbis_look_floor *in,
			  const float *mdct, const float *logmdct,   /* in */
			  const float *logmask, const float *logmax, /* in */
			  float *residue, float *codedflr){          /* out */
  static int seq=0;
  long i,j,k,l;
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;
  long n=look->n;
  long posts=look->posts;
  long nonzero=0;
  lsfit_acc fits[VIF_POSIT+1];
  int fit_valueA[VIF_POSIT+2]; /* index by range list position */
  int fit_valueB[VIF_POSIT+2]; /* index by range list position */
  int fit_flag[VIF_POSIT+2];

  int loneighbor[VIF_POSIT+2]; /* sorted index of range list position (+2) */
  int hineighbor[VIF_POSIT+2]; 
  int memo[VIF_POSIT+2];
  codec_setup_info *ci=vb->vd->vi->codec_setup;
  static_codebook **sbooks=ci->book_param;
  codebook *books=((backend_lookup_state *)(vb->vd->backend_state))->
    fullbooks;   

  memset(fit_flag,0,sizeof(fit_flag));
  for(i=0;i<posts;i++)loneighbor[i]=0; /* 0 for the implicit 0 post */
  for(i=0;i<posts;i++)hineighbor[i]=1; /* 1 for the implicit post at n */
  for(i=0;i<posts;i++)memo[i]=-1;      /* no neighbor yet */

  /* Scan back from high edge to first 'used' frequency */
  for(;n>info->unusedmin_n;n--)
    if(logmdct[n-1]>-floor1_rangedB && 
       logmdct[n-1]+info->twofitatten>logmask[n-1])break;

  /* quantize the relevant floor points and collect them into line fit
     structures (one per minimal division) at the same time */
  if(posts==0){
    nonzero+=accumulate_fit(logmask,logmax,0,n,fits,n,info);
  }else{
    for(i=0;i<posts-1;i++)
      nonzero+=accumulate_fit(logmask,logmax,look->sorted_index[i],
			      look->sorted_index[i+1],fits+i,
			      n,info);
  }
  
  if(nonzero){
    /* start by fitting the implicit base case.... */
    int y0=-200;
    int y1=-200;
    int mse=fit_line(fits,posts-1,&y0,&y1);
    if(mse<0){
      /* Only a single nonzero point */
      y0=-200;
      y1=0;
      fit_line(fits,posts-1,&y0,&y1);
    }

    look->seq++;

    fit_flag[0]=1;
    fit_flag[1]=1;
    fit_valueA[0]=y0;
    fit_valueB[0]=y0;
    fit_valueB[1]=y1;
    fit_valueA[1]=y1;

    if(mse>=0){
      /* Non degenerate case */
      /* start progressive splitting.  This is a greedy, non-optimal
	 algorithm, but simple and close enough to the best
	 answer. */
      for(i=2;i<posts;i++){
	int sortpos=look->reverse_index[i];
	int ln=loneighbor[sortpos];
	int hn=hineighbor[sortpos];

	/* eliminate repeat searches of a particular range with a memo */
	if(memo[ln]!=hn){
	  /* haven't performed this error search yet */
	  int lsortpos=look->reverse_index[ln];
	  int hsortpos=look->reverse_index[hn];
	  memo[ln]=hn;

	  /* if this is an empty segment, its endpoints don't matter.
	     Mark as such */
	  for(j=lsortpos;j<hsortpos;j++)
	    if(fits[j].un)break;
	  if(j==hsortpos){
	    /* empty segment; important to note that this does not
               break 0/n post case */
	    fit_valueB[ln]=-200;
	    if(fit_valueA[ln]<0)
	      fit_flag[ln]=0;
	    fit_valueA[hn]=-200;
	    if(fit_valueB[hn]<0)
	      fit_flag[hn]=0;
 
	  }else{
	    /* A note: we want to bound/minimize *local*, not global, error */
	    int lx=info->postlist[ln];
	    int hx=info->postlist[hn];	  
	    int ly=post_Y(fit_valueA,fit_valueB,ln);
	    int hy=post_Y(fit_valueA,fit_valueB,hn);
	    
	    if(inspect_error(lx,hx,ly,hy,logmask,logmdct,info)){
	      /* outside error bounds/begin search area.  Split it. */
	      int ly0=-200;
	      int ly1=-200;
	      int hy0=-200;
	      int hy1=-200;
	      int lmse=fit_line(fits+lsortpos,sortpos-lsortpos,&ly0,&ly1);
	      int hmse=fit_line(fits+sortpos,hsortpos-sortpos,&hy0,&hy1);
	      
	      /* the boundary/sparsity cases are the hard part.  They
                 don't happen often given that we use the full mask
                 curve (weighted) now, but when they do happen they
                 can go boom. Pay them detailed attention */
	      /* cases for a segment:
		 >=0) normal fit (>=2 unique points)
		 -1) one point on x0;
		 one point on x1; <-- disallowed by fit_line
		 -2) one point in between x0 and x1
		 -3) no points */

	      switch(lmse){ 
	      case -2:  
		/* no points in the low segment */
		break;
	      case -1:
		ly0=fits[lsortpos].edgey0;
		break;
	      default:
	      }

	      switch(hmse){ 
	      case -2:  
		/* no points in the hi segment */
		break;
	      case -1:
		hy0=fits[sortpos].edgey0;
		break;
	      }

	      /* store new edge values */
	      fit_valueB[ln]=ly0;
	      if(ln==0 && ly0>=0)fit_valueA[ln]=ly0;
	      fit_valueA[i]=ly1;
	      fit_valueB[i]=hy0;
	      fit_valueA[hn]=hy1;
	      if(hn==1 && hy1>=0)fit_valueB[hn]=hy1;

	      if(ly0<0 && fit_valueA[ln]<0)
		fit_flag[ln]=0;
	      if(hy1<0 && fit_valueB[hn]<0)
		fit_flag[hn]=0;

	      if(ly1>=0 || hy0>=0){
		/* store new neighbor values */
		for(j=sortpos-1;j>=0;j--)
		  if(hineighbor[j]==hn)
		  hineighbor[j]=i;
		  else
		    break;
		for(j=sortpos+1;j<posts;j++)
		  if(loneighbor[j]==ln)
		    loneighbor[j]=i;
		  else
		    break;
		
		/* store flag (set) */
		fit_flag[i]=1;
	      }
	    }
	  }
	}
      }
    }

    /* quantize values to multiplier spec */
    switch(info->mult){
    case 1: /* 1024 -> 256 */
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)>>2;
      break;
    case 2: /* 1024 -> 128 */
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)>>3;
      break;
    case 3: /* 1024 -> 86 */
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)/12;
      break;
    case 4: /* 1024 -> 64 */
      for(i=0;i<posts;i++)
	if(fit_flag[i])
	  fit_valueA[i]=post_Y(fit_valueA,fit_valueB,i)>>4;
      break;
    }

    /* find prediction values for each post and subtract them */
    for(i=2;i<posts;i++){
      int sp=look->reverse_index[i];
      int ln=look->loneighbor[i-2];
      int hn=look->hineighbor[i-2];
      int x0=info->postlist[ln];
      int x1=info->postlist[hn];
      int y0=fit_valueA[ln];
      int y1=fit_valueA[hn];
	
      int predicted=render_point(x0,x1,y0,y1,info->postlist[i]);
	
      if(fit_flag[i]){
	int headroom=(look->quant_q-predicted<predicted?
		      look->quant_q-predicted:predicted);
	
	int val=fit_valueA[i]-predicted;
	
	/* at this point the 'deviation' value is in the range +/- max
	   range, but the real, unique range can always be mapped to
	   only [0-maxrange).  So we want to wrap the deviation into
	   this limited range, but do it in the way that least screws
	   an essentially gaussian probability distribution. */
	
	if(val<0)
	  if(val<-headroom)
	    val=headroom-val-1;
	  else
	    val=-1-(val<<1);
	else
	  if(val>=headroom)
	    val= val+headroom;
	  else
	    val<<=1;
	
	fit_valueB[i]=val;
	
	/* unroll the neighbor arrays */
	for(j=sp+1;j<posts;j++)
	  if(loneighbor[j]==i)
	    loneighbor[j]=loneighbor[sp];
	  else
	    break;
	for(j=sp-1;j>=0;j--)
	  if(hineighbor[j]==i)
	    hineighbor[j]=hineighbor[sp];
	  else
	    break;
	
      }else{
	fit_valueA[i]=predicted;
	fit_valueB[i]=0;
      }
      if(fit_valueB[i]==0)
	fit_valueA[i]|=0x8000;
      else{
	fit_valueA[look->loneighbor[i-2]]&=0x7fff;
	fit_valueA[look->hineighbor[i-2]]&=0x7fff;
      }
    }

    /* we have everything we need. pack it out */
    /* mark nontrivial floor */
    oggpack_write(&vb->opb,1,1);

    /* beginning/end post */
    look->postbits+=ilog(look->quant_q-1)*2;
    oggpack_write(&vb->opb,fit_valueA[0],ilog(look->quant_q-1));
    oggpack_write(&vb->opb,fit_valueA[1],ilog(look->quant_q-1));

#ifdef TRAIN_FLOOR1
    {
      FILE *of;
      char buffer[80];
      sprintf(buffer,"line%d_full.vqd",vb->mode);
      of=fopen(buffer,"a");
      for(j=2;j<posts;j++)
	fprintf(of,"%d\n",fit_valueB[j]);
      fclose(of);
    }
#endif

    
    /* partition by partition */
    for(i=0,j=2;i<info->partitions;i++){
      int class=info->partitionclass[i];
      int cdim=info->class_dim[class];
      int csubbits=info->class_subs[class];
      int csub=1<<csubbits;
      int bookas[8]={0,0,0,0,0,0,0,0};
      int cval=0;
      int cshift=0;

      /* generate the partition's first stage cascade value */
      if(csubbits){
	int maxval[8];
	for(k=0;k<csub;k++){
	  int booknum=info->class_subbook[class][k];
	  if(booknum<0){
	    maxval[k]=1;
	  }else{
	    maxval[k]=sbooks[info->class_subbook[class][k]]->entries;
	  }
	}
	for(k=0;k<cdim;k++){
	  for(l=0;l<csub;l++){
	    int val=fit_valueB[j+k];
	    if(val<maxval[l]){
	      bookas[k]=l;
	      break;
	    }
	  }
	  cval|= bookas[k]<<cshift;
	  cshift+=csubbits;
	}
	/* write it */
	look->classbits+=vorbis_book_encode(books+info->class_book[class],cval,&vb->opb);

#ifdef TRAIN_FLOOR1
	{
	  FILE *of;
	  char buffer[80];
	  sprintf(buffer,"line%d_class%d.vqd",vb->mode,class);
	  of=fopen(buffer,"a");
	  fprintf(of,"%d\n",cval);
	  fclose(of);
	}
#endif
      }
      
      /* write post values */
      for(k=0;k<cdim;k++){
	int book=info->class_subbook[class][bookas[k]];
	if(book>=0){
	  look->subbits+=vorbis_book_encode(books+book,
			     fit_valueB[j+k],&vb->opb);

#ifdef TRAIN_FLOOR1
	  {
	    FILE *of;
	    char buffer[80];
	    sprintf(buffer,"line%d_%dsub%d.vqd",vb->mode,class,bookas[k]);
	    of=fopen(buffer,"a");
	    fprintf(of,"%d\n",fit_valueB[j+k]);
	    fclose(of);
	  }
#endif
	}
      }
      j+=cdim;
    }
    
    {
      /* generate quantized floor equivalent to what we'd unpack in decode */
      int hx;
      int lx=0;
      int ly=fit_valueA[0]*info->mult;
      for(j=1;j<posts;j++){
	int current=look->forward_index[j];
	if(!(fit_valueA[current]&0x8000)){
	  int hy=(fit_valueA[current]&0x7fff)*info->mult;
	  hx=info->postlist[current];
	  
	  render_line(lx,hx,ly,hy,codedflr);
	  
	  lx=hx;
	  ly=hy;
	}
      }
      for(j=hx;j<look->n;j++)codedflr[j]=codedflr[j-1]; /* be certain */

      /* use it to create residue vector.  Eliminate residue elements
         that were below the error training attenuation relative to
         the original mask.  This avoids portions of the floor fit
         that were considered 'unused' in fitting from being used in
         coding residue if the unfit values are significantly below
         the original input mask */
      for(j=0;j<n;j++)
	if(logmdct[j]+info->twofitatten<logmask[j])
	  residue[j]=0.f;
	else
	  residue[j]=mdct[j]/codedflr[j];
      for(j=n;j<look->n;j++)residue[j]=0.f;

    }    

  }else{
    oggpack_write(&vb->opb,0,1);
    memset(codedflr,0,n*sizeof(float));
  }
  seq++;
  return(nonzero);
}

static int floor1_inverse(vorbis_block *vb,vorbis_look_floor *in,float *out){
  vorbis_look_floor1 *look=(vorbis_look_floor1 *)in;
  vorbis_info_floor1 *info=look->vi;

  codec_setup_info   *ci=vb->vd->vi->codec_setup;
  int                  n=ci->blocksizes[vb->mode]/2;
  int i,j,k;
  codebook *books=((backend_lookup_state *)(vb->vd->backend_state))->
    fullbooks;   

  /* unpack wrapped/predicted values from stream */
  if(oggpack_read(&vb->opb,1)==1){
    int fit_value[VIF_POSIT+2];

    fit_value[0]=oggpack_read(&vb->opb,ilog(look->quant_q-1));
    fit_value[1]=oggpack_read(&vb->opb,ilog(look->quant_q-1));

    /* partition by partition */
    /* partition by partition */
    for(i=0,j=2;i<info->partitions;i++){
      int class=info->partitionclass[i];
      int cdim=info->class_dim[class];
      int csubbits=info->class_subs[class];
      int csub=1<<csubbits;
      int cval=0;

      /* decode the partition's first stage cascade value */
      if(csubbits){
	cval=vorbis_book_decode(books+info->class_book[class],&vb->opb);

	if(cval==-1)goto eop;
      }

      for(k=0;k<cdim;k++){
	int book=info->class_subbook[class][cval&(csub-1)];
	cval>>=csubbits;
	if(book>=0){
	  if((fit_value[j+k]=vorbis_book_decode(books+book,&vb->opb))==-1)
	    goto eop;
	}else{
	  fit_value[j+k]=0;
	}
      }
      j+=cdim;
    }

    /* unwrap positive values and reconsitute via linear interpolation */
    for(i=2;i<look->posts;i++){
      int predicted=render_point(info->postlist[look->loneighbor[i-2]],
				 info->postlist[look->hineighbor[i-2]],
				 fit_value[look->loneighbor[i-2]],
				 fit_value[look->hineighbor[i-2]],
				 info->postlist[i]);
      int hiroom=look->quant_q-predicted;
      int loroom=predicted;
      int room=(hiroom<loroom?hiroom:loroom)<<1;
      int val=fit_value[i];

      if(val){
	if(val>=room){
	  if(hiroom>loroom){
	    val = val-loroom;
	  }else{
	  val = -1-(val-hiroom);
	  }
	}else{
	  if(val&1){
	    val= -((val+1)>>1);
	  }else{
	    val>>=1;
	  }
	}

	fit_value[i]=val+predicted;
	fit_value[look->loneighbor[i-2]]&=0x7fff;
	fit_value[look->hineighbor[i-2]]&=0x7fff;

      }else{
	fit_value[i]=predicted|0x8000;
      }
	
    }

    /* render the lines */
    {
      int hx;
      int lx=0;
      int ly=fit_value[0]*info->mult;
      for(j=1;j<look->posts;j++){
	int current=look->forward_index[j];
	int hy=fit_value[current]&0x7fff;
	if(hy==fit_value[current]){

	  hy*=info->mult;
	  hx=info->postlist[current];
	  
	  render_line(lx,hx,ly,hy,out);
	  
	  lx=hx;
	  ly=hy;
	}
      }
      for(j=hx;j<n;j++)out[j]=out[j-1]; /* be certain */
    }    
    return(1);
  }

  /* fall through */
 eop:
  memset(out,0,sizeof(float)*n);
  return(0);
}

/* export hooks */
vorbis_func_floor floor1_exportbundle={
  &floor1_pack,&floor1_unpack,&floor1_look,&floor1_copy_info,&floor1_free_info,
  &floor1_free_look,&floor1_forward,&floor1_inverse
};

#if 0
static float test_mask[]={
  -44.5606,
  -52.1648,
  -64.1442,
  -81.206,
  -92.407,
  -92.5,
  -92.5,
  -88.7254,
  -84.746,
  -80.643,
  -67.643,
  -63.643,
  -56.1236,
  -48.643,
  -48.643,
-48.643,
-51.643,
-50.643,
-51.643,
-51.643,
-51.643,
-51.643,
-50.643,
-50.643,
-49.643,
-51.643,
-51.643,
-50.1648,
-49.3369,
-49.3369,
-49.3369,
-49.3369,
-55.1005,
-56.3369,
-56.3369,
-56.3369,
-57.143,
-57.143,
-57.143,
-58.3369,
-56.1236,
-55.1005,
-55.1005,
-55.1005,
-55.1005,
-55.1005,
-60.643,
-62.6005,
-62.6005,
-63.143,
-63.143,
-64.643,
-64.643,
-64.643,
-64.643,
-64.643,
-65.643,
-65.643,
-65.643,
-65.643,
-65.643,
-67.143,
-67.143,
-67.143,
-67.143,
-67.143,
-69.5167,
-69.5167,
-69.1624,
-69.1624,
-69.1624,
-69.1624,
-69.1624,
-69.1624,
-69.1624,
-69.1624,
-69.1624,
-69.3452,
-71,
-71,
-71,
-69.9889,
-68.1854,
-68.1854,
-68.1854,
-68.1854,
-68,
-67.5,
-66.5,
-66.5,
-65,
-64,
-64,
-65,
-64,
-64,
-64,
-64,
-64,
-64,
-64,
-64,
-64,
-63,
-62,
-63,
-63,
-63,
-63,
-63,
-63,
-63,
-61.5,
-61.5,
-61.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-59.5,
-59.5,
-59,
-59,
-59,
-59,
-59,
-59,
-59,
-59,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-58,
-59,
-59,
-59,
-59.5,
-59.5,
-60.5,
-60.5,
-61.5,
-61.5,
-60.5,
-60.5,
-60.5,
-61.5,
-61.5,
-61.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-60.5,
-61.5,
-61.5,
-61.5,
-61.5,
-61.5,
-62.5,
-62.5,
-62.5,
-62.5,
-62.5,
-63,
-63,
-65,
-65,
-66.5,
-66.5,
-66.5,
-71.5,
-71.5,
-71.5,
-71.5,
-71.5,
-71.5,
-71.5,
-79,
-79,
-79.5,
-79,
-79,
-79,
-79,
-79,
-79.5,
-79.5,
-79.5,
-79.5,
-79.5,
-79.5,
-79.5,
-79.5,
-79.5,
-79,
-79,
-79,
-77.5,
-77.5,
-77.5,
-77.5,
-77.5,
-77.5,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-77,
-76.9913,
-76.9803,
-76.9695,
-76.9586,
-76.9478,
-76.937,
-76.9263,
-76.9156,
-76.905,
-76.8944,
-77.3838,
-77.3733,
-77.3628,
-77.3523,
-77.3419,
-77.3315,
-79.3212,
-79.3109,
-79.3006,
-79.2904,
-79.2802,
-79.27,
-79.2599,
-79.2498,
-79.2397,
-79.2297,
-79.2197,
-79.2098,
-79.1999,
-78.69,
-76.6802,
-76.6703,
-76.6606,
-76.6508,
-76.6411,
-74.6314,
-74.6218,
-74.6122,
-74.6026,
-74.593,
-74.5835,
-74.074,
-74.0645,
-73.5551,
-72.0457,
-71.0363,
-71.027,
-71.0177,
-71.0084,
-70.9992,
-70.9899,
-70.9808,
-70.9716,
-70.9625,
-70.9534,
-70.9443,
-70.9352,
-70.9262,
-70.9172,
-70.9083,
-70.8993,
-70.8904,
-70.8816,
-70.8727,
-70.8639,
-70.8551,
-70.8463,
-70.8376,
-70.8289,
-70.8202,
-70.8115,
-70.8029,
-70.7943,
-70.7857,
-70.7771,
-70.7686,
-70.7601,
-70.7516,
-70.7431,
-70.7347,
-70.7263,
-70.7179,
-70.7095,
-70.7012,
-70.6928,
-74.1846,
-74.1763,
-74.168,
-74.1598,
-74.1516,
-75.1434,
-75.1353,
-75.1272,
-76.1191,
-76.111,
-76.1029,
-76.0949,
-76.0869,
-76.5789,
-76.5709,
-76.563,
-76.555,
-76.5471,
-78.0392,
-78.0314,
-78.0235,
-78.0157,
-78.0079,
-78.0002,
-78,
-78,
-79,
-79,
-79,
-79,
-79,
-79,
-80.5,
-80.5,
-80.5,
-80.5,
-80.5,
-80.5,
-81,
-81,
-81,
-81,
-81,
-81,
-81,
-81,
-83,
-83,
-83,
-83,
-83,
-83,
-83,
-83,
-83,
-83,
-83,
-83,
-85.5,
-85.5,
-85.5,
-84,
-84,
-85.5,
-85.5,
-84,
-85.5,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84.5,
-84.5,
-84.5,
-84.5,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84,
-84.5,
-84.5,
-84.5,
-84.5,
-84.5,
-84.5,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85,
-85.5,
-85.5,
-85.5,
-85.5,
-85.5,
-85.5,
-85.5,
-85.5,
-85.5,
-85.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-86.5,
-87,
-87,
-87,
-87,
-87,
-88.4276,
-88.4276,
-88.4276,
-88.4276,
-88.4276,
-88.4276,
-88.4276,
-88.4276,
-89.4276,
-89.4276,
-89.4276,
-89.4276,
-89.4276,
-89.4276,
-91.5,
-91.5,
-91.5,
-91.5,
-91,
-91,
-90.5,
-90.5,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-89,
-89,
-89,
-89,
-89,
-89,
-89,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-89,
-89,
-89,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-89,
-89,
-89,
-89,
-89,
-89,
-89,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5223,
-90.5223,
-90.5223,
-90.5223,
-90.5223,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90.5,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-90,
-89,
-89,
-89,
-89,
-89,
-89,
-90,
-90,
-90,
-90,
-90,
-90,
-89,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.5429,
-87.4384,
-87.3027,
-87.1669,
-87.0312,
-86.8954,
-86.7597,
-86.6239,
-86.4882,
-86.3524,
-86.2167,
-86.081,
-85.9452,
-85.8095,
-85.6737,
-85.538,
-85.4022,
-85.2665,
-85.1307,
-84.995,
-84.8592,
-84.7235,
-84.5878,
-84.452,
-84.3163,
-84.1805,
-84.0448,
-83.909,
-83.7733,
-83.6375,
-83.5018,
-83.366,
-83.2303,
-83.0945,
-82.9588,
-82.8231,
-82.6873,
-82.5516,
-82.4158,
-82.2801,
-82.1443,
-82.0086,
-81.8728,
-81.7371,
-81.6013,
-81.4656,
-81.3298,
-81.1941,
-81.0584,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3296,
-76.3073,
-76.1715,
-76.0358,
-75.9,
-75.7643,
-75.6285,
-75.4928,
-75.357,
-75.2213,
-75.0855,
-74.9498,
-74.814,
-74.6783,
-74.5425,
-74.4068,
-74.2711,
-74.1353,
-73.9996,
-73.8638,
-73.7281,
-73.5923,
-73.4566,
-73.3208,
-73.1851,
-73.0493,
-72.9136,
-72.7778,
-72.6421,
-72.5064,
-72.3706,
-72.2349,
-72.0991,
-71.9634,
-71.8276,
-71.6919,
-71.5561,
-71.4204,
-71.2846,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-56.3296,
-61.5223,
-61.5223,
-61.5109,
-61.3752,
-61.2394,
-61.1037,
-60.9679,
-60.8322,
-60.6965,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.5606,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-60.0713,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
-58.8284,
  -41.3296};

static float test_mdct[]={
-25.2422,
-47.1418,
-47.1418,
-64.2884,
-72.2472,
-75.5017,
-70.309,
-78.2678,
-92.1125,
-74.0507,
-61.3658,
-55.9889,
-50.6636,
-31.2628,
-27.3369,
-42.7048,
-49.9683,
-77.2448,
-64.2884,
-79.4276,
-76.3296,
-83.2654,
-87.5429,
-86.7872,
-81.5223,
-65.2035,
-60.206,
-58.2678,
-25.8859,
-36.6842,
-52.2472,
-52.2472,
-58.2678,
-74.0507,
-77.2448,
-77.2448,
-68.7254,
-70.309,
-65.2035,
-60.206,
-54.1854,
-43.9477,
-35.1005,
-41.1211,
-48.7254,
-66.2266,
-71.2242,
-62.0095,
-74.0507,
-82.3502,
-79.4276,
-84.2884,
-86.7872,
-88.3708,
-77.2448,
-66.2266,
-50.6636,
-71.2242,
-78.2678,
-74.0507,
-77.2448,
-89.2859,
-70.309,
-96.8902,
-94.3914,
-80.7666,
-90.8696,
-92.1125,
-75.5017,
-66.2266,
-54.746,
-100.412,
-60.206,
-60.206,
-68.7254,
-80.7666,
-89.2859,
-70.309,
-74.746,
-75.5017,
-86.7872,
-74.0507,
-72.2472,
-77.2448,
-64.2884,
-56.6842,
-54.1854,
-56.6842,
-55.9889,
-61.3658,
-67.3864,
-66.2266,
-78.2678,
-76.3296,
-73.407,
-78.2678,
-70.309,
-63.4605,
-60.206,
-98.8284,
-59.1829,
-60.206,
-63.4605,
-72.8078,
-77.2448,
-88.3708,
-83.2654,
-80.7666,
-78.8284,
-106.433,
-71.2242,
-66.7872,
-62.7048,
-72.2472,
-60.7666,
-59.1829,
-63.4605,
-67.3864,
-77.2448,
-72.2472,
-84.2884,
-80.7666,
-90.309,
-74.746,
-84.2884,
-71.2242,
-60.7666,
-64.2884,
-57.4399,
-59.1829,
-63.4605,
-62.7048,
-60.7666,
-57.4399,
-62.0095,
-60.206,
-80.0713,
-67.3864,
-56.6842,
-58.2678,
-60.206,
-58.2678,
-55.9889,
-53.1623,
-54.746,
-57.4399,
-61.3658,
-62.7048,
-63.4605,
-66.2266,
-74.746,
-75.5017,
-88.3708,
-71.2242,
-63.4605,
-68.0301,
-61.3658,
-118.474,
-66.7872,
-64.2884,
-66.2266,
-70.309,
-76.3296,
-70.309,
-83.2654,
-69.4811,
-68.0301,
-70.309,
-74.0507,
-63.4605,
-82.3502,
-59.1829,
-59.1829,
-64.2884,
-64.2884,
-73.407,
-71.2242,
-78.2678,
-72.2472,
-68.7254,
-68.0301,
-60.206,
-60.7666,
-74.746,
-60.206,
-84.2884,
-62.7048,
-60.206,
-62.0095,
-66.2266,
-75.5017,
-74.0507,
-76.3296,
-66.2266,
-81.5223,
-64.2884,
-66.2266,
-71.2242,
-71.2242,
-86.7872,
-89.2859,
-79.4276,
-96.3296,
-78.2678,
-71.2242,
-78.2678,
-86.0919,
-95.3065,
-79.4276,
-86.7872,
-78.8284,
-86.7872,
-82.3502,
-125.41,
-87.5429,
-91.4688,
-93.5635,
-89.2859,
-107.348,
-85.4482,
-102.35,
-99.5841,
-100.412,
-92.8078,
-102.911,
-93.5635,
-89.2859,
-84.849,
-86.7872,
-108.371,
-86.7872,
-79.4276,
-86.7872,
-92.1125,
-106.433,
-86.0919,
-83.2654,
-83.2654,
-80.7666,
-78.2678,
-102.911,
-78.8284,
-105.605,
-77.2448,
-84.849,
-84.849,
-72.8078,
-83.2654,
-80.7666,
-91.4688,
-79.4276,
-80.7666,
-96.3296,
-74.0507,
-81.5223,
-79.4276,
-76.3296,
-96.8902,
-94.3914,
-75.5017,
-77.2448,
-83.2654,
-86.0919,
-84.2884,
-94.3914,
-83.2654,
-101.327,
-75.5017,
-95.3065,
-78.8284,
-86.7872,
-84.2884,
-83.2654,
-78.8284,
-85.4482,
-89.2859,
-84.2884,
-88.3708,
-86.0919,
-99.5841,
-98.1331,
-83.2654,
-90.8696,
-119.389,
-89.2859,
-92.8078,
-84.849,
-90.309,
-81.5223,
-92.1125,
-108.371,
-92.1125,
-87.5429,
-84.2884,
-86.7872,
-92.1125,
-76.3296,
-79.4276,
-96.8902,
-89.2859,
-78.8284,
-82.3502,
-80.7666,
-78.8284,
-86.0919,
-82.3502,
-80.0713,
-80.7666,
-76.3296,
-82.3502,
-80.0713,
-73.407,
-78.2678,
-74.746,
-76.3296,
-87.5429,
-74.746,
-77.2448,
-72.2472,
-91.4688,
-75.5017,
-71.2242,
-80.0713,
-96.3296,
-74.0507,
-82.3502,
-71.2242,
-70.309,
-70.309,
-78.2678,
-76.3296,
-81.5223,
-70.309,
-69.4811,
-80.7666,
-75.5017,
-74.746,
-80.7666,
-82.3502,
-77.2448,
-83.2654,
-76.3296,
-84.2884,
-80.7666,
-78.8284,
-80.7666,
-70.309,
-78.8284,
-81.5223,
-81.5223,
-86.7872,
-86.7872,
-77.2448,
-88.3708,
-75.5017,
-102.35,
-81.5223,
-82.3502,
-80.7666,
-88.3708,
-78.2678,
-88.3708,
-84.2884,
-89.2859,
-96.3296,
-101.327,
-78.2678,
-88.3708,
-74.746,
-104.154,
-84.2884,
-94.3914,
-88.3708,
-102.35,
-79.4276,
-88.3708,
-87.5429,
-92.1125,
-83.2654,
-106.433,
-91.4688,
-81.5223,
-102.911,
-89.2859,
-80.7666,
-92.1125,
-83.2654,
-91.4688,
-95.3065,
-87.5429,
-400,
-83.2654,
-90.309,
-92.1125,
-81.5223,
-89.2859,
-109.531,
-90.8696,
-89.2859,
-91.4688,
-87.5429,
-88.3708,
-91.4688,
-106.433,
-92.8078,
-90.8696,
-92.1125,
-92.1125,
-90.309,
-98.1331,
-112.453,
-86.0919,
-97.4894,
-92.1125,
-98.1331,
-83.2654,
-118.474,
-91.4688,
-109.531,
-88.3708,
-104.849,
-91.4688,
-94.3914,
-109.531,
-90.309,
-113.368,
-92.1125,
-87.5429,
-102.35,
-84.2884,
-86.7872,
-90.8696,
-87.5429,
-84.2884,
-86.7872,
-84.2884,
-400,
-89.2859,
-119.389,
-88.3708,
-81.5223,
-108.931,
-80.7666,
-104.154,
-115.551,
-89.2859,
-111.625,
-93.5635,
-104.154,
-89.2859,
-92.8078,
-98.1331,
-102.35,
-85.4482,
-86.0919,
-90.8696,
-83.2654,
-96.8902,
-89.2859,
-89.2859,
-100.412,
-84.2884,
-88.3708,
-119.389,
-92.8078,
-84.849,
-96.8902,
-98.1331,
-91.4688,
-88.3708,
-105.605,
-89.2859,
-99.5841,
-126.993,
-90.309,
-95.3065,
-130.515,
-106.433,
-123.667,
-87.5429,
-85.4482,
-107.348,
-85.4482,
-91.4688,
-86.7872,
-95.3065,
-90.8696,
-102.35,
-88.3708,
-81.5223,
-86.0919,
-86.0919,
-94.3914,
-96.3296,
-87.5429,
-106.433,
-94.3914,
-120.412,
-96.3296,
-95.3065,
-91.4688,
-108.371,
-98.8284,
-95.3065,
-108.931,
-90.8696,
-99.5841,
-102.35,
-110.174,
-114.391,
-98.8284,
-107.348,
-94.3914,
-108.931,
-94.3914,
-93.5635,
-103.51,
-96.3296,
-96.3296,
-98.8284,
-95.3065,
-116.89,
-95.3065,
-102.911,
-96.3296,
-104.154,
-106.433,
-98.8284,
-100.412,
-97.4894,
-92.8078,
-92.8078,
-95.3065,
-97.4894,
-102.35,
-110.174,
-92.8078,
-109.531,
-92.1125,
-94.3914,
-91.4688,
-97.4894,
-94.3914,
-92.1125,
-87.5429,
-89.2859,
-114.391,
-90.309,
-103.51,
-90.8696,
-94.3914,
-90.309,
-90.309,
-98.8284,
-100.412,
-95.3065,
-112.453,
-92.1125,
-107.348,
-90.8696,
-95.3065,
-104.154,
-103.51,
-90.309,
-90.8696,
-101.327,
-88.3708,
-94.3914,
-98.8284,
-103.51,
-96.3296,
-90.309,
-90.8696,
-89.2859,
-85.4482,
-101.327,
-98.8284,
-96.3296,
-95.3065,
-134.952,
-83.2654,
-99.5841,
-90.309,
-103.51,
-102.35,
-105.605,
-112.453,
-94.3914,
-98.1331,
-92.1125,
-98.8284,
-94.3914,
-92.8078,
-112.453,
-102.911,
-86.0919,
-90.309,
-118.474,
-94.3914,
-104.154,
-92.1125,
-134.257,
-95.3065,
-89.2859,
-93.5635,
-101.327,
-102.35,
-113.368,
-93.5635,
-101.327,
-93.5635,
-94.3914,
-99.5841,
-110.174,
-116.195,
-91.4688,
-87.5429,
-93.5635,
-92.1125,
-87.5429,
-107.348,
-91.4688,
-87.5429,
-92.1125,
-90.8696,
-98.8284,
-104.849,
-90.309,
-105.605,
-120.412,
-105.605,
-100.412,
-90.309,
-99.5841,
-92.1125,
-89.2859,
-98.1331,
-94.3914,
-102.35,
-96.3296,
-96.3296,
-110.174,
-91.4688,
-101.327,
-108.931,
-96.8902,
-93.5635,
-97.4894,
-90.8696,
-85.4482,
-108.371,
-90.309,
-99.5841,
-92.1125,
-94.3914,
-93.5635,
-400,
-96.8902,
-100.412,
-97.4894,
-92.1125,
-86.0919,
-102.35,
-103.51,
-101.327,
-97.4894,
-96.8902,
-102.35,
-92.8078,
-99.5841,
-120.412,
-92.8078,
-96.3296,
-107.348,
-101.327,
-98.8284,
-102.35,
-96.3296,
-106.433,
-91.4688,
-91.4688,
-90.8696,
-92.1125,
-99.5841,
-105.605,
-90.8696,
-114.952,
-102.35,
-92.8078,
-98.1331,
-99.5841,
-96.3296,
-96.3296,
-98.8284,
-107.348,
-101.327,
-86.7872,
-86.7872,
-90.8696,
-98.8284,
-102.35,
-107.348,
-113.368,
-95.3065,
-100.412,
-96.3296,
-95.3065,
-90.309,
-95.3065,
-94.3914,
-110.87,
-93.5635,
-90.309,
-110.87,
-97.4894,
-120.412,
-101.327,
-95.3065,
-93.5635,
-89.2859,
-92.1125,
-95.3065,
-87.5429,
-89.2859,
-90.8696,
-98.1331,
-99.5841,
-109.531,
-102.35,
-102.35,
-104.849,
-96.3296,
-107.348,
-100.412,
-91.4688,
-109.531,
-87.5429,
-93.5635,
-104.849,
-95.3065,
-118.474,
-119.389,
-96.3296,
-92.1125,
-90.309,
-89.2859,
-100.412,
-92.1125,
-123.667,
-88.3708,
-94.3914,
-107.348,
-122.216,
-107.348,
-108.371,
-124.494,
-96.3296,
-107.348,
-115.551,
-96.3296,
-97.4894,
-90.8696,
-98.8284,
-96.8902,
-89.2859,
-104.154,
-89.2859,
-85.4482,
-102.35,
-96.3296,
-112.453,
-95.3065,
-106.433,
-92.1125,
-108.931,
-86.7872,
-139.034,
-95.3065,
-108.371,
-98.8284,
-86.7872,
-92.1125,
-105.605,
-90.309,
-100.412,
-93.5635,
-108.371,
-108.931,
-105.605,
-96.8902,
-91.4688,
-94.3914,
-90.309,
-96.8902,
-101.327,
-90.309,
-100.412,
-103.51,
-128.236,
-100.412,
-90.309,
-92.8078,
-95.3065,
-102.35,
-93.5635,
-84.2884,
-96.8902,
-103.51,
-98.8284,
-90.309,
-86.7872,
-93.5635,
-93.5635,
-103.51,
-98.1331,
-91.4688,
-90.8696,
-98.8284,
-94.3914,
-100.412,
-96.8902,
-85.4482,
-100.412,
-100.412,
-96.3296,
-82.3502,
-83.2654,
-79.4276,
-86.7872,
-92.8078,
-85.4482,
-100.412,
-91.4688,
-125.41,
-90.309,
-87.5429,
-82.3502,
-117.646,
-92.1125,
-83.2654,
-104.849,
-94.3914,
-97.4894,
-82.3502,
-97.4894,
-90.309,
-86.7872,
-114.952,
-97.4894,
-120.412,
-115.551,
-97.4894,
-92.1125,
-92.8078,
-113.368,
-109.531,
-102.35,
-107.348,
-86.7872,
-86.7872,
-92.8078,
-94.3914,
-98.8284,
-92.1125,
-90.8696,
-111.625,
-92.8078,
-100.412,
-102.35,
-89.2859,
-93.5635,
-96.3296,
-86.0919,
-96.8902,
-111.625,
-107.348,
-131.43,
-94.3914,
-100.412,
-92.1125,
-91.4688,
-93.5635,
-120.412,
-95.3065,
-88.3708,
-102.911,
-90.309,
-94.3914,
-90.309,
-89.2859,
-90.309,
-91.4688,
-86.7872,
-108.931,
-98.1331,
-107.348,
-92.1125,
-90.8696,
-87.5429,
-100.412,
-96.8902,
-101.327,
-95.3065,
-86.7872,
-92.1125,
-98.1331,
-103.51,
-98.8284,
-95.3065,
-94.3914,
-102.35,
-96.3296,
-111.625,
-91.4688,
-103.51,
-107.348,
-98.1331,
-90.309,
-105.605,
-95.3065,
-92.1125,
-90.309,
-114.952,
-102.35,
-101.327,
-97.4894,
-104.849,
-86.7872,
-88.3708,
-89.2859,
-90.309,
-92.1125,
-99.5841,
-98.8284,
-91.4688,
-100.412,
-102.35,
-94.3914,
-96.8902,
-94.3914,
-102.35,
-93.5635,
-97.4894,
-98.8284,
-96.8902,
-89.2859,
-96.8902,
-99.5841,
-99.5841,
-95.3065,
-97.4894,
-105.605,
-112.453,
-89.2859,
-92.1125,
-104.849,
-91.4688,
-104.849,
-102.911,
-101.327,
-99.5841,
-96.3296,
-96.3296,
-100.412,
-94.3914,
-107.348,
-100.412,
-102.35,
-105.605,
-98.8284,
-95.3065,
-104.849,
-96.3296,
-94.3914,
-114.952,
-96.3296,
-103.51,
-101.327,
-96.3296,
-97.4894,
-95.3065,
-93.5635,
-102.911,
-125.41,
-110.174,
-104.154,
-100.412,
-108.371,
-91.4688,
-106.433,
-129.687,
-121.572,
-121.572,
-98.8284,
-101.327,
-99.5841,
-101.327,
-95.3065,
-97.4894,
-105.605,
-104.154,
-97.4894,
-92.8078,
-102.35,
-101.327,
-106.433,
-99.5841,
-94.3914,
-99.5841,
-94.3914,
-104.849,
-92.1125,
-100.412,
-116.89,
-123.667,
-132.453,
-97.4894,
-99.5841,
-94.3914,
-105.605,
-94.3914,
-98.1331,
-103.51,
-109.531};

#include "../include/vorbis/vorbisenc.h"
typedef struct {
  drft_lookup fft_look;
  vorbis_info_mode *mode;
  vorbis_info_mapping0 *map;

  vorbis_look_time **time_look;
  vorbis_look_floor **floor_look;

  vorbis_look_residue **residue_look;
  vorbis_look_psy *psy_look;

  vorbis_func_time **time_func;
  vorbis_func_floor **floor_func;
  vorbis_func_residue **residue_func;

  int ch;
  long lastframe; /* if a different mode is called, we need to 
                     invalidate decay */
} vorbis_look_mapping0;

int main(){
  vorbis_info      vi; /* struct that stores all the static vorbis bitstream
                          settings */
  vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
  vorbis_block     vb; /* local working space for packet->PCM decode */
  backend_lookup_state *b;
  vorbis_look_mapping0  *m;
  float out[1024];

  /* choose an encoding mode */
  /* (mode 0: 44kHz stereo uncoupled, roughly 128kbps VBR) */
  vorbis_info_init(&vi);
  vorbis_encode_init(&vi,2,44100, -1, 128000, -1);

  /* set up the analysis state and auxiliary encoding storage */
  vorbis_analysis_init(&vd,&vi);
  vorbis_block_init(&vd,&vb);
  b=vd.backend_state;
  m=(vorbis_look_mapping0 *)(b->mode[1]);

  floor1_forward(&vb,m->floor_look[0],
		 test_mdct, test_mdct,
		 test_mask, test_mdct,
		 test_mdct, out);
    
  _analysis_output("testout",0,out,1024,0,1);
}

#endif
