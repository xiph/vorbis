analysis_packetout(vorbis_dsp_state *v, vorbis_block *vb,
			      ogg_packet *op){

  /* find block's envelope vector and apply it */


  /* the real analysis begins; forward MDCT with window */

  
  /* Noise floor, resolution floor */

  /* encode the floor into LSP; get the actual floor back for quant */

  /* use noise floor, res floor for culling, actual floor for quant */

  /* encode residue */

}
