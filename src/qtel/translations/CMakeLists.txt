# Language translations
set(TRANSLATIONS
  qtel_sv.ts qtel_de.ts qtel_tr.ts qtel_nl.ts qtel_it.ts qtel_uk.ts qtel_ru.ts
  qtel_hu.ts qtel_ja.ts qtel_fr.ts qtel_es.ts
  )

if (Qt5Core_FOUND)
  find_package(Qt5LinguistTools REQUIRED)
  QT5_ADD_TRANSLATION(TRANSLATIONS_QM ${TRANSLATIONS})
elseif (QT4_FOUND)
  QT4_ADD_TRANSLATION(TRANSLATIONS_QM ${TRANSLATIONS})
else()
  message(FATAL_ERROR "Neither Qt4 nor Qt5 translation tools are present")
endif()

add_custom_target(translations ALL DEPENDS ${TRANSLATIONS_QM})

install(FILES ${TRANSLATIONS_QM}
  DESTINATION ${SHARE_INSTALL_PREFIX}/qtel/translations
  )
