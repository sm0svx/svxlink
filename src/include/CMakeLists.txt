# Read the project versions file and set up CMake variables for each version
# specified in that file. If the version name begins with LIB then also a
# variable containing a suitable SOVERSION will be generated.
#
# In addition to the variables, an include file is generated for each version
# entry containing a define for the specific version.
#
# As an example, if the versions file contains a row containing MY_APP=1.0.0 a
# CMake variable VER_MY_APP=1.0.0 will be defined and can thus be used in the
# rest of the project. The generated include file can be included using
# #include <version/MY_APP.h>

file(STRINGS ${PROJECT_SOURCE_DIR}/versions versions REGEX .*=.*)
foreach(version ${versions})
  string(REGEX REPLACE \(.*\)=\(.*\) "\\1" ver_name ${version})
  string(REGEX REPLACE \(.*\)=\(.*\) "\\2" ver_value ${version})
  set(VER_${ver_name} "${ver_value}")
  set(VER_${ver_name} "${ver_value}" PARENT_SCOPE)
  if(ver_name MATCHES ^LIB)
    string(REGEX MATCH [0-9]+\\.[0-9]+ VER_${ver_name}_SOVERSION "${ver_value}")
    set(VER_${ver_name}_SOVERSION ${VER_${ver_name}_SOVERSION} PARENT_SCOPE)
  endif(ver_name MATCHES ^LIB)
endforeach(version)

execute_process(
  COMMAND git describe --tags
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  OUTPUT_VARIABLE PROJECT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
if("${PROJECT_VERSION}" STREQUAL "")
  set(PROJECT_VERSION "${VER_PROJECT}")
endif()
set(PROJECT_VERSION "${PROJECT_VERSION}" PARENT_SCOPE)

foreach(version ${versions})
  string(REGEX REPLACE \(.*\)=\(.*\) "\\1" ver_name ${version})
  string(REGEX REPLACE \(.*\)=\(.*\) "\\2" ver_value ${version})
  configure_file(version_include_file.in
    ${CMAKE_CURRENT_BINARY_DIR}/version/${ver_name}.h
    @ONLY)
endforeach(version)

# Force a CMake rerun when the versions file change
set_property(DIRECTORY APPEND PROPERTY
  CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/versions)
