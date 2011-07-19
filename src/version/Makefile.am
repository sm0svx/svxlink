# Import some general items
include $(top_srcdir)/common.mk

# these version files are provided for compatibility
# it may ultimately be best to manage them in 'configure.ac'

# for the time being, generating them in this manner prevents
# unnecessary recompilation due to dependencies

BUILT_SOURCES = @VERSIONHEADERS@

CLEANFILES = $(BUILT_SOURCES)

@VERSIONHEADERS@: $(top_srcdir)/@VERSIONFILE@
	@echo Creating $(builddir)/$@
	@while read LINE; \
	do \
	PKGNAME="`echo $${LINE} | $(AWK) -F\= '/^.*=.*$$/ {print $$1}'`" ; \
	if [ \"$${PKGNAME}\" = \"$*\" ] ; then \
	FILENAME="$${PKGNAME}.h" ; \
        VARNAME="$${PKGNAME}_VERSION" ; \
	VERSION="`echo $${LINE} | $(AWK) -F\= '/^.*=.*$$/ {print $$2}'`" ; \
	echo "#ifndef $${VARNAME}" > "$${FILENAME}" ; \
	echo "#define $${VARNAME} \"$${VERSION}\"" >> "$${FILENAME}" ; \
	echo "#endif" >> "$${FILENAME}" ; \
	fi ; \
	done < "$(top_srcdir)/@VERSIONFILE@"
