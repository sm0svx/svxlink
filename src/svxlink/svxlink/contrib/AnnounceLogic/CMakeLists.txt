# Find the pjproject libraries and include files
# Set up the build environment for the plugin
###add_manual_pages(AnnounceLogic.conf.5)

# Build the plugin
add_library(AnnounceLogic MODULE AnnounceLogic.cpp)
set_target_properties(AnnounceLogic PROPERTIES PREFIX "")
set_property(TARGET AnnounceLogic PROPERTY NO_SONAME 1)
#target_link_libraries(AnnounceLogic ${LIBS})


# Install plugin, TCL event handler and config file
install(TARGETS AnnounceLogic DESTINATION ${SVX_LOGIC_CORE_INSTALL_DIR})
install(
  FILES ${CMAKE_CURRENT_SOURCE_DIR}/AnnounceLogicType.tcl
        ${CMAKE_CURRENT_SOURCE_DIR}/AnnounceLogic.tcl
  DESTINATION ${SVX_SHARE_INSTALL_DIR}/events.d
)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/AnnounceLogic.conf.in
  ${CMAKE_CURRENT_BINARY_DIR}/AnnounceLogic.conf
  @ONLY
)
install_if_not_exists(${CMAKE_CURRENT_BINARY_DIR}/AnnounceLogic.conf
  ${SVX_SYSCONF_INSTALL_DIR}/svxlink.d
)
install_if_not_exists(Announcement_remote_nodes.tcl ${SVX_SHARE_INSTALL_DIR}/events.d)
