cmake_minimum_required(VERSION 2.6)
project(svxlink C CXX)

# Create an include file under the global include directory
function(expinc filename)
  configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${filename}
    ${PROJECT_BINARY_DIR}/include/${filename}
    COPYONLY
    )
endfunction(expinc)

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
include_directories("${PROJECT_BINARY_DIR}/include")

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

