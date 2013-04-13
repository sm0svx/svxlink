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

# Build the executable
add_executable(remotetrx
  TrxHandler.cpp Uplink.cpp NetUplink.cpp RfUplink.cpp
  NetTrxAdapter.cpp remotetrx.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version/REMOTE_TRX.h
)
target_link_libraries(remotetrx ${LIBS})
set_target_properties(remotetrx PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)
add_version_target(REMOTE_TRX)

# Install targets
install(TARGETS remotetrx DESTINATION ${BIN_INSTALL_DIR})
install(FILES remotetrx.conf DESTINATION ${SVX_SYSCONF_INSTALL_DIR})
