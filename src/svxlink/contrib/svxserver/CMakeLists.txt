# Find the popt library
find_package(Popt REQUIRED)
set(LIBS ${LIBS} ${POPT_LIBRARIES})
include_directories(${POPT_INCLUDE_DIRS})
add_definitions(${POPT_DEFINITIONS})

# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Add project libraries
set(LIBS ${LIBS} trx asynccpp asyncaudio asynccore)

# Add targets for version files
set(VERSION_DEPENDS)
add_version_target(SVXSERVER VERSION_DEPENDS)

# Build the executable
add_executable(svxserver
  svxserver.cpp
  server.cpp
  ${VERSION_DEPENDS}
)
target_link_libraries(svxserver ${LIBS})
set_target_properties(svxserver PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

configure_file(${SYSTEMD_SOURCE_DIR}/svxserver.service.in
               ${PROJECT_BINARY_DIR}/systemd/svxserver.service)
#message(STATUS "Configured systemd unit file:svxserver.service")

# Install targets
install(FILES ${PROJECT_BINARY_DIR}/svxserver.service
        DESTINATION ${SYSTEMD_CONFIGURATIONS_FILES_DIR})

install(TARGETS svxserver DESTINATION ${BIN_INSTALL_DIR})
install_if_not_exists(svxserver.conf ${SVX_SYSCONF_INSTALL_DIR})

