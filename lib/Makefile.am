## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

lib_LTLIBRARIES = libvorbis.la

libvorbis_la_SOURCES = mdct.c smallft.c block.c envelope.c window.c lsp.c lpc.c\
			analysis.c synthesis.c psy.c info.c time0.c floor0.c\
			res0.c mapping0.c registry.c codebook.c sharedbook.c\
			iir.c vorbisfile.c\
			envelope.h lpc.h lsp.h bookinternal.h misc.h psy.h\
			masking.h sharedbook.h iir.h os.h
libvorbis_la_LDFLAGS = -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
