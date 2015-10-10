# Language translations
set(TRANSLATIONS
  qtel_sv.ts qtel_de.ts qtel_tr.ts qtel_nl.ts qtel_it.ts qtel_uk.ts qtel_ru.ts
  qtel_hu.ts qtel_ja.ts qtel_fr.ts qtel_es.ts
  )
  
# The QT5_ADD_TRANSLATION macro does not seem to work on CentOS 5.
# We'll have to do it manually.
qt5_add_translation(TRANSLATIONS_QM ${TRANSLATIONS})
#set(TRANSLATIONS_QM)
#foreach(qm ${TRANSLATIONS})
#  string(REGEX REPLACE ^\(.*\)\\.qm \\1 trans ${qm})
#  list(APPEND TRANSLATIONS_QM ${trans}.qm)
#  set(silent)
#  add_custom_command(OUTPUT ${trans}.qm DEPENDS ${trans}.ts
#    COMMAND ${QT_LRELEASE_EXECUTABLE}
#              ${CMAKE_CURRENT_SOURCE_DIR}/${trans}.ts
#              -qm ${CMAKE_CURRENT_BINARY_DIR}/${trans}.qm
#    COMMENT "Adding translation: ${trans}..."
#    )
#  install(
#    FILES ${CMAKE_CURRENT_BINARY_DIR}/${trans}.qm
#    DESTINATION ${SHARE_INSTALL_PREFIX}/qtel/translations
#    )
#endforeach(qm)

add_custom_target(translations ALL DEPENDS ${TRANSLATIONS_QM})

