# Find the pjproject libraries and include files
# Set up the build environment for the plugin
add_manual_pages(TetraLogic.conf.5)

# Build the plugin
add_library(TetraLogic MODULE TetraLogic.cpp DapNetClient.cpp)
set_target_properties(TetraLogic PROPERTIES PREFIX "")
set_property(TARGET TetraLogic PROPERTY NO_SONAME 1)
target_link_libraries(TetraLogic ${LIBS})

# Install plugin, TCL event handler and config file
install(TARGETS TetraLogic DESTINATION ${SVX_LOGIC_CORE_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/TetraLogic.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/TetraLogic.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/TetraLogic.conf
  @ONLY
)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/TetraLogic.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
)
install_if_not_exists(tetra_users.json ${SVX_SYSCONF_INSTALL_DIR})
install_if_not_exists(pei-init.json ${SVX_SYSCONF_INSTALL_DIR})
