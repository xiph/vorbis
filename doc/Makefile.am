## Process this with automake to create Makefile.in

AUTOMAKE_OPTIONS = foreign

SUBDIRS = vorbisfile vorbisenc

docdir = $(datadir)/doc/$(PACKAGE)-$(VERSION)

static_docs = components.png \
	draft-kerr-avt-vorbis-rtp-02.txt \
	eightphase.png \
	evenlsp.png \
	floor1-1.png \
	floor1-2.png \
	floor1-3.png \
	floor1-4.png \
	floor1_inverse_dB_table.html \
	floorval.png \
	fourphase.png \
	framing.html \
	helper.html \
	hufftree-under.png \
	hufftree.png \
	index.html \
	lspmap.png \
	oddlsp.png \
	oggstream.html \
	programming.html \
	residue-pack.png \
	residue2.png \
	squarepolar.png \
	stereo.html \
	stream.png \
	v-comment.html \
	vorbis-clip.txt \
	vorbis-errors.txt \
	vorbis-fidelity.html \
	vorbis.html \
	vorbisword2.png \
	wait.png \
	white-ogg.png \
	white-xifish.png \
	window1.png \
	window2.png

built_docs = Vorbis_I_spec.html Vorbis_I_spec.pdf

# conditionally make the generated documentation
if BUILD_DOCS
doc_DATA = $(static_docs) $(built_docs)
else
doc_DATA = $(static_docs)
endif

EXTRA_DIST = $(static_docs) $(built_docs)

doc_sources = xml/Vorbis_I_spec.xml \
	xml/01-introduction.xml \
	xml/02-bitpacking.xml \
	xml/03-codebook.xml \
	xml/04-codec.xml \
	xml/05-comment.xml \
	xml/06-floor0.xml \
	xml/07-floor1.xml \
	xml/08-residue.xml \
	xml/09-helper.xml \
	xml/10-tables.xml \
	xml/a1-encapsulation_ogg.xml \
	xml/a2-encapsulation_rtp.xml \
	xml/footer.xml \
	xml/spec-common.xsl \
	xml/spec-fo.xsl \
	xml/spec-html.xsl

nobase_EXTRA_DIST = $(doc_sources)

# these are expensive; only remove if we have to
MAINTAINERCLEANFILES = $(built_docs)
CLEANFILES = Vorbis_I_spec.fo spec.aux spec.log spec.out

# explicit rules for generating docs
Vorbis_I_spec.html:
	xsltproc --xinclude --output $@ $(srcdir)/xml/spec-html.xsl $(srcdir)/xml/Vorbis_I_spec.xml

Vorbis_I_spec.fo:
	xsltproc --xinclude --output $@ $(srcdir)/xml/spec-fo.xsl $(srcdir)/xml/Vorbis_I_spec.xml

Vorbis_I_spec.pdf: Vorbis_I_spec.fo
	cp $< spec.fo # work around a passivetex bug
	pdfxmltex --interaction nonstopmode spec.fo
	pdfxmltex --interaction nonstopmode spec.fo
	rm spec.fo
	mv spec.pdf $@

