# $Id: Makefile,v 1.3 2000/11/08 03:23:23 xiphmont Exp $

###############################################################################
#                                                                             #
# These are vq codebook generation tools for use by team members.  This       #
# makefile assumes gcc.                                                       #
#                                                                             #
# One need not build these utils to make libvorbis                            #
#                                                                             #
###############################################################################

FLAGS=-I. -I../lib -I../include 
OPT=-O20 $(FLAGS)
DEBUG=-g -Wall $(FLAGS)
PROFILE=-g -pg -O20 $(FLAGS)
CC=gcc
LD=gcc
LDFLAGS=$(FLAGS)
LIBS=-lm 

HFILES =	../lib/codebook.h vqgen.h vqext.h bookutil.h

OFILES = 	vqgen.o vqsplit.o bookutil.o ../lib/sharedbook.o
ALLOFILES = 	$(OFILES) lspdata.o genericdata.o train.o build.o run.o\
		cascade.o partition.o metrics.o residuedata.o latticebuild.o\
		latticepare.o latticehint.o latticetune.o

all:
	$(MAKE) target CFLAGS="$(OPT)"

debug:	
	$(MAKE) target CFLAGS="$(DEBUG)"

profile: 
	$(MAKE) target CFLAGS="$(PROFILE)"

target:	lspvqtrain genericvqtrain residuevqtrain vqbuild vqcascade vqmetrics latticebuild latticepare latticehint latticetune huffbuild residuesplit

lspvqtrain:	$(OFILES) lspdata.o train.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

residuevqtrain:	$(OFILES) residuedata.o train.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

genericvqtrain:	$(OFILES) genericdata.o train.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

vqbuild:	$(OFILES) build.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

vqcascade:	$(OFILES) run.o cascade.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

vqmetrics:	$(OFILES) run.o metrics.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

latticebuild:	$(OFILES) latticebuild.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

latticehint:	$(OFILES) latticehint.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

latticepare:	$(OFILES) latticepare.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

latticetune:	$(OFILES) latticetune.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

huffbuild:	$(OFILES) huffbuild.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)

residuesplit:	$(OFILES) residuesplit.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)


$(ALLOFILES): 	$(HFILES)

.c.o:
	$(CC) $(CFLAGS) -c $<

$(OFILES):	$(HFILES)

clean:
	-rm -f *.o *.a test* *~ *.out *.m config.* \
		lspvqtrain genericvqtrain residuevqtrain\
		vqbuild vqmetrics latticebuild vqcascade latticepare\
		huffbuild residuesplit

distclean:	clean
	-rm -f Makefile
