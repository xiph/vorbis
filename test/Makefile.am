## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

check_PROGRAMS = test test_codebook

check: $(check_PROGRAMS)
	./test$(EXEEXT)
	./test_codebook$(EXEEXT)

AM_CPPFLAGS = -I$(top_srcdir)/include @OGG_CFLAGS@

test_SOURCES = util.c util.h write_read.c write_read.h test.c
test_LDADD = ../lib/libvorbisenc.la ../lib/libvorbis.la @OGG_LIBS@ @VORBIS_LIBS@

test_codebook_SOURCES = test_codebook.c
test_codebook_LDADD = ../lib/libvorbis.la @OGG_LIBS@ @VORBIS_LIBS@

EXTRA_DIST = CMakeLists.txt

debug:
	$(MAKE) check CFLAGS="@DEBUG@"

profile:
	$(MAKE) check CFLAGS="@PROFILE@"
