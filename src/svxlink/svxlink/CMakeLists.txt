# Where to put executable files
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Find the popt library
find_package(Popt REQUIRED)
set(LIBS ${LIBS} ${POPT_LIBRARIES})
include_directories(${POPT_INCLUDE_DIRS})
add_definitions(${POPT_DEFINITIONS})

# Find the GSM codec library
find_package(GSM REQUIRED)
set(LIBS ${LIBS} ${GSM_LIBRARY})
include_directories(${GSM_INCLUDE_DIR})

# Find the TCL library
find_package(TCL)
if(NOT TCL_FOUND)
  message("--- TCL not found")
  return(1)
endif(NOT TCL_FOUND)
set(LIBS ${LIBS} ${TCL_LIBRARY})
include_directories(${TCL_INCLUDE_PATH})

# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Find the dl library
find_package(DL REQUIRED)
set(LIBS ${LIBS} ${DL_LIBRARIES})
include_directories(${DL_INCLUDES})

# Add project libraries
set(LIBS ${LIBS} trx locationinfo asynccpp asyncaudio asynccore)

# Build the executable
add_executable(svxlink
  MsgHandler.cpp Module.cpp Logic.cpp SimplexLogic.cpp
  RepeaterLogic.cpp EventHandler.cpp AudioSwitchMatrix.cpp
  CmdParser.cpp QsoRecorder.cpp svxlink.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/version/SVXLINK.h
)
target_link_libraries(svxlink ${LIBS})
set_target_properties(svxlink PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${RUNTIME_OUTPUT_DIRECTORY}
)
add_version_target(SVXLINK ${VER_SVXLINK})

