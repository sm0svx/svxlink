# - Try to find libsigc++ >= 2
# Once done this will define
#  SIGC_FOUND         - System has libsigc++
#  SIGC_INCLUDE_DIRS  - The libsigc++ include directories
#  SIGC_LIBRARIES     - The libraries needed to use libsigc++
#  SIGC_DEFINITIONS   - Compiler defines required for using libsigc++
#  SIGC_CXX_FLAGS     - Required C++ specific compiler switches

find_package(PkgConfig)
pkg_search_module(PC_SIGC sigc++-3.0 sigc++-2.0)
if(PC_SIGC_VERSION)
  #set(SIGC_DEFINITIONS ${PC_SIGC_CFLAGS_OTHER})

  if(${PC_SIGC_VERSION} VERSION_GREATER "3.0.0")
    set(SIGC_CXX_FLAGS "--std=c++17")
  elseif(${PC_SIGC_VERSION} VERSION_GREATER "2.5.0")
    set(SIGC_CXX_FLAGS "--std=c++11")
  endif()

  find_path(SIGC_CONFIG_INCLUDE_DIR sigc++config.h
    HINTS ${PC_SIGC_INCLUDEDIR} ${PC_SIGC_INCLUDE_DIRS}
    )
  set(SIGC_INCLUDE_DIRS ${SIGC_INCLUDE_DIRS} ${SIGC_CONFIG_INCLUDE_DIR})

  find_path(SIGC_INCLUDE_DIR sigc++/sigc++.h
    HINTS ${PC_SIGC_INCLUDEDIR} ${PC_SIGC_INCLUDE_DIRS}
    )
  set(SIGC_INCLUDE_DIRS ${SIGC_INCLUDE_DIRS} ${SIGC_INCLUDE_DIR})

  #find_library(SIGC_LIBRARY NAMES sigc-3.0
  #  HINTS ${PC_SIGC_LIBDIR} ${PC_SIGC_LIBRARY_DIRS}
  #  )
  set(SIGC_LIBRARY ${PC_SIGC_LIBRARIES})
  set(SIGC_LIBRARIES ${SIGC_LIBRARY})
endif(PC_SIGC_VERSION)

# handle the QUIETLY and REQUIRED arguments and set SIGC_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SIGC DEFAULT_MSG
                                  SIGC_LIBRARY SIGC_INCLUDE_DIR)

mark_as_advanced(SIGC_CONFIG_INCLUDE_DIR SIGC_INCLUDE_DIR SIGC_LIBRARY)
