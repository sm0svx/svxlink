
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

BUILT_SOURCES = $(QTELUSERDOC)

# install in $docdir
doc_DATA = $(QTELUSERDOC)

$(QTELUSERDOC): $(DOCSOURCES)
	$(AM_V_GEN)TEXINPUTS="$(srcdir):" \
		@PDFLATEX@ -interaction nonstopmode $< > /dev/null