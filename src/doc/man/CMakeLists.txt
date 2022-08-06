# Add an empty man target and make the doc target depend on it. We'll add
# dependencies later
add_custom_target(man)
add_dependencies(doc man)

# Search for the gzip and groff programs
include(FindGROFF)
include(FindGZIP)
if(NOT GROFF_TOOL OR NOT GZIP_TOOL)
  message("--   The groff and gzip tools are optional dependencies.")
  message("--   They are used to build the UNIX manual pages. SvxLink will")
  message("--   build without them but usage documentation will be missing.")
endif(NOT GROFF_TOOL OR NOT GZIP_TOOL)

# Call this function to add manual pages to be built
function(add_manual_pages man_pages)
  if(GROFF_TOOL AND GZIP_TOOL)
    foreach(man ${ARGV})
      add_custom_command(
        OUTPUT ${man}.gz
        DEPENDS ${man}
        COMMAND ${GZIP_TOOL} -c ${CMAKE_CURRENT_SOURCE_DIR}/${man} > ${man}.gz
        COMMENT "Compressing manual page ${man}.gz..."
      )
      add_custom_command(
        OUTPUT ${man}.html
        DEPENDS ${man}
        COMMAND ${GROFF_TOOL} -mandoc -Thtml < ${CMAKE_CURRENT_SOURCE_DIR}/${man} > ${man}.html
        COMMENT "Createing HTML manual page ${man}.html..."
      )

      string(REGEX REPLACE ^.*\([1-9]\)\$ \\1 sec ${man})
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${man}.gz
        DESTINATION ${MAN_INSTALL_DIR}/man${sec}
        OPTIONAL
      )
      install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${man}.html
        DESTINATION ${DOC_INSTALL_DIR}/man${sec}
        OPTIONAL
      )

      add_custom_target(man_${man} DEPENDS ${man}.gz ${man}.html)
      add_dependencies(man man_${man})
    endforeach(man)
  endif(GROFF_TOOL AND GZIP_TOOL)
endfunction(add_manual_pages)

# Set up which man pages to build and install
add_manual_pages(
  svxlink.1 svxlink.conf.5 remotetrx.1 remotetrx.conf.5 siglevdetcal.1 devcal.1
  svxreflector.1 svxreflector.conf.5 qtel.1 ModuleHelp.conf.5
  ModuleParrot.conf.5 ModuleEchoLink.conf.5 ModuleTclVoiceMail.conf.5
  ModuleDtmfRepeater.conf.5 ModulePropagationMonitor.conf.5
  ModuleSelCallEnc.conf.5 ModuleFrn.conf.5 ModuleTrx.conf.5
)

