## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I../include

lib_LTLIBRARIES = libvorbis.la libvorbisfile.la libvorbisenc.la

libvorbis_la_SOURCES = mdct.c smallft.c block.c envelope.c window.c lsp.c lpc.c\
			analysis.c synthesis.c psy.c info.c time0.c floor0.c\
			res0.c mapping0.c registry.c codebook.c sharedbook.c\
			iir.c lookup.c\
			envelope.h lpc.h lsp.h codebook.h misc.h psy.h\
			masking.h sharedbook.h iir.h os.h mdct.h smallft.h\
			registry.h scales.h window.h lookup.h lookup_data.h\
			codec_internal.h backends.h
libvorbis_la_LDFLAGS = -version-info @V_LIB_CURRENT@:@V_LIB_REVISION@:@V_LIB_AGE@

libvorbisfile_la_SOURCES = vorbisfile.c
libvorbisfile_la_LDFLAGS = -version-info @VF_LIB_CURRENT@:@VF_LIB_REVISION@:@VF_LIB_AGE@

libvorbisenc_la_SOURCES = vorbisenc.c\
			os.h misc.h mode_A.h mode_B.h mode_C.h mode_D.h\
			mode_E.h modes.h\
			res0_1024a_192_4.vqh res0_128a_192_3.vqh\
			lsp12_0.vqh res0_1024a_192_5.vqh\
			res0_128a_192_4.vqh lsp30_0.vqh res0_1024a_256_1.vqh\
			res0_128a_192_5.vqh res0_1024a_128_1.vqh\
	 		res0_1024a_256_2.vqh res0_128a_256_1.vq\
			res0_1024a_128_2.vqh res0_1024a_256_3.vqh\
			res0_128a_256_2.vqh res0_1024a_128_3.vqh\
			res0_1024a_256_4.vqh res0_128a_256_3.vqh\
			res0_1024a_128_4.vqh res0_1024a_256_5.vqh\
			res0_128a_256_4.vqh res0_1024a_128_5.vqh\
			res0_1024a_350_1.vqh res0_128a_256_5.vqh\
			res0_1024a_128_6.vqh res0_1024a_350_2.vqh\
			res0_128a_350_1.vqh res0_1024a_128_7.vqh\
			res0_1024a_350_3.vqh res0_128a_350_2.vqh\
			res0_1024a_128_8.vqh res0_1024a_350_4.vqh\
			res0_128a_350_3.vqh res0_1024a_128_9.vqh\
			res0_1024a_350_5.vqh res0_128a_350_4.vqh\
			res0_1024a_160_1.vqh res0_128a_128_1.vqh\
			res0_128a_350_5.vqh res0_1024a_160_2.vqh\
			res0_128a_128_2.vqh resaux0_1024a_128.vqh\
			res0_1024a_160_3.vqh res0_128a_128_3.vqh\
			resaux0_1024a_160.vqh res0_1024a_160_4.vqh\
			res0_128a_128_4.vqh resaux0_1024a_192.vqh\
			res0_1024a_160_5.vqh res0_128a_128_5.vqh\
			resaux0_1024a_256.vqh res0_1024a_160_6.vqh\
			res0_128a_160_1.vqh resaux0_1024a_350.vqh\
			res0_1024a_160_7.vqh res0_128a_160_2.vqh\
			resaux0_128a_128.vqh res0_1024a_160_8.vqh\
			res0_128a_160_3.vqh resaux0_128a_160.vqh\
			res0_1024a_160_9.vqh res0_128a_160_4.vqh\
			resaux0_128a_192.vqh res0_1024a_192_1.vqh\
			res0_128a_160_5.vqh resaux0_128a_256.vqh\
			res0_1024a_192_2.vqh res0_128a_192_1.vqh\
			resaux0_128a_350.vqh res0_1024a_192_3.vqh\
			 res0_128a_192_2.vqh

libvorbisenc_la_LDFLAGS = -version-info @VF_LIB_CURRENT@:@VF_LIB_REVISION@:@VF_LIB_AGE@

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
