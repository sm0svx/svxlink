.SILENT:

%PHONY: all
all:	qteluserdoc.pdf


qteluserdoc.pdf: qteluserdoc.tex qtel-mainwindow.pdf qtel-comdialog.pdf qtel-settings.pdf
	echo "--- Running pdflatex on $<..."
	pdflatex $< > /dev/null
	pdflatex $< > /dev/null


%PHONY: clean realclean
clean realclean:
	$(RM) -f qteluserdoc.aux qteluserdoc.log qteluserdoc.lof \
		 qteluserdoc.pdf qteluserdoc.toc

