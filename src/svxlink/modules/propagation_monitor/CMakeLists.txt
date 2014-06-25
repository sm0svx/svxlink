# The name of the module without the Module prefix
set(MODNAME PropagationMonitor)

# Install targets
install_mkdir(${SVX_SPOOL_INSTALL_DIR}/propagation_monitor svxlink.daemon)
install(FILES Module${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/modules.d)
install(FILES ${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d)
install_if_not_exists(Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
install_if_not_exists(.procmailrc
  ${SVX_SYSCONF_INSTALL_DIR}
  )
