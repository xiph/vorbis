## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_PROGRAMS = decoder_example encoder_example chaining_example\
		vorbisfile_example seeking_example

LDADD = $(top_srcdir)/lib/.libs/libvorbis.a

decoder_example_SOURCES = decoder_example.c
encoder_example_SOURCES = encoder_example.c
chaining_example_SOURCES = chaining_example.c
chaining_example_LDADD = $(top_srcdir)/lib/.libs/libvorbisfile.a $(top_srcdir)/lib/.libs/libvorbis.a
vorbisfile_example_SOURCES = vorbisfile_example.c
vorbisfile_example_LDADD = $(top_srcdir)/lib/.libs/libvorbisfile.a $(top_srcdir)/lib/.libs/libvorbis.a
seeking_example_SOURCES = seeking_example.c
seeking_example_LDADD = $(top_srcdir)/lib/.libs/libvorbisfile.a $(top_srcdir)/lib/.libs/libvorbis.a

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
