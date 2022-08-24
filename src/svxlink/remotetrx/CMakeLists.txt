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
set(LIBS ${LIBS} trx asynccpp asyncaudio asynccore svxmisc)

# Build the executable
add_executable(remotetrx
  TrxHandler.cpp Uplink.cpp NetUplink.cpp RfUplink.cpp
  NetTrxAdapter.cpp remotetrx.cpp
)
target_link_libraries(remotetrx ${LIBS})
set_target_properties(remotetrx PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

# Install targets
install(TARGETS remotetrx DESTINATION ${BIN_INSTALL_DIR})
install_if_not_exists(remotetrx.conf ${SVX_SYSCONF_INSTALL_DIR})
