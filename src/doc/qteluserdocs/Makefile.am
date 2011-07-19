QTELUSERDOC = $(builddir)/qteluserdoc.pdf

DOCSOURCES = \
		$(srcdir)/qteluserdoc.tex \
		$(srcdir)/qtel-mainwindow.pdf \
		$(srcdir)/qtel-comdialog.pdf \
		$(srcdir)/qtel-settings.pdf

EXTRA_DIST = $(DOCSOURCES)

CLEANFILES = $(QTELUSERDOC) \
		qteluserdoc.aux \
		qteluserdoc.log \
		qteluserdoc.lof \
		qteluserdoc.toc


if HAVE_PDFLATEX

BUILT_SOURCES = $(QTELUSERDOC)

# install in $datadir
doc_DATA = $(QTELUSERDOC)

$(QTELUSERDOC): $(DOCSOURCES)
	echo Building documentation \'$(QTELUSERDOC)\'.
	TEXINPUTS="$(srcdir):" @PDFLATEX@ $< > /dev/null

endif
