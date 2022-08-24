# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Add project libraries
set(LIBS ${LIBS} trx asynccpp asyncaudio asynccore svxmisc)

# Build the executable
add_executable(siglevdetcal siglevdetcal.cpp)
target_link_libraries(siglevdetcal ${LIBS})
set_target_properties(siglevdetcal PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)

# Install targets
install(TARGETS siglevdetcal DESTINATION ${BIN_INSTALL_DIR})
