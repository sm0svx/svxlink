#.rst:
# FindGPIOD
# --------
# Find the GPIOD library and include directory
#
#  GPIOD_FOUND         - Set to true if the gpiod library is found
#  GPIOD_INCLUDE_DIRS  - The directory where gpiod.h can be found
#  GPIOD_LIBRARIES     - Libraries to link with to use gpiod
#  GPIOD_VERSION       - Full version string (if available)
#  GPIOD_VERSION_MAJOR - Major version (if available)
#  GPIOD_VERSION_MINOR - Minor version (if available)

#=============================================================================
# Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#=============================================================================

if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.6)
  message(AUTHOR_WARNING
    "Your project should require at least CMake 2.6 to use FindGPIOD.cmake")
endif()

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig)
if(CMAKE_VERSION VERSION_LESS 2.8.2)
  pkg_check_modules(PC_GPIOD libgpiod)
else()
  pkg_check_modules(PC_GPIOD QUIET libgpiod)
endif()

# Try to find the directory where the gpiod.h header file is located
find_path(GPIOD_INCLUDE_DIR
  NAMES gpiod.h
  PATHS ${PC_GPIOD_INCLUDE_DIRS}
  #PATH_SUFFIXES gpiod
  DOC "GPIOD include directory"
)

# Try to find the GPIOD library
find_library(GPIOD_LIBRARY
  NAMES gpiod
  DOC "GPIOD library path"
  PATHS ${PC_GPIOD_LIBRARY_DIRS}
)

# Set up version variables
if(PC_GPIOD_VERSION)
  set(GPIOD_VERSION ${PC_GPIOD_VERSION})
  string(REGEX MATCHALL "[0-9]+" _GPIOD_VERSION_PARTS "${PC_GPIOD_VERSION}")
  list(GET _GPIOD_VERSION_PARTS 0 GPIOD_VERSION_MAJOR)
  list(GET _GPIOD_VERSION_PARTS 1 GPIOD_VERSION_MINOR)
endif()

# Handle the QUIETLY and REQUIRED arguments and set GPIOD_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
if(CMAKE_VERSION VERSION_LESS 2.8.12)
  find_package_handle_standard_args(GPIOD
    DEFAULT_MSG
    GPIOD_LIBRARY GPIOD_INCLUDE_DIR
  )
else()
  find_package_handle_standard_args(GPIOD
    FOUND_VAR GPIOD_FOUND
    REQUIRED_VARS GPIOD_LIBRARY GPIOD_INCLUDE_DIR
    VERSION_VAR GPIOD_VERSION
  )
endif()

if(GPIOD_FOUND)
  set(GPIOD_FOUND 1)
  set(GPIOD_LIBRARIES ${GPIOD_LIBRARY})
  set(GPIOD_INCLUDE_DIRS ${GPIOD_INCLUDE_DIR})
  set(GPIOD_DEFINITIONS ${PC_GPIOD_CFLAGS_OTHER}
      -DGPIOD_VERSION_MAJOR=${GPIOD_VERSION_MAJOR}
      -DGPIOD_VERSION_MINOR=${GPIOD_VERSION_MINOR})
endif()

mark_as_advanced(GPIOD_INCLUDE_DIR GPIOD_LIBRARY)

