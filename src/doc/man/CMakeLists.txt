# Set up which man pages to build and install
set(MAN_PAGES svxlink.1 svxlink.conf.5 remotetrx.1 remotetrx.conf.5
  siglevdetcal.1 devcal.1 svxreflector.1 svxreflector.conf.5 qtel.1
  ModuleHelp.conf.5 ModuleParrot.conf.5 ModuleEchoLink.conf.5
  ModuleTclVoiceMail.conf.5 ModuleDtmfRepeater.conf.5
  ModulePropagationMonitor.conf.5 ModuleSelCallEnc.conf.5
  ModuleFrn.conf.5 ModuleTrx.conf.5
)

# Search for the gzip and groff programs. Error out if not found.
include(FindGZIP)
include(FindGROFF)

# Set up a target for compressing each manual page. Also add an install
# target for it.
if(GROFF_TOOL AND GZIP_TOOL)
  set(TARGETS)
  foreach(man ${MAN_PAGES})
    add_custom_command(OUTPUT ${man}.gz
      DEPENDS ${man}
      COMMAND ${GZIP_TOOL} -c ${CMAKE_CURRENT_SOURCE_DIR}/${man} > ${man}.gz
      COMMENT "Compressing manual page ${man}.gz..."
    )
    add_custom_command(OUTPUT ${man}.html
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
      DESTINATION ${DOC_INSTALL_DIR}/man/man${sec}
      OPTIONAL
    )
    set(TARGETS ${TARGETS} ${man}.gz ${man}.html)
  endforeach(man)

  # Add a custom target to trigger the build of the man pages
  add_custom_target(man DEPENDS ${TARGETS})
  add_dependencies(doc man)
else(GROFF_TOOL AND GZIP_TOOL)
  message("--   The groff and gzip tools are optional dependencies.")
  message("--   They are used to build the UNIX manual pages. SvxLink will")
  message("--   build without them but usage documentation will be missing.")
endif(GROFF_TOOL AND GZIP_TOOL)
