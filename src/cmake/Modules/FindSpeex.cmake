#.rst:
# FindSpeex
# --------
# Find the speex library and include directory
#
#  Speex_FOUND         - Set to true if the speex library is found
#  Speex_INCLUDE_DIRS  - The directory where speex.h can be found
#  Speex_LIBRARIES     - Libraries to link with to use speex
#  Speex_VERSION       - Full version string (if available)
#  Speex_VERSION_MAJOR - Major version (if available)
#  Speex_VERSION_MINOR - Minor version (if available)

#=============================================================================
# Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX
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
    "Your project should require at least CMake 2.6 to use FindSpeex.cmake")
endif()

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig)
if(CMAKE_VERSION VERSION_LESS 2.8.2)
  pkg_check_modules(PC_Speex speex)
else()
  pkg_check_modules(PC_Speex QUIET speex)
endif()

# Try to find the directory where the speex.h header file is located
find_path(Speex_INCLUDE_DIR
  NAMES speex.h
  PATHS ${PC_Speex_INCLUDE_DIRS}
  PATH_SUFFIXES speex
  DOC "Speex include directory"
)

# Try to find the speex library
find_library(Speex_LIBRARY
  NAMES speex
  DOC "Speex library path"
  PATHS ${PC_Speex_LIBRARY_DIRS}
)

# Set up version variables
if(PC_Speex_VERSION)
  set(Speex_VERSION ${PC_Speex_VERSION})
  string(REGEX MATCHALL "[0-9]+" _Speex_VERSION_PARTS "${PC_Speex_VERSION}")
  list(GET _Speex_VERSION_PARTS 0 Speex_VERSION_MAJOR)
  list(GET _Speex_VERSION_PARTS 1 Speex_VERSION_MINOR)
endif()

# Handle the QUIETLY and REQUIRED arguments and set Speex_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
if(CMAKE_VERSION VERSION_LESS 2.8.12)
  find_package_handle_standard_args(SPEEX
    DEFAULT_MSG
    Speex_LIBRARY Speex_INCLUDE_DIR
  )
else()
  find_package_handle_standard_args(Speex
    FOUND_VAR SPEEX_FOUND
    REQUIRED_VARS Speex_LIBRARY Speex_INCLUDE_DIR
    VERSION_VAR Speex_VERSION
  )
endif()

if(SPEEX_FOUND)
  set(Speex_FOUND 1)
  set(Speex_LIBRARIES ${Speex_LIBRARY})
  set(Speex_INCLUDE_DIRS ${Speex_INCLUDE_DIR})
  set(Speex_DEFINITIONS ${PC_Speex_CFLAGS_OTHER})
endif()

mark_as_advanced(Speex_INCLUDE_DIR Speex_LIBRARY)

