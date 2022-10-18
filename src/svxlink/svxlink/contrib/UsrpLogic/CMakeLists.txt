message(
  "--   The installing EXPERIMENTAL UsrpLogic extension"
)

add_manual_pages(UsrpLogic.conf.5)

# Build the plugin
add_library(UsrpLogic MODULE UsrpLogic.cpp)
set_target_properties(UsrpLogic PROPERTIES PREFIX "")
set_property(TARGET UsrpLogic PROPERTY NO_SONAME 1)
target_link_libraries(UsrpLogic ${LIBS})

# Install plugin, TCL event handler and config file
install(TARGETS UsrpLogic DESTINATION ${SVX_LOGIC_CORE_INSTALL_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/UsrpLogic.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
)
install_if_not_exists(${CMAKE_CURRENT_SOURCE_DIR}/dv_users.json
  ${SVX_SYSCONF_INSTALL_DIR}
  )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/UsrpLogic.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/UsrpLogic.conf
  @ONLY
)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/UsrpLogic.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
)
