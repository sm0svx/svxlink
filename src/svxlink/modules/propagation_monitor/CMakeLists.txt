# The name of the module without the Module prefix
set(MODNAME PropagationMonitor)

# Generate configuration files with correct paths
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ModulePropagationMonitor.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/ModulePropagationMonitor.conf
  @ONLY
  )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/procmailrc.in
  ${CMAKE_CURRENT_BINARY_DIR}/.procmailrc
  @ONLY
  )

# Install targets
install_mkdir(${SVX_SPOOL_INSTALL_DIR}/propagation_monitor ${SVXLINK_USER}:${SVXLINK_GROUP})
install(FILES Module${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/modules.d)
install(FILES ${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/.procmailrc
  ${SVX_SYSCONF_INSTALL_DIR}
  )
