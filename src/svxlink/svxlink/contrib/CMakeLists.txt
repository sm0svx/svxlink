# Add a subdirectory containing contributed code
#
#   name - The upper case name of the contrib
#   subdir - The subdirectory where the code is found
#   comment - Comment text for the CMake option
#
macro(add_contrib name subdir comment)
  option(WITH_CONTRIB_${name} ${comment})
  if(${WITH_CONTRIB_${name}})
    message(STATUS "Building experimental contributed code ${subdir}")
    add_subdirectory(${subdir})
  endif(${WITH_CONTRIB_${name}})
endmacro(add_contrib)

add_contrib(ANNOUNCE_LOGIC AnnounceLogic
  "Set to ON to build and install contributed logic core AnnounceLogic")
add_contrib(SIP_LOGIC SipLogic
  "Set to ON to build and install contributed logic core SipLogic")
