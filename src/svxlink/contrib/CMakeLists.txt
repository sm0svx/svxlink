option(WITH_CONTRIB_MODULE_REMOTE_RELAY
  "Set to ON to build and install contributed module RemoteRelay")
if(WITH_CONTRIB_MODULE_REMOTE_RELAY)
  message(STATUS "Building experimental module RemoteRelay")
  add_subdirectory(module_remote_relay)
endif(WITH_CONTRIB_MODULE_REMOTE_RELAY)

option(WITH_CONTRIB_MODULE_TCL_SSTV
  "Set to ON to build and install contributed module TclSSTV")
if(WITH_CONTRIB_MODULE_TCL_SSTV)
  message(STATUS "Building experimental module TclSSTV")
  add_subdirectory(module_tclsstv)
endif(WITH_CONTRIB_MODULE_TCL_SSTV)

option(WITH_CONTRIB_SVXSERVER
  "Set to ON to build and install contributed application svxserver")
if(WITH_CONTRIB_SVXSERVER)
  message(STATUS "Building experimental application svxserver")
  add_subdirectory(svxserver)
endif(WITH_CONTRIB_SVXSERVER)

option(WITH_CONTRIB_MODULE_SITE_STATUS
  "Set to ON to build and install contributed module SITE_STATUS")
if(WITH_CONTRIB_MODULE_SITE_STATUS)
  message(STATUS "Building experimental module SITE_STATUS")
  add_subdirectory(module_site_status)
endif(WITH_CONTRIB_MODULE_SITE_STATUS)

option(WITH_CONTRIB_MODULE_TXFAN
  "Set to ON to build and install contributed module TXFAN")
if(WITH_CONTRIB_MODULE_TXFAN)
  message(STATUS "Building experimental module TXFAN")
  add_subdirectory(module_txfan)
endif(WITH_CONTRIB_MODULE_TXFAN)

