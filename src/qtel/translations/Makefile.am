# Import some general items
include $(top_srcdir)/common.mk

all-local: $(TRANSLATIONS)

TRANSLATIONS = \
		qtel_tmpl.ts \
		qtel_sv.ts \
		qtel_de.ts \
		qtel_tr.ts \
		qtel_nl.ts \
		qtel_it.ts \
		qtel_uk.ts \
		qtel_ru.ts \
		qtel_hu.ts \
		qtel_ja.ts \
		qtel_es.ts

LRELEASE_GENERATED = $(TRANSLATIONS:.ts=.qm)

EXTRA_DIST = $(TRANSLATIONS)

CLEANFILES = $(LRELEASE_GENERATED)

SUFFIXES += .qm .ts

.ts.qm:
	echo Creating translation file $@...
	@QT_LRELEASE@ $<
