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
 last modification date: Oct 4 1999

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

/* this code is still seriously abbreviated.  I'm filling in pieces as
   we go... --Monty 19991004 */

int vorbis_analysis(vorbis_block *vb,ogg_packet *op){
  int i;
  double           *window=vb->vd->window[vb->W][vb->lW][vb->nW];
  lpc_lookup       *vl=&vb->vd->vl[vb->W];
  vorbis_dsp_state *vd=vi->vd;
  vorbis_info      *vi=vd->vi;
  int              n=vb->pcmend;

  /*lpc_lookup       *vbal=&vb->vd->vbal[vb->W];
    double balance_v[vbal->m];
    double balance_amp;*/

  /* we have the preecho metrics; decie what to do with them */
  _ve_envelope_sparsify(vb);
  _ve_envelope_apply(vb,0);
  
  for(i=0;i<vi->channels;i++)
    mdct_forward(&vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);


  /* no balance or channel coupling yet */
  /*{
    double *C=vb->pcm[0];
    double *D=vb->pcm[1];
    
    balance_amp=_vp_balance_compute(D,C,balance_v,vbal);
    
    {
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
      
    }
    
    _vp_balance_apply(D,C,balance_v,balance_amp,vbal,1);
      
    {
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
      
    }
  }*/
    
  /* extract the spectral envelope and residue */
  /* just do by channel.  No coupling yet */
  {
    for(i=0;i<vi->channels;vi++){
      double floor[n/2];
      double curve[n/2];
      double *lpc=vb->lpc[i];
      double *lsp=vb->lsp[i];

      memset(floor,0,sizeof(double)*n/2);
      
      _vp_noise_floor(vb->pcm[i],floor,n/2);
      _vp_mask_floor(vb->pcm[i],floor,n/2);
      vb->amp[i]=sqrt(vorbis_curve_to_lpc(floor,lpc,vl));

      vorbis_lpc_to_lsp(lpc,lsp,vl->m);
      
      {
	/* make this scale configurable */
	int scale=1020;
	int last=0;
	for(i=0;i<vl->m;i++){
	  double q=lsp[i]/M_PI*scale;
	  int val=rint(q-last);
	  
	  last+=val;
	  lsp[i]=val;
	  
	}
      }
      
      /* make residue.  Get the floor curve back from LPC (do we want
         to recover all the way from LSP in the future?  Yes, once the
         residue massaging is smarter) */

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

