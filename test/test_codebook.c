/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis SOFTWARE CODEC SOURCE CODE.   *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis SOURCE CODE IS (C) COPYRIGHT 1994-2007             *
 * by the Xiph.Org Foundation https://xiph.org/                     *
 *                                                                  *
 ********************************************************************

 function: exercise vorbis_book_init_decode() with codebooks that
           claim very large numbers of used entries.

 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>
#include "vorbis/codec.h"

static int ilog(unsigned int v) {
  int r = 0;
  while (v) {
    r++;
    v >>= 1;
  }
  return r;
}

static void build_id_header(oggpack_buffer *opb) {
  int i;
  const char *magic = "vorbis";
  oggpack_writeinit(opb);
  oggpack_write(opb, 0x01, 8);
  for (i = 0; i < 6; i++)
    oggpack_write(opb, magic[i], 8);
  oggpack_write(opb, 0, 32);      /* version */
  oggpack_write(opb, 1, 8);       /* channels */
  oggpack_write(opb, 48000, 32);  /* sample rate */
  oggpack_write(opb, 0, 32);      /* bitrate_upper */
  oggpack_write(opb, 0, 32);      /* bitrate_nominal */
  oggpack_write(opb, 0, 32);      /* bitrate_lower */
  oggpack_write(opb, 6, 4);       /* blocksize_0 = 2^6 = 64 */
  oggpack_write(opb, 6, 4);       /* blocksize_1 = 2^6 = 64 */
  oggpack_write(opb, 1, 1);       /* framing bit */
}

static void build_comment_header(oggpack_buffer *opb) {
  int i;
  const char *magic = "vorbis";
  oggpack_writeinit(opb);
  oggpack_write(opb, 0x03, 8);
  for (i = 0; i < 6; i++)
    oggpack_write(opb, magic[i], 8);
  oggpack_write(opb, 0, 32);      /* vendor string length */
  oggpack_write(opb, 0, 32);      /* user comment list length */
  oggpack_write(opb, 1, 1);       /* framing bit */
}

/*
 * Build a setup header containing two codebooks:
 *   Codebook 0: a minimal valid codebook (dim=1, entries=2, unordered)
 *   Codebook 1: codebook with given dim and entries, using ordered length
 *               encoding to mark all entries as used in a few bytes of
 *               bitstream. maptype=0 so dim has no VQ effect.
 *
 * The rest of the setup header (times, floors, residues, mappings, modes)
 * uses minimal valid configurations.
 */
static void build_setup_header(oggpack_buffer *opb, int dim, long entries) {
  int i;
  const char *magic = "vorbis";

  oggpack_writeinit(opb);
  oggpack_write(opb, 0x05, 8);
  for (i = 0; i < 6; i++)
    oggpack_write(opb, magic[i], 8);

  /* 2 codebooks (count - 1 = 1) */
  oggpack_write(opb, 1, 8);

  /* Codebook 0: minimal valid codebook */
  oggpack_write(opb, 0x564342, 24);  /* sync */
  oggpack_write(opb, 1, 16);         /* dim = 1 */
  oggpack_write(opb, 2, 24);         /* entries = 2 */
  oggpack_write(opb, 0, 1);          /* unordered */
  oggpack_write(opb, 0, 1);          /* no unused entries */
  oggpack_write(opb, 0, 5);          /* lengthlist[0] = 1 */
  oggpack_write(opb, 0, 5);          /* lengthlist[1] = 1 */
  oggpack_write(opb, 1, 4);          /* lookup_type = 1 */
  oggpack_write(opb, 0, 32);         /* q_min */
  oggpack_write(opb, 0, 32);         /* q_delta */
  oggpack_write(opb, 0, 4);          /* q_quant - 1 = 0, so q_quant = 1 */
  oggpack_write(opb, 0, 1);          /* q_sequencep */
  /* quantvals for dim=1, entries=2: 2 values, each 1 bit (q_quant=1) */
  oggpack_write(opb, 0, 1);
  oggpack_write(opb, 0, 1);

  /* Codebook 1: large entry count, ordered encoding */
  {
    int k = ilog(entries) - 1;
    oggpack_write(opb, 0x564342, 24);    /* sync */
    oggpack_write(opb, dim, 16);         /* dim */
    oggpack_write(opb, entries, 24);     /* entries */
    oggpack_write(opb, 1, 1);            /* ordered */
    oggpack_write(opb, k, 5);            /* initial length = k+1 */
    /* single read marks all entries used at this length */
    oggpack_write(opb, entries, ilog(entries));
    oggpack_write(opb, 0, 4);            /* lookup_type = 0 (no mapping) */
  }

  /* Time backends: 1 entry, type 0 */
  oggpack_write(opb, 0, 6);   /* count - 1 = 0 */
  oggpack_write(opb, 0, 16);  /* type 0 */

  /* Floor backends: 1 entry, type 1 (floor1) */
  oggpack_write(opb, 0, 6);   /* count - 1 = 0 */
  oggpack_write(opb, 1, 16);  /* type 1 = floor1 */
  /* floor1 data: partitions=0, multiplier-1=0, rangebits=0 */
  oggpack_write(opb, 0, 5);   /* partitions = 0 */
  oggpack_write(opb, 0, 4);   /* multiplier - 1 = 0 */
  oggpack_write(opb, 0, 4);   /* rangebits = 0 */

  /* Residue backends: 1 entry, type 0 */
  oggpack_write(opb, 0, 6);   /* count - 1 = 0 */
  oggpack_write(opb, 0, 16);  /* type 0 = residue0 */
  oggpack_write(opb, 0, 24);  /* begin */
  oggpack_write(opb, 0, 24);  /* end */
  oggpack_write(opb, 0, 24);  /* partition_size - 1 */
  oggpack_write(opb, 0, 6);   /* classifications - 1 = 0 */
  oggpack_write(opb, 0, 8);   /* classbook = 0 */
  oggpack_write(opb, 0, 3);   /* low bits of cascade[0] */
  oggpack_write(opb, 0, 1);   /* bitflag = 0, no high bits */

  /* Mapping backends: 1 entry, type 0 */
  oggpack_write(opb, 0, 6);   /* count - 1 = 0 */
  oggpack_write(opb, 0, 16);  /* type 0 */
  oggpack_write(opb, 0, 1);   /* submaps flag = 0 (1 submap) */
  oggpack_write(opb, 0, 1);   /* coupling_steps flag = 0 */
  /* submap 0: floor=0, residue=0 */
  oggpack_write(opb, 0, 8);   /* time submap (unused) */
  oggpack_write(opb, 0, 8);   /* floor submap */
  oggpack_write(opb, 0, 8);   /* residue submap */

  /* Modes: 1 entry */
  oggpack_write(opb, 0, 6);   /* count - 1 = 0 */
  oggpack_write(opb, 0, 1);   /* blockflag = 0 */
  oggpack_write(opb, 0, 16);  /* windowtype = 0 */
  oggpack_write(opb, 0, 16);  /* transformtype = 0 */
  oggpack_write(opb, 0, 8);   /* mapping = 0 */

  /* EOP framing bit */
  oggpack_write(opb, 1, 1);
}

static ogg_packet make_packet(oggpack_buffer *opb, int b_o_s, int packetno) {
  ogg_packet op;
  memset(&op, 0, sizeof(op));
  op.packet = oggpack_get_buffer(opb);
  op.bytes = oggpack_bytes(opb);
  op.b_o_s = b_o_s;
  op.e_o_s = 0;
  op.granulepos = 0;
  op.packetno = packetno;
  return op;
}

/* Returns 0 on pass, 1 on fail. */
static int run_test(int dim, long entries) {
  vorbis_info vi;
  vorbis_comment vc;
  vorbis_dsp_state vd;
  oggpack_buffer opb_id, opb_comment, opb_setup;
  ogg_packet op;
  int ret;
  int result = 0;

  printf("  dim=%d, entries=%ld (ov_ilog check: %d+%d=%d)...\n",
         dim, entries, ilog(dim), ilog((unsigned int)entries),
         ilog(dim) + ilog((unsigned int)entries));

  vorbis_info_init(&vi);
  vorbis_comment_init(&vc);

  /* Build all three packet buffers up front so the cleanup path can
     unconditionally call oggpack_writeclear on each one, regardless
     of where we jump from. Each build_* function calls
     oggpack_writeinit on its buffer. */
  build_id_header(&opb_id);
  build_comment_header(&opb_comment);
  build_setup_header(&opb_setup, dim, entries);

  op = make_packet(&opb_id, 1, 0);
  ret = vorbis_synthesis_headerin(&vi, &vc, &op);
  if (ret != 0) {
    fprintf(stderr, "    FAIL: id header rejected (ret=%d)\n", ret);
    result = 1;
    goto cleanup;
  }

  op = make_packet(&opb_comment, 0, 1);
  ret = vorbis_synthesis_headerin(&vi, &vc, &op);
  if (ret != 0) {
    fprintf(stderr, "    FAIL: comment header rejected (ret=%d)\n", ret);
    result = 1;
    goto cleanup;
  }

  op = make_packet(&opb_setup, 0, 2);
  ret = vorbis_synthesis_headerin(&vi, &vc, &op);
  if (ret != 0) {
    printf("    PASS: setup header rejected at headerin (ret=%d)\n", ret);
    goto cleanup;
  }

  printf("    Setup header accepted, calling vorbis_synthesis_init...\n");
  fflush(stdout);

  ret = vorbis_synthesis_init(&vd, &vi);
  if (ret == 0) {
    printf("    PASS: vorbis_synthesis_init succeeded (ret=0)\n");
    vorbis_dsp_clear(&vd);
  } else {
    printf("    PASS: vorbis_synthesis_init returned cleanly (ret=%d)\n", ret);
  }

cleanup:
  oggpack_writeclear(&opb_id);
  oggpack_writeclear(&opb_comment);
  oggpack_writeclear(&opb_setup);
  vorbis_comment_clear(&vc);
  vorbis_info_clear(&vi);
  return result;
}

/*
 * For each dim, entries is the maximum that passes the
 * ov_ilog(dim)+ov_ilog(entries)<=24 check in vorbis_staticbook_unpack(),
 * which is 2^(24-ov_ilog(dim)) - 1 for dim>0 and 2^23 for dim=0.
 */
int main(int argc, char *argv[]) {
  struct { int dim; long entries; } tests[] = {
    { 0,  1L << 23 },        /* 8388608:  ov_ilog 0+24=24 */
    { 1,  (1L << 23) - 1 }, /* 8388607:  ov_ilog 1+23=24 */
    { 2,  (1L << 22) - 1 }, /* 4194303:  ov_ilog 2+22=24 */
    { 4,  (1L << 21) - 1 }, /* 2097151:  ov_ilog 3+21=24 */
    { 8,  (1L << 20) - 1 }, /* 1048575:  ov_ilog 4+20=24 */
    { 16, (1L << 19) - 1 }, /* 524287:   ov_ilog 5+19=24 */
    { 32, (1L << 18) - 1 }, /* 262143:   ov_ilog 6+18=24 */
    { 64, (1L << 17) - 1 }, /* 131071:   ov_ilog 7+17=24 */
  };
  int ntests = sizeof(tests) / sizeof(tests[0]);
  int i, start = 0, end = ntests, failures = 0;

  /* Optional: run a single test by index (argv[1]) */
  if (argc > 1) {
    start = atoi(argv[1]);
    if (start < 0 || start >= ntests) {
      fprintf(stderr, "Usage: %s [test_index 0..%d]\n", argv[0], ntests - 1);
      return 1;
    }
    end = start + 1;
  }

  printf("Testing codebooks with large entry counts across multiple dim values...\n\n");

  for (i = start; i < end; i++) {
    if (run_test(tests[i].dim, tests[i].entries) != 0)
      failures++;
  }

  printf("\n%d/%d tests passed.\n", (end - start) - failures, end - start);
  if (failures) {
    fprintf(stderr, "FAIL: %d test(s) failed\n", failures);
    return 1;
  }
  printf("All tests passed.\n");
  return 0;
}
