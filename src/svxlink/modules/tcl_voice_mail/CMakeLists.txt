# The name of the module without the Module prefix
set(MODNAME TclVoiceMail)

# Generate config file with correct paths
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TclVoiceMail.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/TclVoiceMail.conf
  @ONLY
  )

# Generate TCL module with correct paths
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModuleTclVoiceMail.tcl.in
  ${CMAKE_CURRENT_BINARY_DIR}/ModuleTclVoiceMail.tcl
  @ONLY
  )

# Install targets
install_mkdir(${SVX_SPOOL_INSTALL_DIR}/voice_mail ${SVXLINK_USER}:${SVXLINK_GROUP})
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/Module${MODNAME}.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/modules.d
  )
install(FILES ${MODNAME}.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
  )
install_if_not_exists(Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}
  )
