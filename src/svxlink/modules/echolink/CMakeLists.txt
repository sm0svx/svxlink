# The name of the module without the Module prefix
set(MODNAME EchoLink)

# Module source code
set(MODSRC QsoImpl.cpp)

# Project libraries to link to
set(LIBS ${LIBS} echolib)

# Find the TCL library
find_package(TCL)
if(NOT TCL_FOUND)
  message(FATAL_ERROR "TCL not found")
endif(NOT TCL_FOUND)
set(LIBS ${LIBS} ${TCL_LIBRARY})
include_directories(${TCL_INCLUDE_PATH})

# Find the GSM codec library and include directory
find_package(GSM REQUIRED)
if(NOT GSM_FOUND)
  message(FATAL_ERROR "libgsm not found")
endif(NOT GSM_FOUND)
include_directories(${GSM_INCLUDE_DIR})
set(LIBS ${LIBS} ${GSM_LIBRARY})

string(TOUPPER MODULE_${MODNAME} VERNAME)

# Add targets for version files
set(VERSION_DEPENDS)
add_version_target(${VERNAME} VERSION_DEPENDS)
add_version_target(SVXLINK VERSION_DEPENDS)

# Build the plugin
add_library(Module${MODNAME} MODULE Module${MODNAME}.cpp ${MODSRC}
  ${VERSION_DEPENDS}
)
set_target_properties(Module${MODNAME} PROPERTIES PREFIX "")
target_link_libraries(Module${MODNAME} ${LIBS})

# Install targets
install(TARGETS Module${MODNAME} DESTINATION ${SVX_MODULE_INSTALL_DIR})
install(FILES ${MODNAME}.tcl DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d)
install_if_not_exists(Module${MODNAME}.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
  )
