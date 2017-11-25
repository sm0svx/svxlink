# The name of the module without the Module prefix
set(MODNAME TclSSTV)

# Install targets
install(FILES go_sstv.py DESTINATION ${BIN_INSTALL_DIR})
install(FILES Module${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/modules.d)
install_if_not_exists(Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
