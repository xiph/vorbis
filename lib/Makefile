# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.4-p4 from Makefile.am

# Copyright (C) 1994, 1995-8, 1999 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

DESTDIR =

pkgdatadir = $(datadir)/libvorbis
pkglibdir = $(libdir)/libvorbis
pkgincludedir = $(includedir)/libvorbis

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL} $(AM_INSTALL_PROGRAM_FLAGS)
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
host_alias = powerpc-unknown-linux-gnu
host_triplet = powerpc-unknown-linux-gnu
AS = @AS@
CC = gcc
CPP = gcc -E
DEBUG = -g -Wall -D_REENTRANT -D__NO_MATH_INLINES -fsigned-char
DLLTOOL = @DLLTOOL@
ECHO = echo
EXEEXT = 
LIBS = -lm  -logg
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
MAKEINFO = makeinfo
OBJDUMP = @OBJDUMP@
OBJEXT = o
OGG_CFLAGS = -INONE/include
OGG_LIBS =  -logg
PACKAGE = libvorbis
PROFILE = -pg -g -O20 -ffast-math -D_REENTRANT -fsigned-char
RANLIB = ranlib
STRIP = strip
VERSION = 1.0rc2
VE_LIB_AGE = 0
VE_LIB_CURRENT = 0
VE_LIB_REVISION = 1
VF_LIB_AGE = 1
VF_LIB_CURRENT = 1
VF_LIB_REVISION = 0
V_LIB_AGE = 0
V_LIB_CURRENT = 0
V_LIB_REVISION = 1
pthread_lib = -lpthread

AUTOMAKE_OPTIONS = foreign

SUBDIRS = modes books

INCLUDES = -I$(top_srcdir)/include -INONE/include

lib_LTLIBRARIES = libvorbis.la libvorbisfile.la libvorbisenc.la

libvorbis_la_SOURCES = mdct.c smallft.c block.c envelope.c window.c lsp.c 			lpc.c analysis.c synthesis.c psy.c info.c time0.c 			floor1.c floor0.c 			res0.c mapping0.c registry.c codebook.c sharedbook.c 			lookup.c bitrate.c 			envelope.h lpc.h lsp.h codebook.h misc.h psy.h 			masking.h iir.h os.h mdct.h smallft.h 			registry.h scales.h window.h lookup.h lookup_data.h 			codec_internal.h backends.h bitrate.h 

libvorbis_la_LDFLAGS = -version-info 0:1:0

libvorbisfile_la_SOURCES = vorbisfile.c
libvorbisfile_la_LDFLAGS = -version-info 1:0:1

libvorbisenc_la_SOURCES = vorbisenc.c registry-api.h
libvorbisenc_la_LDFLAGS = -version-info 0:1:0

EXTRA_PROGRAMS = barkmel tone psytune
CLEANFILES = $(EXTRA_PROGRAMS)

barkmel_SOURCES = barkmel.c
tone_SOURCES = tone.c
psytune_SOURCES = psytune.c
psytune_LDFLAGS = -static
psytune_LDADD = libvorbis.la

EXTRA_DIST = lookups.pl iir.c
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_CLEAN_FILES = 
LTLIBRARIES =  $(lib_LTLIBRARIES)


DEFS =  -DPACKAGE=\"libvorbis\" -DVERSION=\"1.0rc2\" -DHAVE_DLFCN_H=1 -DHAVE_ALLOCA_H=1 -DHAVE_ALLOCA=1 -DHAVE_SQRTF=1 -DHAVE_LOGF=1 -DHAVE_EXPF=1 -DHAVE_ACOSF=1 -DHAVE_ATANF=1 -DHAVE_FREXPF=1 -DHAVE_RINTF=1  -I. -I$(srcdir) -DTRAIN_RES
CPPFLAGS = 
LDFLAGS =  
libvorbis_la_LIBADD = 
libvorbis_la_OBJECTS =  mdct.lo smallft.lo block.lo envelope.lo \
window.lo lsp.lo lpc.lo analysis.lo synthesis.lo psy.lo info.lo \
time0.lo floor1.lo floor0.lo res0.lo mapping0.lo registry.lo \
codebook.lo sharedbook.lo lookup.lo bitrate.lo
libvorbisfile_la_LIBADD = 
libvorbisfile_la_OBJECTS =  vorbisfile.lo
libvorbisenc_la_LIBADD = 
libvorbisenc_la_OBJECTS =  vorbisenc.lo
barkmel_OBJECTS =  barkmel.$(OBJEXT)
barkmel_LDADD = $(LDADD)
barkmel_DEPENDENCIES = 
barkmel_LDFLAGS = 
tone_OBJECTS =  tone.$(OBJEXT)
tone_LDADD = $(LDADD)
tone_DEPENDENCIES = 
tone_LDFLAGS = 
psytune_OBJECTS =  psytune.$(OBJEXT)
psytune_DEPENDENCIES =  libvorbis.la
CFLAGS = -O20 -ffast-math -D_REENTRANT -fsigned-char  -DUSE_MEMORY_H
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(LIBTOOL) --mode=link $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  Makefile.am Makefile.in


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP_ENV = --best
DEP_FILES =  .deps/analysis.P .deps/barkmel.P .deps/bitrate.P \
.deps/block.P .deps/codebook.P .deps/envelope.P .deps/floor0.P \
.deps/floor1.P .deps/info.P .deps/lookup.P .deps/lpc.P .deps/lsp.P \
.deps/mapping0.P .deps/mdct.P .deps/psy.P .deps/psytune.P \
.deps/registry.P .deps/res0.P .deps/sharedbook.P .deps/smallft.P \
.deps/synthesis.P .deps/time0.P .deps/tone.P .deps/vorbisenc.P \
.deps/vorbisfile.P .deps/window.P
SOURCES = $(libvorbis_la_SOURCES) $(libvorbisfile_la_SOURCES) $(libvorbisenc_la_SOURCES) $(barkmel_SOURCES) $(tone_SOURCES) $(psytune_SOURCES)
OBJECTS = $(libvorbis_la_OBJECTS) $(libvorbisfile_la_OBJECTS) $(libvorbisenc_la_OBJECTS) $(barkmel_OBJECTS) $(tone_OBJECTS) $(psytune_OBJECTS)

all: all-redirect
.SUFFIXES:
.SUFFIXES: .S .c .lo .o .obj .s
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --foreign lib/Makefile

Makefile: $(srcdir)/Makefile.in  $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-libLTLIBRARIES:

clean-libLTLIBRARIES:
	-test -z "$(lib_LTLIBRARIES)" || rm -f $(lib_LTLIBRARIES)

distclean-libLTLIBRARIES:

maintainer-clean-libLTLIBRARIES:

install-libLTLIBRARIES: $(lib_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(libdir)
	@list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(LIBTOOL)  --mode=install $(INSTALL) $$p $(DESTDIR)$(libdir)/$$p"; \
	    $(LIBTOOL)  --mode=install $(INSTALL) $$p $(DESTDIR)$(libdir)/$$p; \
	  else :; fi; \
	done

uninstall-libLTLIBRARIES:
	@$(NORMAL_UNINSTALL)
	list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  $(LIBTOOL)  --mode=uninstall rm -f $(DESTDIR)$(libdir)/$$p; \
	done

# FIXME: We should only use cygpath when building on Windows,
# and only if it is available.
.c.obj:
	$(COMPILE) -c `cygpath -w $<`

.s.o:
	$(COMPILE) -c $<

.S.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core *.core
	-rm -f *.$(OBJEXT)

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.s.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

.S.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs _libs

distclean-libtool:

maintainer-clean-libtool:

libvorbis.la: $(libvorbis_la_OBJECTS) $(libvorbis_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libvorbis_la_LDFLAGS) $(libvorbis_la_OBJECTS) $(libvorbis_la_LIBADD) $(LIBS)

libvorbisfile.la: $(libvorbisfile_la_OBJECTS) $(libvorbisfile_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libvorbisfile_la_LDFLAGS) $(libvorbisfile_la_OBJECTS) $(libvorbisfile_la_LIBADD) $(LIBS)

libvorbisenc.la: $(libvorbisenc_la_OBJECTS) $(libvorbisenc_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libvorbisenc_la_LDFLAGS) $(libvorbisenc_la_OBJECTS) $(libvorbisenc_la_LIBADD) $(LIBS)

barkmel$(EXEEXT): $(barkmel_OBJECTS) $(barkmel_DEPENDENCIES)
	@rm -f barkmel$(EXEEXT)
	$(LINK) $(barkmel_LDFLAGS) $(barkmel_OBJECTS) $(barkmel_LDADD) $(LIBS)

tone$(EXEEXT): $(tone_OBJECTS) $(tone_DEPENDENCIES)
	@rm -f tone$(EXEEXT)
	$(LINK) $(tone_LDFLAGS) $(tone_OBJECTS) $(tone_LDADD) $(LIBS)

psytune$(EXEEXT): $(psytune_OBJECTS) $(psytune_DEPENDENCIES)
	@rm -f psytune$(EXEEXT)
	$(LINK) $(psytune_LDFLAGS) $(psytune_OBJECTS) $(psytune_LDADD) $(LIBS)

# This directory's subdirectories are mostly independent; you can cd
# into them and run `make' without going through this Makefile.
# To change the values of `make' variables: instead of editing Makefiles,
# (1) if the variable is set in `config.status', edit `config.status'
#     (which will cause the Makefiles to be regenerated when you run `make');
# (2) otherwise, pass the desired values on the `make' command line.



all-recursive install-data-recursive install-exec-recursive \
installdirs-recursive install-recursive uninstall-recursive  \
check-recursive installcheck-recursive info-recursive dvi-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	target=`echo $@ | sed s/-recursive//`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    dot_seen=yes; \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done; \
	if test "$$dot_seen" = "no"; then \
	  $(MAKE) $(AM_MAKEFLAGS) "$$target-am" || exit 1; \
	fi; test -z "$$fail"

mostlyclean-recursive clean-recursive distclean-recursive \
maintainer-clean-recursive:
	@set fnord $(MAKEFLAGS); amf=$$2; \
	dot_seen=no; \
	rev=''; list='$(SUBDIRS)'; for subdir in $$list; do \
	  rev="$$subdir $$rev"; \
	  test "$$subdir" = "." && dot_seen=yes; \
	done; \
	test "$$dot_seen" = "no" && rev=". $$rev"; \
	target=`echo $@ | sed s/-recursive//`; \
	for subdir in $$rev; do \
	  echo "Making $$target in $$subdir"; \
	  if test "$$subdir" = "."; then \
	    local_target="$$target-am"; \
	  else \
	    local_target="$$target"; \
	  fi; \
	  (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) $$local_target) \
	   || case "$$amf" in *=*) exit 1;; *k*) fail=yes;; *) exit 1;; esac; \
	done && test -z "$$fail"
tags-recursive:
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test "$$subdir" = . || (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) tags); \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES) $(LISP)
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	here=`pwd` && cd $(srcdir) \
	  && mkid -f$$here/ID $$unique $(LISP)

TAGS: tags-recursive $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
   if test "$$subdir" = .; then :; else \
	    test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
   fi; \
	done; \
	list='$(SOURCES) $(HEADERS)'; \
	unique=`for i in $$list; do echo $$i; done | \
	  awk '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(ETAGS_ARGS)$$unique$(LISP)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $$unique $(LISP) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = lib

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --foreign lib/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  if test -d $$d/$$file; then \
	    cp -pr $$d/$$file $(distdir)/$$file; \
	  else \
	    test -f $(distdir)/$$file \
	    || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	    || cp -p $$d/$$file $(distdir)/$$file || :; \
	  fi; \
	done
	for subdir in $(SUBDIRS); do \
	  if test "$$subdir" = .; then :; else \
	    test -d $(distdir)/$$subdir \
	    || mkdir $(distdir)/$$subdir \
	    || exit 1; \
	    chmod 777 $(distdir)/$$subdir; \
	    (cd $$subdir && $(MAKE) $(AM_MAKEFLAGS) top_distdir=../$(top_distdir) distdir=../$(distdir)/$$subdir distdir) \
	      || exit 1; \
	  fi; \
	done

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:
	-rm -rf .deps

maintainer-clean-depend:

%.o: %.c
	@echo '$(COMPILE) -c $<'; \
	$(COMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-cp .deps/$(*F).pp .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm .deps/$(*F).pp

%.lo: %.c
	@echo '$(LTCOMPILE) -c $<'; \
	$(LTCOMPILE) -Wp,-MD,.deps/$(*F).pp -c $<
	@-sed -e 's/^\([^:]*\)\.o[ 	]*:/\1.lo \1.o :/' \
	  < .deps/$(*F).pp > .deps/$(*F).P; \
	tr ' ' '\012' < .deps/$(*F).pp \
	  | sed -e 's/^\\$$//' -e '/^$$/ d' -e '/:$$/ d' -e 's/$$/ :/' \
	    >> .deps/$(*F).P; \
	rm -f .deps/$(*F).pp
info-am:
info: info-recursive
dvi-am:
dvi: dvi-recursive
check-am: all-am
check: check-recursive
installcheck-am:
installcheck: installcheck-recursive
install-exec-am: install-libLTLIBRARIES
install-exec: install-exec-recursive

install-data-am:
install-data: install-data-recursive

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am
install: install-recursive
uninstall-am: uninstall-libLTLIBRARIES
uninstall: uninstall-recursive
all-am: Makefile $(LTLIBRARIES)
all-redirect: all-recursive
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) AM_INSTALL_PROGRAM_FLAGS=-s install
installdirs: installdirs-recursive
installdirs-am:
	$(mkinstalldirs)  $(DESTDIR)$(libdir)


mostlyclean-generic:

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(CONFIG_CLEAN_FILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*

maintainer-clean-generic:
mostlyclean-am:  mostlyclean-libLTLIBRARIES mostlyclean-compile \
		mostlyclean-libtool mostlyclean-tags mostlyclean-depend \
		mostlyclean-generic

mostlyclean: mostlyclean-recursive

clean-am:  clean-libLTLIBRARIES clean-compile clean-libtool clean-tags \
		clean-depend clean-generic mostlyclean-am

clean: clean-recursive

distclean-am:  distclean-libLTLIBRARIES distclean-compile \
		distclean-libtool distclean-tags distclean-depend \
		distclean-generic clean-am
	-rm -f libtool

distclean: distclean-recursive

maintainer-clean-am:  maintainer-clean-libLTLIBRARIES \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-tags maintainer-clean-depend \
		maintainer-clean-generic distclean-am
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

maintainer-clean: maintainer-clean-recursive

.PHONY: mostlyclean-libLTLIBRARIES distclean-libLTLIBRARIES \
clean-libLTLIBRARIES maintainer-clean-libLTLIBRARIES \
uninstall-libLTLIBRARIES install-libLTLIBRARIES mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-libtool distclean-libtool clean-libtool \
maintainer-clean-libtool install-data-recursive \
uninstall-data-recursive install-exec-recursive \
uninstall-exec-recursive installdirs-recursive uninstalldirs-recursive \
all-recursive check-recursive installcheck-recursive info-recursive \
dvi-recursive mostlyclean-recursive distclean-recursive clean-recursive \
maintainer-clean-recursive tags tags-recursive mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir \
mostlyclean-depend distclean-depend clean-depend \
maintainer-clean-depend info-am info dvi-am dvi check check-am \
installcheck-am installcheck install-exec-am install-exec \
install-data-am install-data install-am install uninstall-am uninstall \
all-redirect all-am all installdirs-am installdirs mostlyclean-generic \
distclean-generic clean-generic maintainer-clean-generic clean \
mostlyclean distclean maintainer-clean


debug:
	$(MAKE) all CFLAGS="-g -Wall -D_REENTRANT -D__NO_MATH_INLINES -fsigned-char"

profile:
	$(MAKE) all CFLAGS="-pg -g -O20 -ffast-math -D_REENTRANT -fsigned-char"

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
