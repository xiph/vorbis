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
 last modification date: Oct 7 1999

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
#include "bitwise.h"
#include "spectrum.h"

/* this code is still seriously abbreviated.  I'm filling in pieces as
   we go... --Monty 19991004 */

int vorbis_analysis(vorbis_block *vb,ogg_packet *op){
  static int frameno=0;
  
  int i;
  double           *window=vb->vd->window[vb->W][vb->lW][vb->nW];
  lpc_lookup       *vl=&vb->vd->vl[vb->W];
  vorbis_dsp_state *vd=vb->vd;
  vorbis_info      *vi=vd->vi;
  oggpack_buffer   *opb=&vb->opb;

  int              n=vb->pcmend;
  int              spectral_order=vi->floororder[vb->W];

  /*lpc_lookup       *vbal=&vb->vd->vbal[vb->W];
    double balance_v[vbal->m];
    double balance_amp;*/

  /* first things first.  Make sure encode is ready*/
  _oggpack_reset(opb);
  /* Encode the packet type */
  _oggpack_write(opb,0,1);
  /* Encode the block size */
  _oggpack_write(opb,vb->W,1);

  /* we have the preecho metrics; decide what to do with them */
  /*_ve_envelope_sparsify(vb);
    _ve_envelope_apply(vb,0);*/

  /* Encode the envelope */
  /*if(_ve_envelope_encode(vb))return(-1);*/
  
  /* time domain PCM -> MDCT domain */
  for(i=0;i<vi->channels;i++)
    mdct_forward(&vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);

  /* no balance yet */
    
  /* extract the spectral envelope and residue */
  /* just do by channel.  No coupling yet */
  {
    for(i=0;i<vi->channels;i++){
      double floor[n/2];
      double curve[n/2];
      double *lpc=vb->lpc[i];
      double *lsp=vb->lsp[i];

      memset(floor,0,sizeof(double)*n/2);
      
      _vp_noise_floor(vb->pcm[i],floor,n/2);
      _vp_mask_floor(vb->pcm[i],floor,n/2);

      /* Convert our floor to a set of lpc coefficients */
      vb->amp[i]=sqrt(vorbis_curve_to_lpc(floor,lpc,vl));

      /* LSP <-> LPC is orthogonal and LSP quantizes more stably */
      vorbis_lpc_to_lsp(lpc,lsp,vl->m);

      /* code the spectral envelope; mutates the lsp coeffs to reflect
         what was actually encoded */
      _vs_spectrum_encode(vb,vb->amp[i],lsp);

      /* Generate residue from the decoded envelope, which will be
         slightly different to the pre-encoding floor due to
         quantization.  Slow, yes, but perhaps more accurate */

      vorbis_lsp_to_lpc(lsp,lpc,vl->m); 
      vorbis_lpc_to_curve(curve,lpc,vb->amp[i],vl);
      
      /* this may do various interesting massaging too...*/
      _vs_residue_quantize(vb->pcm[i],curve,vi,n/2);

      /* encode the residue */
      _vs_residue_encode(vb,vb->pcm[i]);

    }
  }

  /* set up the packet wrapper */

  op->packet=opb->buffer;
  op->bytes=_oggpack_bytes(opb);
  op->b_o_s=0;
  op->e_o_s=vb->eofflag;
  op->frameno=vb->frameno;

  return(0);
}




/* commented out, relocated balance stuff */
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
