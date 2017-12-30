configure_file(svxserver.service.in
               ${PROJECT_BINARY_DIR}/systemd/svxserver.service)
#message(STATUS "Configured systemd unit file:svxserver.service")
configure_file(svxserver.default.in
               ${PROJECT_BINARY_DIR}/systemd/svxserver)
#message(STATUS "Configured systemd unit file:remotetrx")

install(FILES ${PROJECT_BINARY_DIR}/systemd/svxserver.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})
install(FILES ${PROJECT_BINARY_DIR}/systemd/svxserver
        DESTINATION ${SYSTEMD_DEFAULTS_FILES_DIR})
