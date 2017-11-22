option(WITH_CONTRIB_MODULE_A2D_MCP320X
  "Set to ON to build and install contributed module A2D_MCP320X")
if(WITH_CONTRIB_MODULE_A2D_MCP320X)
  message(STATUS "Building experimental module A2D_MCP320X")
  add_subdirectory(module_a2d_mcp320X)
endif(WITH_CONTRIB_MODULE_A2D_MCP320X)

option(WITH_CONTRIB_MODULE_REMOTE_RELAY
  "Set to ON to build and install contributed module RemoteRelay")
if(WITH_CONTRIB_MODULE_REMOTE_RELAY)
  message(STATUS "Building experimental module RemoteRelay")
  add_subdirectory(module_remote_relay)
endif(WITH_CONTRIB_MODULE_REMOTE_RELAY)

option(WITH_CONTRIB_MODULE_SITE_STATUS
  "Set to ON to build and install contributed module SITE_STATUS")
if(WITH_CONTRIB_MODULE_SITE_STATUS)
  message(STATUS "Building experimental module SITE_STATUS")
  add_subdirectory(module_site_status)
endif(WITH_CONTRIB_MODULE_SITE_STATUS)

option(WITH_CONTRIB_MODULE_SVXLINKCFG
  "Set to ON to build and install contributed module SVXLINKCFG")
if(WITH_CONTRIB_MODULE_SVXLINKCFG)
  message(STATUS "Building experimental module SVXLINKCFG")
  add_subdirectory(module_svxlinkcfg)
endif(WITH_CONTRIB_MODULE_SVXLINKCFG)

option(WITH_CONTRIB_MODULE_TCL_SSTV
  "Set to ON to build and install contributed module TclSSTV")
if(WITH_CONTRIB_MODULE_TCL_SSTV)
  message(STATUS "Building experimental module TclSSTV")
  add_subdirectory(module_tclsstv)
endif(WITH_CONTRIB_MODULE_TCL_SSTV)
