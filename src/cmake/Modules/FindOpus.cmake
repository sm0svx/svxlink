#.rst:
# FindOpus
# --------
# Find the opus library and include directory
#
#  Opus_FOUND         - Set to true if the opus library is found
#  Opus_INCLUDE_DIRS  - The directory where opus.h can be found
#  Opus_LIBRARIES     - Libraries to link with to use opus
#  Opus_VERSION       - Full version string (if available)
#  Opus_VERSION_MAJOR - Major version (if available)
#  Opus_VERSION_MINOR - Minor version (if available)

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
    "Your project should require at least CMake 2.6 to use FindOpus.cmake")
endif()

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
find_package(PkgConfig)
if(CMAKE_VERSION VERSION_LESS 2.8.2)
  pkg_check_modules(PC_Opus opus)
else()
  pkg_check_modules(PC_Opus QUIET opus)
endif()

# Try to find the directory where the opus.h header file is located
find_path(Opus_INCLUDE_DIR
  NAMES opus.h
  PATHS ${PC_Opus_INCLUDE_DIRS}
  PATH_SUFFIXES opus
  DOC "Opus include directory"
)

# Try to find the opus library
find_library(Opus_LIBRARY
  NAMES opus
  DOC "Opus library path"
  PATHS ${PC_Opus_LIBRARY_DIRS}
)

# Set up version variables
if(PC_Opus_VERSION)
  set(Opus_VERSION ${PC_Opus_VERSION})
  string(REGEX MATCHALL "[0-9]+" _Opus_VERSION_PARTS "${PC_Opus_VERSION}")
  list(GET _Opus_VERSION_PARTS 0 Opus_VERSION_MAJOR)
  list(GET _Opus_VERSION_PARTS 1 Opus_VERSION_MINOR)
endif()

# Handle the QUIETLY and REQUIRED arguments and set Opus_FOUND to TRUE if 
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
if(CMAKE_VERSION VERSION_LESS 2.8.12)
  find_package_handle_standard_args(OPUS
    DEFAULT_MSG
    Opus_LIBRARY Opus_INCLUDE_DIR
  )
else()
  find_package_handle_standard_args(Opus
    FOUND_VAR OPUS_FOUND
    REQUIRED_VARS Opus_LIBRARY Opus_INCLUDE_DIR
    VERSION_VAR Opus_VERSION
  )
endif()

if(OPUS_FOUND)
  set(Opus_FOUND 1)
  set(Opus_LIBRARIES ${Opus_LIBRARY})
  set(Opus_INCLUDE_DIRS ${Opus_INCLUDE_DIR})
  set(Opus_DEFINITIONS ${PC_Opus_CFLAGS_OTHER})
endif()

mark_as_advanced(Opus_INCLUDE_DIR Opus_LIBRARY)

