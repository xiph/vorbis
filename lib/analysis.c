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
 last modification date: Aug 09 1999

 ********************************************************************/

#include <stdio.h>
#include <math.h>
#include "lpc.h"
#include "codec.h"
#include "envelope.h"
#include "mdct.h"
#include "psy.h"

int vorbis_analysis(vorbis_block *vb){
  int i;
  double *window=vb->vd->window[vb->W][vb->lW][vb->nW];
  int n=vb->pcmend;
  static int frameno=0;

  _ve_envelope_sparsify(vb);
  _ve_envelope_apply(vb,0);
  
  for(i=0;i<vb->pcm_channels;i++)
    mdct_forward(&vb->vd->vm[vb->W],vb->pcm[i],vb->pcm[i],window);

  /* handle vectors to be encoded one at a time. The PCM storage now
     has twice the space we need in the MDCT domain, so mix down into
     the second half of storage */

  /* XXXX Mix down to C+D stereo now */
  {
    double *C=vb->pcm[0]+n/2;
    double *D=vb->pcm[1]+n/2;
    double *L=vb->pcm[0];
    double *R=vb->pcm[1];
    for(i=0;i<n/2;i++){
      C[i]=(L[i]+R[i])*.5;
      D[i]=(L[i]-R[i])*.5;
    }

    /* extract the spectral envelope and residue */
    
    {
      double floor1[n/2];
      double floor2[n/2];
      double curve[n/2];
      double work[n/2];
      double lpc1[40];
      double lpc2[40];
      double amp1,amp2;

      memset(floor1,0,sizeof(floor1));
      memset(floor2,0,sizeof(floor2));
      memset(work,0,sizeof(work));
      
      _vp_noise_floor(C,floor1,n/2);
      /*      _vp_noise_floor(D,floor1,n/2);*/
      _vp_mask_floor(C,floor1,n/2);
      /*      _vp_mask_floor(D,floor1,n/2);*/

      _vp_psy_sparsify(C,floor1,n/2);
      _vp_psy_sparsify(D,floor1,n/2);


      memmove(floor1+n/4,floor1,(n/4)*sizeof(double));

      amp2=sqrt(vorbis_curve_to_lpc(floor1,n/2,lpc2,20));
      vorbis_lpc_to_curve(work,n/2,lpc2,amp2,20);


      /*      amp2=sqrt(vorbis_curve_to_lpc(floor1,n/2/8,lpc2,12));
      vorbis_lpc_to_curve(work,n/2/8,lpc2,amp2,12);
      amp2=sqrt(vorbis_curve_to_lpc(floor1+n/2/8,n/2/8,lpc2,10));
      vorbis_lpc_to_curve(work+n/2/8,n/2/8,lpc2,amp2,10);
      amp2=sqrt(vorbis_curve_to_lpc(floor1+n/2/4,n/2/4,lpc2,6));
      vorbis_lpc_to_curve(work+n/2/4,n/2/4,lpc2,amp2,6);
      amp2=sqrt(vorbis_curve_to_lpc(floor1+n/2/2,n/2/2,lpc2,6));
      vorbis_lpc_to_curve(work+n/2/2,n/2/2,lpc2,amp2,6);*/

      _vp_psy_quantize(C,work,n/2);
      _vp_psy_quantize(D,work,n/2);

      
      {
	FILE *out;
	char buffer[80];

	
	sprintf(buffer,"mdctC%d.m",frameno);
	out=fopen(buffer,"w+");
	for(i=0;i<n/2;i++)
	  fprintf(out,"%g\n",fabs(C[i]));
	fclose(out);
	
	sprintf(buffer,"mdctD%d.m",frameno);
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

	frameno++;
	}
    }
  }
  return(0);
}

int vorbis_analysis_packetout(vorbis_block *vb, ogg_packet *op){

  /* find block's envelope vector and apply it */


  /* the real analysis begins; forward MDCT with window */

  
  /* Noise floor, resolution floor */

  /* encode the floor into LSP; get the actual floor back for quant */

  /* use noise floor, res floor for culling, actual floor for quant */

  /* encode residue */

}
