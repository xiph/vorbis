## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = lib include doc examples win32 debian

m4datadir = $(datadir)/aclocal
m4data_DATA = vorbis.m4

EXTRA_DIST = README AUTHORS COPYING libvorbis.spec vorbis.m4

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
