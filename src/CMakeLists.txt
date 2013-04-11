cmake_minimum_required(VERSION 2.6)
project(svxlink C CXX)

# The path to the project global include directory
set(PROJECT_INCLUDE_DIR ${PROJECT_BINARY_DIR}/include)

# Where to put library files
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

# Where to put executable files
set(RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

# Create an include file under the global include directory
function(expinc filename)
  #  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${filename}
  #  ${PROJECT_INCLUDE_DIR}/${filename}
  #  COPYONLY
  #  )
endfunction(expinc)

# Add a target for building a version/xyz.h file
function(add_version_target name ver)
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

read_versions_file(${PROJECT_SOURCE_DIR}/versions)

# Add the global include directory to the include search path
#include_directories("${PROJECT_BINARY_DIR}/include")
include_directories(
  ${CMAKE_SOURCE_DIR}/misc
  ${CMAKE_SOURCE_DIR}/async/core
  ${CMAKE_SOURCE_DIR}/async/cpp
  ${CMAKE_SOURCE_DIR}/async/audio
  ${CMAKE_SOURCE_DIR}/async/qt
  ${CMAKE_SOURCE_DIR}/echolib
  ${CMAKE_SOURCE_DIR}/locationinfo
)

# Add extra module directory
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# Optional parts
option(USE_QT "Build Qt applications and libs" ON)

# Find the Sigc++ library
find_package(SIGC REQUIRED)
include_directories(${SIGC_INCLUDE_DIRS})
set(LIBS ${LIBS} ${SIGC_LIBRARIES})

# Add extra defines
add_definitions(-DINTERNAL_SAMPLE_RATE=16000)

# Add directories to build
add_subdirectory(async)
add_subdirectory(misc)
add_subdirectory(echolib)
add_subdirectory(locationinfo)
add_subdirectory(svxlink)
add_subdirectory(qtel)
#add_subdirectory(doc)

