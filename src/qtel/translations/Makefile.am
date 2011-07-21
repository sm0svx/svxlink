
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

QT_MESSAGES = $(TRANSLATIONS:.ts=.qm)

BUILT_SOURCES = $(QT_MESSAGES)

CLEANFILES = $(BUILT_SOURCES)

messagedir = @QTELMESSAGEDIR@
message_DATA = $(QT_MESSAGES)

EXTRA_DIST = $(TRANSLATIONS)

SUFFIXES = .qm .ts

.ts.qm:
	$(AM_V_GEN)@QT_LRELEASE@ $<
