##############################################################################
# SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
# Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX
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
##############################################################################

##############################################################################
# Project setup
##############################################################################
cmake_minimum_required(VERSION 2.6)
project(svxlink C CXX)
#enable_testing()

# The path to the project global include directory
set(PROJECT_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

# Where to put library files
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# Where to put executable files
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Add project local CMake module directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Optional parts
option(USE_QT "Build Qt applications and libs" ON)

# The sample rate used internally in SvxLink
if(NOT DEFINED INTERNAL_SAMPLE_RATE)
  set(INTERNAL_SAMPLE_RATE 16000)
endif(NOT DEFINED INTERNAL_SAMPLE_RATE)
add_definitions(-DINTERNAL_SAMPLE_RATE=${INTERNAL_SAMPLE_RATE})

# Set up include directories
include_directories(
  #${PROJECT_INCLUDE_DIR}
  ${CMAKE_SOURCE_DIR}/misc
  ${CMAKE_SOURCE_DIR}/async/core
  ${CMAKE_SOURCE_DIR}/async/cpp
  ${CMAKE_SOURCE_DIR}/async/audio
  ${CMAKE_SOURCE_DIR}/async/qt
  ${CMAKE_SOURCE_DIR}/echolib
  ${CMAKE_SOURCE_DIR}/locationinfo
  ${CMAKE_BINARY_DIR}
)


##############################################################################
# Install directories setup
##############################################################################

# Where to install include files
if(NOT DEFINED INCLUDE_INSTALL_DIR)
  set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
endif(NOT DEFINED INCLUDE_INSTALL_DIR)

# Where to install libraries
if(NOT DEFINED LIB_INSTALL_DIR)
  set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
endif(NOT DEFINED LIB_INSTALL_DIR)

# The config directory (normally /etc)
if(NOT DEFINED SYSCONF_INSTALL_DIR)
  set(SYSCONF_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/etc)
endif(NOT DEFINED SYSCONF_INSTALL_DIR)

# Architecture independent files directory (normally /usr/share)
if(NOT DEFINED SHARE_INSTALL_PREFIX)
  set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share)
endif(NOT DEFINED SHARE_INSTALL_PREFIX)

# Local state directory (normally /var)
if(NOT DEFINED LOCAL_STATE_DIR)
  set(LOCAL_STATE_DIR ${CMAKE_INSTALL_PREFIX}/var)
endif(NOT DEFINED LOCAL_STATE_DIR)

# Where to install executables
if(NOT DEFINED BIN_INSTALL_DIR)
  set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
endif(NOT DEFINED BIN_INSTALL_DIR)

# Where to install manual pages
if(NOT DEFINED MAN_INSTALL_DIR)
  set(MAN_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/man)
endif(NOT DEFINED MAN_INSTALL_DIR)

# Where to install startup scripts
if(NOT DEFINED INIT_D_INSTALL_DIR)
  set(INIT_D_INSTALL_DIR ${SYSCONF_INSTALL_DIR}/init.d)
endif(NOT DEFINED INIT_D_INSTALL_DIR)

# Where to install SvxLink config files
if(NOT DEFINED SVX_SYSCONF_INSTALL_DIR)
  set(SVX_SYSCONF_INSTALL_DIR ${SYSCONF_INSTALL_DIR}/svxlink)
endif(NOT DEFINED SVX_SYSCONF_INSTALL_DIR)

# Where to install SvxLink spool files
if(NOT DEFINED SVX_SPOOL_INSTALL_DIR)
  set(SVX_SPOOL_INSTALL_DIR ${LOCAL_STATE_DIR}/spool/svxlink)
endif(NOT DEFINED SVX_SPOOL_INSTALL_DIR)

# Where to install SvxLink architecture independent files
if(NOT DEFINED SVX_SHARE_INSTALL_DIR)
  set(SVX_SHARE_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/svxlink)
endif(NOT DEFINED SVX_SHARE_INSTALL_DIR)

# Where to install include files
if(NOT DEFINED SVX_INCLUDE_INSTALL_DIR)
  set(SVX_INCLUDE_INSTALL_DIR ${INCLUDE_INSTALL_DIR}/svxlink)
endif(NOT DEFINED SVX_INCLUDE_INSTALL_DIR)

# Where to install SvxLink modules
if(NOT DEFINED SVX_MODULE_INSTALL_DIR)
  set(SVX_MODULE_INSTALL_DIR ${LIB_INSTALL_DIR}/svxlink)
endif(NOT DEFINED SVX_MODULE_INSTALL_DIR)


##############################################################################
# Functions
##############################################################################

# Create an include file under the global include directory
function(expinc filename)
  #  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${filename}
  #  ${PROJECT_INCLUDE_DIR}/${filename}
  #  COPYONLY
  #  )
endfunction(expinc)

# Add a target for building a version/xyz.h file
function(add_version_target name)
  if(${VER_${name}})
    set(ver ${VER_${name}})
  else(${VER_${name}})
    set(ver "?.?.?")
  endif(${VER_${name}})

  add_custom_command(
    OUTPUT version/${name}.h
    #WORKING_DIRECTORY ${PROJECT_INCLUDE_DIR}
    COMMAND ${CMAKE_SOURCE_DIR}/create_version_include.sh ${name} ${ver}
    DEPENDS ${CMAKE_SOURCE_DIR}/versions
            ${CMAKE_SOURCE_DIR}/create_version_include.sh
  )
  #set(VER_VAR_NAME ${name})
  #configure_file(${CMAKE_SOURCE_DIR}/version.h.in
  #               version/${name}.h)
  include_directories(${CMAKE_CURRENT_BINARY_DIR})
endfunction(add_version_target)

# Read a versions file and set up CMake variables for each entry
function(read_versions_file filename)
  message("-- Reading versions file...")
  file(STRINGS ${filename} versions REGEX .*=.*)
  foreach(version ${versions})
    string(REGEX REPLACE \(.*\)=\(.*\) "VER_\\1;\\2" version ${version})
    set(${version} PARENT_SCOPE)
    if(version MATCHES ^VER_LIB)
      list(GET version 0 varname)
      list(GET version 1 varvalue)
      string(REGEX MATCH [0-9]+\\.[0-9]+ ${varname}_SOVERSION ${varvalue})
      set(${varname}_SOVERSION ${${varname}_SOVERSION} PARENT_SCOPE)
    endif(version MATCHES ^VER_LIB)
  endforeach(version)
endfunction(read_versions_file)

# Create the given directory during installation
function(install_mkdir dir)
  get_filename_component(parent ${dir} PATH)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${dir})
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${dir}
        DESTINATION ${parent})
endfunction(install_mkdir)


##############################################################################
# Main execution starts here
##############################################################################

# Load the versions file and define version variables
read_versions_file(${PROJECT_SOURCE_DIR}/versions)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
  @ONLY
  )

# Find the Sigc++ library
find_package(SIGC REQUIRED)
include_directories(${SIGC_INCLUDE_DIRS})
set(LIBS ${LIBS} ${SIGC_LIBRARIES})

# Add directories to build
add_subdirectory(async)
add_subdirectory(misc)
add_subdirectory(echolib)
add_subdirectory(locationinfo)
add_subdirectory(svxlink)
add_subdirectory(qtel)
add_subdirectory(doc)

