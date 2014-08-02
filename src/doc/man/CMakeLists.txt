# Set up which man pages to build and install
set(MAN_PAGES svxlink.1 remotetrx.1 siglevdetcal.1 svxlink.conf.5
  ModuleHelp.conf.5 remotetrx.conf.5 ModuleParrot.conf.5 ModuleEchoLink.conf.5
  ModuleTclVoiceMail.conf.5 ModuleDtmfRepeater.conf.5
  ModulePropagationMonitor.conf.5 ModuleSelCallEnc.conf.5 qtel.1
)

# Search for gzip program. Error out if not found.
include(FindGZIP)

# Set up a target for compressing each manual page. Also add an install
# target for it.
set(TARGETS)
foreach(man ${MAN_PAGES})
  add_custom_command(OUTPUT ${man}.gz
    DEPENDS ${man}
    COMMAND ${GZIP_TOOL} -c ${CMAKE_CURRENT_SOURCE_DIR}/${man} > ${man}.gz
    COMMENT "Compressing maual page ${man}.gz..."
  )
  string(REGEX REPLACE ^.*\([1-9]\)\$ \\1 sec ${man})
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${man}.gz
    DESTINATION ${MAN_INSTALL_DIR}/man${sec}
    OPTIONAL
  )
  set(TARGETS ${TARGETS} ${man}.gz)
endforeach(man)

# Add a custom target to trigger the build of the man pages
add_custom_target(man DEPENDS ${TARGETS})
add_dependencies(doc man)
