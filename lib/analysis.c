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

 function: single-block PCM analysis
 author: Monty <xiphmont@mit.edu>
 modifications by: Monty
 last modification date: Oct 2 1999

 ********************************************************************/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "lpc.h"
#include "lsp.h"
#include "codec.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"

extern void compute_balance(double *A, double *B, double *phi,int n);

int vorbis_analysis(vorbis_block *vb,ogg_packet *op){
  int i;
  double *window=vb->vd->window[vb->W][vb->lW][vb->nW];
  lpc_lookup *vl=&vb->vd->vl[vb->W];
  lpc_lookup *vbal=&vb->vd->vbal[vb->W];
  int n=vb->pcmend;
  static int frameno=0;

  double balance_v[vbal->m];
  double balance_amp;

  _ve_envelope_sparsify(vb);
  _ve_envelope_apply(vb,0);
  
  for(i=0;i<vb->pcm_channels;i++)
    mdct_forward(&vb->vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);


  /* hardwired to stereo for now */
  {
    double *C=vb->pcm[0];
    double *D=vb->pcm[1];
    
    /* Balance */
    
    balance_amp=_vp_balance_compute(D,C,balance_v,vbal);
    
    /*{
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"com%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out," 0. 0.\n");
	fprintf(out,"%g %g\n",C[i],D[i]);
	fprintf(out,"\n");
      }
      fclose(out);
      
      sprintf(buffer,"L%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",C[i]);
      }
      fclose(out);
      sprintf(buffer,"R%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",D[i]);
      }
      fclose(out);
      
      }*/
    
    /* apply balance vectors, mix down to the vectors we actually encode */
    
    _vp_balance_apply(D,C,balance_v,balance_amp,vbal,1);
    
    /*{
      FILE *out;
      char buffer[80];
      
      sprintf(buffer,"bal%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out," 0. 0.\n");
	fprintf(out,"%g %g\n",C[i],D[i]);
	fprintf(out,"\n");
      }
      fclose(out);
      sprintf(buffer,"C%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",C[i]);
      }
      fclose(out);
      sprintf(buffer,"D%d.m",frameno);
      out=fopen(buffer,"w+");
      for(i=0;i<n/2;i++){
	fprintf(out,"%g\n",D[i]);
      }
      fclose(out);
      
      }*/
    
    /* extract the spectral envelope and residue */
    
    {
      double floor1[n/2];
      double curve[n/2];
      double work[n/2];
      double lpc1[80];
      double lsp1[80];
      double lsp2[80];
      double amp1;
      
      memset(floor1,0,sizeof(floor1));
      memset(work,0,sizeof(work));
      
      _vp_noise_floor(C,floor1,n/2);
      _vp_noise_floor(D,floor1,n/2);
      _vp_mask_floor(C,floor1,n/2);
      _vp_mask_floor(D,floor1,n/2);
    
      memcpy(curve,floor1,sizeof(double)*n/2);
      amp1=sqrt(vorbis_curve_to_lpc(curve,lpc1,vl));
    
      /*vorbis_lpc_to_lsp(lpc1,lsp1,30);
      
      {
      int scale=1020;
      int last=0;
      for(i=0;i<30;i++){
      double q=lsp1[i]/M_PI*scale;
      int val=rint(q-last);
      
      last+=val;
      lsp1[i]=val;
      
      }
      }
      
      
      {
      int scale=1020;
	double last=0;
	for(i=0;i<30;i++){
	  last+=lsp1[i];
	  lsp2[i]=last*M_PI/scale;
	}
      }



      vorbis_lsp_to_lpc(lsp2,lpc1,30);*/

      vorbis_lpc_to_curve(work,lpc1,amp1,vl);
      
      _vp_psy_quantize(C,work,n/2);
      _vp_psy_quantize(D,work,n/2);
      
      _vp_psy_unquantize(C,work,n/2);
      _vp_psy_unquantize(D,work,n/2);
      
      
      {
	FILE *out;
	char buffer[80];
	
	
	/*sprintf(buffer,"qC%d.m",frameno);
	  out=fopen(buffer,"w+");
	  for(i=0;i<n/2;i++)
	  fprintf(out,"%g\n",fabs(C[i]));
	  fclose(out);
	  
	  sprintf(buffer,"qD%d.m",frameno);
	  out=fopen(buffer,"w+");
	  for(i=0;i<n/2;i++)
	  fprintf(out,"%g\n",fabs(D[i]));
	  fclose(out);
	  
	  sprintf(buffer,"floor%d.m",frameno);
	  out=fopen(buffer,"w+");
	  for(i=0;i<n/2;i++)
	  fprintf(out,"%g\n",floor1[i]);
	  fclose(out);
	  
	  sprintf(buffer,"lpc%d.m",frameno);
	  out=fopen(buffer,"w+");
	  for(i=0;i<n/2;i++)
	  fprintf(out,"%g\n",work[i]);
	  fclose(out);
	  
	  sprintf(buffer,"curve%d.m",frameno);
	  out=fopen(buffer,"w+");
	  for(i=0;i<n/2;i++)
	  fprintf(out,"%g\n",curve[i]);
	  fclose(out);
	  
	  sprintf(buffer,"lsp%d.m",frameno);
	  out=fopen(buffer,"w+");
	  for(i=0;i<30;i++){
	  fprintf(out,"%g 0.\n",lsp1[i]);
	  fprintf(out,"%g .1\n",lsp1[i]);
	  fprintf(out,"\n");
	  fprintf(out,"\n");
	  }
	  fclose(out);*/
	
	frameno++;
      }
    }
    /* unmix */
    _vp_balance_apply(D,C,balance_v,balance_amp,vbal,0);
  }

  return(0);
}

