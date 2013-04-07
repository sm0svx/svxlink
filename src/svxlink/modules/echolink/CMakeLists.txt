# The name of the module without the Module prefix
set(MODNAME EchoLink)

# Module source code
set(MODSRC QsoImpl.cpp)

# Project libraries to link to
set(LIBS ${LIBS} echolib)

# Find the TCL library
find_package(TCL)
if(NOT TCL_FOUND)
  message("--- TCL not found")
  return(1)
endif(NOT TCL_FOUND)
set(LIBS ${LIBS} ${TCL_LIBRARY})
include_directories(${TCL_INCLUDE_PATH})


string(TOUPPER ${MODNAME} VERNAME)

# Build the plugin
add_library(Module${MODNAME} MODULE Module${MODNAME}.cpp ${MODSRC}
  ${CMAKE_CURRENT_BINARY_DIR}/version/MODULE_${VERNAME}.h
  ${CMAKE_CURRENT_BINARY_DIR}/version/SVXLINK.h
)
set_target_properties(Module${MODNAME} PROPERTIES PREFIX "")
target_link_libraries(Module${MODNAME} ${LIBS})
add_version_target(MODULE_${VERNAME} ${VER_MODULE_${VERNAME}})
add_version_target(SVXLINK ${VER_SVXLINK})
