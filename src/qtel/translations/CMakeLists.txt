# Language translations
set(TRANSLATIONS
  qtel_sv.qm qtel_de.qm qtel_tr.qm qtel_nl.qm qtel_it.qm qtel_uk.qm qtel_ru.qm
  qtel_hu.qm qtel_ja.qm qtel_fr.qm qtel_es.qm
  )

# The QT4_ADD_TRANSLATION macro does not seem to work on CentOS 5.
# We'll have to do it manually.
#QT4_ADD_TRANSLATION(TRANSLATIONS_QM ${TRANSLATIONS})
set(TRANSLATIONS_QM)
foreach(qm ${TRANSLATIONS})
  string(REGEX REPLACE ^\(.*\)\\.qm \\1 trans ${qm})
  list(APPEND TRANSLATIONS_QM ${trans}.qm)
  set(silent)
  add_custom_command(OUTPUT ${trans}.qm DEPENDS ${trans}.ts
    COMMAND ${QT_LRELEASE_EXECUTABLE}
              ${CMAKE_CURRENT_SOURCE_DIR}/${trans}.ts
              -qm ${CMAKE_CURRENT_BINARY_DIR}/${trans}.qm
    COMMENT "Adding translation: ${trans}..."
    )
  install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${trans}.qm
    DESTINATION ${SHARE_INSTALL_PREFIX}/qtel/translations
    )
endforeach(qm)

add_custom_target(translations ALL DEPENDS ${TRANSLATIONS_QM})

