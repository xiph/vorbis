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
 last modification date: Jul 28 1999

 ********************************************************************/

analysis_packetout(vorbis_dsp_state *v, vorbis_block *vb,
			      ogg_packet *op){

  /* find block's envelope vector and apply it */


  /* the real analysis begins; forward MDCT with window */

  
  /* Noise floor, resolution floor */

  /* encode the floor into LSP; get the actual floor back for quant */

  /* use noise floor, res floor for culling, actual floor for quant */

  /* encode residue */

}
