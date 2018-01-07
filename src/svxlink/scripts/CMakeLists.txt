# Install targets
install_mkdir(${SVX_SPOOL_INSTALL_DIR} ${SVXLINK_USER}:${SVXLINK_GROUP})
install(FILES etc/logrotate.d/svxlink etc/logrotate.d/remotetrx
  DESTINATION ${SYSCONF_INSTALL_DIR}/logrotate.d
  )
install(PROGRAMS etc/init.d/svxlink etc/init.d/remotetrx
  DESTINATION ${INIT_D_INSTALL_DIR}
  )
install(FILES etc/sysconfig/svxlink etc/sysconfig/remotetrx
  DESTINATION ${SYSCONF_INSTALL_DIR}/sysconfig
  )
install(FILES etc/10-svxlink.rules DESTINATION ${SYSCONF_INSTALL_DIR}/udev/rules.d)
install(FILES etc/90-svxlink.perms
  DESTINATION ${SYSCONF_INSTALL_DIR}/security/console.perms.d
  )
