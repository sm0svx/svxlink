option(WITH_CONTRIB_SIP_LOGIC
  "Set to ON to build and install contributed logic core SipLogic")
if(WITH_CONTRIB_SIP_LOGIC)
  message(STATUS "Building experimental logic core SipLogic")
  add_subdirectory(SipLogic)
endif(WITH_CONTRIB_SIP_LOGIC)

# Propagate variables upwards
set(LIBS ${LIBS} PARENT_SCOPE)
set(SVXLINK_SRCS ${SVXLINK_SRCS} PARENT_SCOPE)
set(SVXLINK_TCL_EVENT_FILES ${SVXLINK_TCL_EVENT_FILES} PARENT_SCOPE)

