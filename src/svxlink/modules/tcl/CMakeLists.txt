# The name of the module without the Module prefix
set(MODNAME Tcl)

# Module source code
#set(MODSRC xyz.cpp)

# Project libraries to link to
#set(LIBS ${LIBS} echolib)

# Build the plugin
add_library(Module${MODNAME} MODULE Module${MODNAME}.cpp ${MODSRC})
set_target_properties(Module${MODNAME} PROPERTIES PREFIX "")
set_property(TARGET Module${MODNAME} PROPERTY NO_SONAME 1)
target_link_libraries(Module${MODNAME} ${LIBS})

# Install targets
install(
  TARGETS Module${MODNAME}
  DESTINATION ${SVX_MODULE_INSTALL_DIR}
)
install(
  FILES TclModule.tcl ${MODNAME}.tcl.example
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
)
install(
  FILES Module${MODNAME}.tcl.example
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/modules.d
)
