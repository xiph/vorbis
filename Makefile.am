## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign dist-zip

SUBDIRS = lib include doc examples win32 debian vq

m4datadir = $(datadir)/aclocal
m4data_DATA = vorbis.m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = vorbis.pc vorbisenc.pc vorbisfile.pc

EXTRA_DIST = HACKING todo.txt macos macosx autogen.sh libvorbis.spec \
	vorbis.m4 vorbis.pc.in vorbisenc.pc.in vorbisfile.pc.in

dist-hook:
	rm -rf `find $(distdir)/macos -name CVS`
	rm -rf `find $(distdir)/macosx -name CVS`

debug:
	$(MAKE) all CFLAGS="@DEBUG@"

profile:
	$(MAKE) all CFLAGS="@PROFILE@"
