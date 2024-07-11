##############################################################################
# SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
# Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX
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
cmake_minimum_required(VERSION 3.5)
project(svxlink C CXX)
#enable_testing()

# The path to the project global include directory
set(PROJECT_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

# Where to put library files
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# We need to force buildtime RPATH so that dlopen will find plugins in the
# library output directory when running applications within the build tree
# (uninstalled)
set(CMAKE_BUILD_RPATH "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")

# Where to put executable files
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Add project local CMake module directory
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Optional parts
option(USE_QT "Build Qt applications and libs" ON)
option(BUILD_STATIC_LIBS "Build static libraries in addition to dynamic" OFF)

# The sample rate used internally in SvxLink
if(NOT DEFINED INTERNAL_SAMPLE_RATE)
  set(INTERNAL_SAMPLE_RATE 16000)
endif(NOT DEFINED INTERNAL_SAMPLE_RATE)
add_definitions(-DINTERNAL_SAMPLE_RATE=${INTERNAL_SAMPLE_RATE})

# Set up include directories
include_directories(
  ${PROJECT_INCLUDE_DIR}
  ${CMAKE_BINARY_DIR}
)

# Warnings should be enabled for GCC. Also turning off the NDEBUG flag
# since that remove asserts.
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_C_FLAGS
    "${CMAKE_C_FLAGS} -Wall -Wstrict-prototypes -Wpointer-arith")
  set(CMAKE_C_FLAGS_RELEASE "-O3")
  set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

  set(CMAKE_CXX_FLAGS
    "${CMAKE_CXX_FLAGS} -Wall -Wpointer-arith")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

  option(USE_GPROF "Enable profiling" OFF)
  if(USE_GPROF)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
  endif(USE_GPROF)
endif()

# Set the default build type to Release
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
endif(NOT CMAKE_BUILD_TYPE)


##############################################################################
# Install targets properties setup
##############################################################################

# Set up standard GNU installation directories
include(GNUInstallDirs)

# Where to install include files
if(NOT DEFINED INCLUDE_INSTALL_DIR)
  #set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/include)
  set(INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_FULL_INCLUDEDIR})
endif(NOT DEFINED INCLUDE_INSTALL_DIR)

# Where to install libraries
if(NOT DEFINED LIB_INSTALL_DIR)
  #set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX})
  set(LIB_INSTALL_DIR ${CMAKE_INSTALL_FULL_LIBDIR})
endif(NOT DEFINED LIB_INSTALL_DIR)

# The config directory (normally /etc)
if(NOT DEFINED SYSCONF_INSTALL_DIR)
  #set(SYSCONF_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/etc)
  set(SYSCONF_INSTALL_DIR ${CMAKE_INSTALL_FULL_SYSCONFDIR})
endif(NOT DEFINED SYSCONF_INSTALL_DIR)

# Architecture independent files directory (normally /usr/share)
if(NOT DEFINED SHARE_INSTALL_PREFIX)
  #set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share)
  set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_FULL_DATADIR})
endif(NOT DEFINED SHARE_INSTALL_PREFIX)

# Local state directory (normally /var)
if(NOT DEFINED LOCAL_STATE_DIR)
  #set(LOCAL_STATE_DIR ${CMAKE_INSTALL_PREFIX}/var)
  set(LOCAL_STATE_DIR ${CMAKE_INSTALL_FULL_LOCALSTATEDIR})
endif(NOT DEFINED LOCAL_STATE_DIR)

# Where to install executables
if(NOT DEFINED BIN_INSTALL_DIR)
  #set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
  set(BIN_INSTALL_DIR ${CMAKE_INSTALL_FULL_BINDIR})
endif(NOT DEFINED BIN_INSTALL_DIR)

# Where to install system executables
if(NOT DEFINED SBIN_INSTALL_DIR)
  #set(BIN_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/bin)
  set(SBIN_INSTALL_DIR ${CMAKE_INSTALL_FULL_SBINDIR})
endif(NOT DEFINED SBIN_INSTALL_DIR)

# Where to install manual pages
if(NOT DEFINED MAN_INSTALL_DIR)
  #set(MAN_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/man)
  set(MAN_INSTALL_DIR ${CMAKE_INSTALL_FULL_MANDIR})
endif(NOT DEFINED MAN_INSTALL_DIR)

# Where to install documentation
if(NOT DEFINED DOC_INSTALL_DIR)
  #set(DOC_INSTALL_DIR ${SHARE_INSTALL_PREFIX}/doc/svxlink)
  set(DOC_INSTALL_DIR ${CMAKE_INSTALL_FULL_DOCDIR})
endif(NOT DEFINED DOC_INSTALL_DIR)

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

# Where to put SvxLink variable files
if(NOT DEFINED SVX_LOCAL_STATE_DIR)
  set(SVX_LOCAL_STATE_DIR ${LOCAL_STATE_DIR}/lib/svxlink)
endif(NOT DEFINED SVX_LOCAL_STATE_DIR)

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

# Where to install SvxLink logic cores
if(NOT DEFINED SVX_LOGIC_CORE_INSTALL_DIR)
  set(SVX_LOGIC_CORE_INSTALL_DIR ${LIB_INSTALL_DIR}/svxlink)
endif(NOT DEFINED SVX_LOGIC_CORE_INSTALL_DIR)

# Clear global variables implemented using internal cache
set(CONFIG_FILES "" CACHE INTERNAL "CONFIG_FILES")
set(FILE_OWNER "" CACHE INTERNAL "FILE_OWNER")

##############################################################################
# Functions
##############################################################################

# Create an include file under the global include directory
function(expinc filename)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${filename}
    ${PROJECT_INCLUDE_DIR}/${filename}
    COPYONLY
    )
endfunction(expinc)

# Create a post install target to change the owner of an installed file.
# If DO_INSTALL_CHOWN is set to YES, the owner will be changed during
# installation.
# If DO_INSTALL_CHOWN is unset, the DESTDIR environment variable will be
# examined. If it is set, no chown operations will be performed since we are
# probably not running as a user with administrative rights. If DESTDIR is
# unset, chown operations are performed.
function(install_chown filename owner)
  if(owner)
    set(FILE_OWNER "${FILE_OWNER}${owner} ${filename};"
	    CACHE INTERNAL "FILE_OWNER")
    set(chown_commands "
      set(CHOWN_TOOL ${CHOWN_TOOL})
      if(NOT CHOWN_TOOL)
        MESSAGE(FATAL_ERROR \"Unable to find the 'chown' utility\")
      endif(NOT CHOWN_TOOL)
      set(full_filename \"\$ENV{DESTDIR}${filename}\")
      message(STATUS \"Setting owner of \${full_filename} to ${owner}...\")
      execute_process(
        COMMAND ${CHOWN_TOOL} ${owner} \"\${full_filename}\"
        RESULT_VARIABLE cmd_result
        )
      if(NOT \${cmd_result} EQUAL 0)
        MESSAGE(FATAL_ERROR
          \"Error while changing owner of file \${full_filename}\"
          )
      endif(NOT \${cmd_result} EQUAL 0)
      ")
    if(DEFINED DO_INSTALL_CHOWN)
      if(DO_INSTALL_CHOWN)
        install(CODE "${chown_commands}")
      endif(DO_INSTALL_CHOWN)
    else(DEFINED DO_INSTALL_CHOWN)
      install(CODE "
        if(\"\$ENV{DESTDIR}\" STREQUAL \"\")
          ${chown_commands}
        endif(\"\$ENV{DESTDIR}\" STREQUAL \"\")
        ")
    endif(DEFINED DO_INSTALL_CHOWN)
  endif(owner)
endfunction(install_chown)

# Create the given directory during installation
#   install_mkdir(directory [owner])
function(install_mkdir dir)
  set(owner ${ARGV1})
  get_filename_component(parent ${dir} PATH)
  file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${dir})
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}${dir} DESTINATION ${parent})
  install_chown(${dir} "${owner}")
endfunction(install_mkdir)

# Before installing the file, check if it exists. If it does, don't
# overwrite it.
#   install_if_not_exists(source_file destination_directory)
function(install_if_not_exists src dest)
  if(NOT IS_ABSOLUTE "${src}")
    set(src "${CMAKE_CURRENT_SOURCE_DIR}/${src}")
  endif()
  get_filename_component(src_name "${src}" NAME)
  if (NOT IS_ABSOLUTE "${dest}")
    set(dest "${CMAKE_INSTALL_PREFIX}/${dest}")
  endif()
  set(CONFIG_FILES "${CONFIG_FILES}${dest}/${src_name} "
	  CACHE INTERNAL "CONFIG_FILES")
  install(CODE "
    if(NOT EXISTS \"\$ENV{DESTDIR}${dest}/${src_name}\")
      #file(INSTALL \"${src}\" DESTINATION \"${dest}\")
      message(STATUS \"Installing: \$ENV{DESTDIR}${dest}/${src_name}\")
      execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"${src}\"
                      \"\$ENV{DESTDIR}${dest}/${src_name}\"
                      RESULT_VARIABLE copy_result
                      ERROR_VARIABLE error_output)
      if(copy_result)
        message(FATAL_ERROR \${error_output})
      endif()
    else()
      message(STATUS \"Skipping  : \$ENV{DESTDIR}${dest}/${src_name}\")
    endif()
  ")
endfunction(install_if_not_exists)

##############################################################################
# Main execution starts here
##############################################################################

add_subdirectory(include)

configure_file(${CMAKE_SOURCE_DIR}/config.h.in
  ${CMAKE_BINARY_DIR}/config.h
  @ONLY
  )

# Find the Sigc++ library
find_package(SIGC2 REQUIRED)
include_directories(${SIGC2_INCLUDE_DIRS})
add_definitions(${SIGC2_DEFINITIONS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SIGC2_CXX_FLAGS}")
set(LIBS ${LIBS} ${SIGC2_LIBRARIES})

find_package(LADSPA)
if(LADSPA_FOUND)
  if(DEFINED LADSPA_VERSION_MAJOR)
    include_directories(${LADSPA_INCLUDE_DIRS})
    add_definitions(${LADSPA_DEFINITIONS})
  else()
    message(WARNING
      "Found LADSPA but version could not be resolved. "
      "Will proceed without LADSPA.")
  endif()
else(LADSPA_FOUND)
  message("--   LADSPA is an optional dependency. The build will complete")
  message("--   without it but support for loading LADSPA plugins will")
  message("--   be unavailable.")
endif(LADSPA_FOUND)

# Find the chown utility
include(FindCHOWN)

set(SVXLINK_USER "svxlink" CACHE STRING "Set SvxLink system user")
set(SVXLINK_GROUP "svxlink" CACHE STRING "Set SvxLink system group")
message(STATUS "SvxLink user = ${SVXLINK_USER}")
message(STATUS "SvxLink group = ${SVXLINK_GROUP}")

# Add directories to build
add_subdirectory(doc)
add_subdirectory(async)
add_subdirectory(misc)
add_subdirectory(echolib)
add_subdirectory(locationinfo)
add_subdirectory(svxlink)
if(USE_QT)
  add_subdirectory(qtel)
endif(USE_QT)

# Experimental CPack package building
set(CPACK_SET_DESTDIR "ON")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
  "Advanced voice services system for ham radio use")
string(REPLACE "-" "." CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
message(STATUS "Package Version = ${CPACK_PACKAGE_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION
  "The SvxLink project develops an advanced voice services system for ham radio use. It can be run on a simplex frequency or act as a repeater controller.")
set(CPACK_PACKAGE_VENDOR "SM0SVX")
set(CPACK_PACKAGE_CONTACT "SvxLink Community <svxlink@groups.io>")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/../COPYRIGHT")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/../README.adoc")

if (CPACK_GENERATOR STREQUAL "DEB")
  #set(CPACK_DEBIAN_PACKAGE_DEPENDS "libsigc++-2.0-0v5, libgsm1, libpopt0, tcl, libgcrypt20, libspeex1, libasound2, libopus0, alsa-utils, vorbis-tools, libcurl4, rtl-sdr, libjsoncpp1")
  string(REPLACE " " "\n" CONFIG_FILES_NL "${CONFIG_FILES}")
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/conffiles" "${CONFIG_FILES_NL}")
  foreach(file ${FILE_OWNER})
    set(chown_file_commands "${chown_file_commands}${CHOWN_TOOL} ${file}\n")
  endforeach()
  file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/postinst"
    "getent group ${SVXLINK_GROUP} >/dev/null || groupadd -r ${SVXLINK_GROUP}\n"
    "getent passwd ${SVXLINK_USER} >/dev/null || useradd -r -g ${SVXLINK_GROUP} ${SVXLINK_USER}\n"
    "${chown_file_commands}")
  set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/conffiles;${CMAKE_CURRENT_BINARY_DIR}/postinst")
endif(CPACK_GENERATOR STREQUAL "DEB")

include(CPack)
