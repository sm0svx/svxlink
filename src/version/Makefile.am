
# the version files are provided for compatibility
# it may be better to manage them in 'configure.ac'

# for the time being, generating them in this manner prevents
# unnecessary recompilation due to dependencies

BUILT_SOURCES = @VERSIONHEADERS@

CLEANFILES = $(BUILT_SOURCES)

SUFFIXES = .h

@VERSIONHEADERS@: $(top_srcdir)/@VERSIONFILE@
	@echo Creating "$(builddir)/$@"
	@while read LINE ; \
	do \
	PACKAGE="`echo $${LINE} | $(AWK) -F\= '/^.*=.*$$/ {print $$1}'`" ; \
	if [ \"$${PACKAGE}\" = \"$*\" ] ; then \
	HEADER="$${PACKAGE}.h" ; \
        LABEL="$${PACKAGE}_VERSION" ; \
	VERSION="`echo $${LINE} | $(AWK) -F\= '/^.*=.*$$/ {print $$2}'`" ; \
	echo "#ifndef $${LABEL}" > "$${HEADER}" ; \
	echo "#define $${LABEL} \"$${VERSION}\"" >> "$${HEADER}" ; \
	echo "#endif" >> "$${HEADER}" ; \
	fi ; \
	done < "$(top_srcdir)/@VERSIONFILE@"

