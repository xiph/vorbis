## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS = foreign

INCLUDES = -I../lib -I$(top_srcdir)/include @OGG_CFLAGS@

EXTRA_PROGRAMS = lspvqtrain genericvqtrain residuevqtrain vqbuild\
	vqcascade vqmetrics latticebuild latticepare latticehint\
	latticetune huffbuild residuesplit distribution
CLEANFILES = $(EXTRA_PROGRAMS)

LDFLAGS = -static
LDADD = ../lib/libvorbis.la

lspvqtrain_SOURCES = lspdata.c train.c vqgen.c bookutil.c\
	vqgen.h bookutil.h vqext.h
genericvqtrain_SOURCES = genericdata.c train.c vqgen.c bookutil.c\
	vqgen.h bookutil.h vqext.h
residuevqtrain_SOURCES = residuedata.c train.c vqgen.c bookutil.c\
	vqgen.h bookutil.h vqext.h
vqbuild_SOURCES = build.c vqgen.c bookutil.c vqsplit.c\
	vqgen.h bookutil.h vqext.h
vqcascade_SOURCES = run.c cascade.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
vqmetrics_SOURCES = run.c metrics.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
latticebuild_SOURCES = latticebuild.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
latticepare_SOURCES = latticepare.c vqgen.c bookutil.c vqsplit.c\
	vqgen.h bookutil.h
latticehint_SOURCES = latticehint.c bookutil.c\
	bookutil.h
latticetune_SOURCES = latticetune.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
huffbuild_SOURCES = huffbuild.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
residuesplit_SOURCES = residuesplit.c vqgen.c bookutil.c\
	vqgen.h bookutil.h
distribution_SOURCES = distribution.c bookutil.c\
	bookutil.h

EXTRA_DIST = residue_entropy auxpartition.pl

debugvq:
	$(MAKE) vq CFLAGS="@DEBUG@"

profilevq:
	$(MAKE) vq CFLAGS="@PROFILE@"

vq: 
	$(MAKE) $(EXTRA_PROGRAMS) 
