## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I$(top_srcdir)/include

noinst_PROGRAMS = decoder_example encoder_example chaining_example\
		vorbisfile_example

decoder_example_SOURCES = decoder_example.c
decoder_example_LDFLAGS = -L$(top_srcdir)/lib/.libs -lvorbis
encoder_example_SOURCES = encoder_example.c
encoder_example_LDFLAGS = -L$(top_srcdir)/lib/.libs -lvorbis
chaining_example_SOURCES = chaining_example.c
chaining_example_LDFLAGS = -L$(top_srcdir)/lib/.libs -lvorbis
vorbisfile_example_SOURCES = vorbisfile_example.c
vorbisfile_example_LDFLAGS = -L$(top_srcdir)/lib/.libs -lvorbis

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
