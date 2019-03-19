# The name of the module without the Module prefix
set(MODNAME MetarInfo)

# Module source code
#set(MODSRC xyz.cpp)

# Project libraries to link to
set(LIBS ${LIBS} ${CURL_LIBRARY})

# The version tag name without the VER_ prefix
set(VERNAME ${MODNAME})
#string(REGEX REPLACE \(.\)\([A-Z]\) \\1_\\2 VERNAME ${VERNAME})
string(TOUPPER MODULE_${VERNAME} VERNAME)

# Add targets for version files
set(VERSION_DEPENDS)
add_version_target(${VERNAME} VERSION_DEPENDS)
add_version_target(SVXLINK VERSION_DEPENDS)

# Build the plugin
add_library(Module${MODNAME} MODULE Module${MODNAME}.cpp ${MODSRC}
  ${VERSION_DEPENDS}
)
set_target_properties(Module${MODNAME} PROPERTIES PREFIX "")
set_property(TARGET Module${MODNAME} PROPERTY NO_SONAME 1)
target_link_libraries(Module${MODNAME} ${LIBS})

# Install targets
install(TARGETS Module${MODNAME} DESTINATION ${SVX_MODULE_INSTALL_DIR})
install(FILES ${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d)
install_if_not_exists(Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
