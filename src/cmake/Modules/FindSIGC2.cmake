# - Try to find libsigc++-2
# Once done this will define
#  SIGC2_FOUND        - System has libsigc++-2
#  SIGC2_INCLUDE_DIRS - The libsigc++-2 include directories
#  SIGC2_LIBRARIES    - The libraries needed to use libsigc++-2
#  SIGC2_DEFINITIONS  - Compiler switches required for using libsigc++-2
#  SIGC_CXX_FLAGS     - Required C++ specific compiler switches

find_package(PkgConfig)
pkg_check_modules(PC_SIGC2 sigc++-2.0)
set(SIGC2_DEFINITIONS ${PC_SIGC2_CFLAGS_OTHER})

if(${PC_SIGC2_VERSION} VERSION_GREATER "2.5.0")
  set(SIGC2_CXX_FLAGS "--std=c++11")
endif()

find_path(SIGC2_CONFIG_INCLUDE_DIR sigc++config.h
  HINTS ${PC_SIGC2_INCLUDEDIR} ${PC_SIGC2_INCLUDE_DIRS}
  )
set(SIGC2_INCLUDE_DIRS ${SIGC2_INCLUDE_DIRS} ${SIGC2_CONFIG_INCLUDE_DIR})

find_path(SIGC2_INCLUDE_DIR sigc++/sigc++.h
  HINTS ${PC_SIGC2_INCLUDEDIR} ${PC_SIGC2_INCLUDE_DIRS}
  )
set(SIGC2_INCLUDE_DIRS ${SIGC2_INCLUDE_DIRS} ${SIGC2_INCLUDE_DIR})

find_library(SIGC2_LIBRARY NAMES sigc-2.0
  HINTS ${PC_SIGC2_LIBDIR} ${PC_SIGC2_LIBRARY_DIRS}
  )
set(SIGC2_LIBRARIES ${SIGC2_LIBRARY})


# handle the QUIETLY and REQUIRED arguments and set SIGC2_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SIGC2 DEFAULT_MSG
                                  SIGC2_LIBRARY SIGC2_INCLUDE_DIR)

mark_as_advanced(SIGC2_CONFIG_INCLUDE_DIR SIGC2_INCLUDE_DIR SIGC2_LIBRARY)

