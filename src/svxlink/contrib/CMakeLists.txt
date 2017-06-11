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

