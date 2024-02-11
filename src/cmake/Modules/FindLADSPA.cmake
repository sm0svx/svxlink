#.rst:
# FindLADSPA
# --------
# Find the LADSPA include file
#
#  LADSPA_FOUND         - Set to true if the ladspa include file is found
#  LADSPA_INCLUDE_DIRS  - The directory where ladspa.h can be found
#  LADSPA_PLUGIN_DIRS   - The directories where LADSPA plugins can be found
#  LADSPA_VERSION       - Full version string (if available)
#  LADSPA_VERSION_MAJOR - Major version (if available)
#  LADSPA_VERSION_MINOR - Minor version (if available)

#=============================================================================
# Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX
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

if(CMAKE_MINIMUM_REQUIRED_VERSION VERSION_LESS 2.8.12)
  message(AUTHOR_WARNING
    "Your project should require at least CMake 2.6 to use FindLADSPA.cmake")
endif()

# Try to find the directory where the ladspa.h header file is located
find_path(LADSPA_INCLUDE_DIR
  NAMES ladspa.h
  DOC "LADSPA include directory"
)

# Set up version variables if the include file was found
if (LADSPA_INCLUDE_DIR)
  file(READ "${LADSPA_INCLUDE_DIR}/ladspa.h" ladspa_h)
  string(REGEX MATCH "#define LADSPA_VERSION \"([^ ]+)\"" _ ${ladspa_h})
  set(LADSPA_VERSION ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define LADSPA_VERSION_MAJOR ([0-9]+)" _ ${ladspa_h})
  set(LADSPA_VERSION_MAJOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define LADSPA_VERSION_MINOR ([0-9]+)" _ ${ladspa_h})
  set(LADSPA_VERSION_MINOR ${CMAKE_MATCH_1})
endif (LADSPA_INCLUDE_DIR)

# Find the LADSPA plugin directory
include(GNUInstallDirs)
find_file(LADSPA_PLUGIN_DIR
  NAMES ladspa
  DOC "LADSPA plugin directory"
  PATHS /usr/${CMAKE_INSTALL_LIBDIR} /usr/lib
  NO_DEFAULT_PATH
)

# Handle the version, QUIETLY and REQUIRED arguments and set LADSPA_FOUND to
# TRUE if all required variables are available
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LADSPA
  FOUND_VAR LADSPA_FOUND
  REQUIRED_VARS LADSPA_INCLUDE_DIR LADSPA_PLUGIN_DIR
  VERSION_VAR LADSPA_VERSION
)

# Set up other standard variables if LADSPA was found
if(LADSPA_FOUND)
  set(LADSPA_INCLUDE_DIRS ${LADSPA_INCLUDE_DIR})
  set(LADSPA_PLUGIN_DIRS "${LADSPA_PLUGIN_DIR}")
  set(LADSPA_DEFINITIONS
      -DLADSPA_PLUGIN_DIRS="${LADSPA_PLUGIN_DIRS}"
      -DLADSPA_VERSION=${LADSPA_VERSION}
      -DLADSPA_VERSION_MAJOR=${LADSPA_VERSION_MAJOR}
      -DLADSPA_VERSION_MINOR=${LADSPA_VERSION_MINOR})
endif(LADSPA_FOUND)

# Hide these variables for normal usage
mark_as_advanced(LADSPA_INCLUDE_DIR LADSPA_PLUGIN_DIR)

