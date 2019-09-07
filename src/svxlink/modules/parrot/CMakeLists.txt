# The name of the module without the Module prefix
set(MODNAME Parrot)

# Module source code
#set(MODSRC xyz.cpp)

# Project libraries to link to
#set(LIBS ${LIBS} echolib)

# Build the plugin
add_library(Module${MODNAME} MODULE Module${MODNAME}.cpp ${MODSRC})
set_target_properties(Module${MODNAME} PROPERTIES PREFIX "")
set_property(TARGET Module${MODNAME} PROPERTY NO_SONAME 1)
target_link_libraries(Module${MODNAME} ${LIBS})

# Deduce the version h-file target name from the module name and make the
# module depend on that target
string(REGEX REPLACE \(.\)\([A-Z]\) \\1-\\2 VERSION_TARGET_NAME ${MODNAME})
string(TOLOWER version-module-${VERSION_TARGET_NAME} VERSION_TARGET_NAME)
add_dependencies(Module${MODNAME} version-svxlink ${VERSION_TARGET_NAME})

# Install targets
install(TARGETS Module${MODNAME} DESTINATION ${SVX_MODULE_INSTALL_DIR})
install(FILES ${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d)
install_if_not_exists(Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
