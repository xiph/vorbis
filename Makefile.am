## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = lib include doc examples win32 debian vq

m4datadir = $(datadir)/aclocal
m4data_DATA = vorbis.m4

EXTRA_DIST = HACKING todo.txt mac \
	autogen.sh libvorbis.spec vorbis.m4

dist-hook:
	rm -rf `find $(distdir)/mac -name CVS`

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
