#.rst:
# FindOGG
# --------
# Find the ogg library and include directory
#
#  OGG_FOUND         - Set to true if the ogg library is found
#  OGG_INCLUDE_DIRS  - The directory where ogg.h can be found
#  OGG_LIBRARIES     - Libraries to link with to use ogg
#  OGG_VERSION       - Full version string (if available)
#  OGG_VERSION_MAJOR - Major version (if available)
#  OGG_VERSION_MINOR - Minor version (if available)

#=============================================================================
# Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX
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
    "Your project should require at least CMake 2.6 to use FindOGG.cmake")
endif()

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig)
if(CMAKE_VERSION VERSION_LESS 2.8.2)
  pkg_check_modules(PC_OGG ogg)
else()
  pkg_check_modules(PC_OGG QUIET ogg)
endif()

# Try to find the directory where the ogg.h header file is located
find_path(OGG_INCLUDE_DIR
  NAMES ogg.h
  PATHS ${PC_OGG_INCLUDE_DIRS}
  PATH_SUFFIXES ogg
  DOC "OGG include directory"
)

# Try to find the ogg library
find_library(OGG_LIBRARY
  NAMES ogg
  DOC "OGG library path"
  PATHS ${PC_OGG_LIBRARY_DIRS}
)

# Set up version variables
if(PC_OGG_VERSION)
  set(OGG_VERSION ${PC_OGG_VERSION})
  string(REGEX MATCHALL "[0-9]+" _OGG_VERSION_PARTS "${PC_OGG_VERSION}")
  list(GET _OGG_VERSION_PARTS 0 OGG_VERSION_MAJOR)
  list(GET _OGG_VERSION_PARTS 1 OGG_VERSION_MINOR)
endif()

# Handle the QUIETLY and REQUIRED arguments and set OGG_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
if(CMAKE_VERSION VERSION_LESS 2.8.12)
  find_package_handle_standard_args(OPUS
    DEFAULT_MSG
    OGG_LIBRARY OGG_INCLUDE_DIR
  )
else()
  find_package_handle_standard_args(OGG
    FOUND_VAR OGG_FOUND
    REQUIRED_VARS OGG_LIBRARY OGG_INCLUDE_DIR
    VERSION_VAR OGG_VERSION
  )
endif()

if(OGG_FOUND)
  set(OGG_FOUND 1)
  set(OGG_LIBRARIES ${OGG_LIBRARY})
  set(OGG_INCLUDE_DIRS ${OGG_INCLUDE_DIR})
  set(OGG_DEFINITIONS ${PC_OGG_CFLAGS_OTHER})
endif()

mark_as_advanced(OGG_INCLUDE_DIR OGG_LIBRARY)

