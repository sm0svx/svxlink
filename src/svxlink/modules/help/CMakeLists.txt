# The name of the module without the Module prefix
set(MODNAME Help)

# Module source code
#set(MODSRC xyz.cpp)

# Project libraries to link to
#set(LIBS ${LIBS} echolib)

# The version tag name without the VER_ prefix
set(VERNAME ${MODNAME})
string(REGEX REPLACE \(.\)\([A-Z]\) \\1_\\2 VERNAME ${VERNAME})
string(TOUPPER MODULE_${VERNAME} VERNAME)

# Build the plugin
add_library(Module${MODNAME} MODULE Module${MODNAME}.cpp ${MODSRC}
  ${CMAKE_CURRENT_BINARY_DIR}/version/${VERNAME}.h
  ${CMAKE_CURRENT_BINARY_DIR}/version/SVXLINK.h
)
set_target_properties(Module${MODNAME} PROPERTIES PREFIX "")
target_link_libraries(Module${MODNAME} ${LIBS})
add_version_target(${VERNAME} ${VER_${VERNAME}})
add_version_target(SVXLINK ${VER_SVXLINK})
