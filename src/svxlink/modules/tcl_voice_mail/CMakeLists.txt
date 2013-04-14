# The name of the module without the Module prefix
set(MODNAME TclVoiceMail)

# Generate config file with correct paths
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TclVoiceMail.conf
  ${CMAKE_CURRENT_BINARY_DIR}/TclVoiceMail.conf
  @ONLY
  )

# Install targets
install_mkdir(${SVX_SPOOL_INSTALL_DIR}/voice_mail svxlink.daemon)
install(FILES Module${MODNAME}.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/modules.d
  )
install(FILES ${MODNAME}.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
  )
install(FILES Module${MODNAME}.conf
  DESTINATION ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}.conf
  DESTINATION ${SVX_SYSCONF_INSTALL_DIR}
  )
