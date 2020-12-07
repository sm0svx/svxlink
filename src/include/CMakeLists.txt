# Read the project versions file and set up CMake variables for each version
# specified in that file.  In addition to the variables, a target is set up for
# each version to generate a corresponding version include file. If the version
# name begins with LIB then also a variable containing a suitable SOVERSION
# will be generated.
#
# As an example, if the versions file contains a row containing MY_APP=1.0.0 a
# CMake variable VER_MY_APP=1.0.0 will be defined and can thus be used in the
# rest of the project. A target version-my-app will also be generated so that
# the dependency to a specific version h-file can be specified using the
# add_dependency(mytarget version-my-app) function call. The h-file can be
# included using #include <version/MY_APP.h>

#message(STATUS "Reading versions file...")
file(STRINGS ${PROJECT_SOURCE_DIR}/versions versions REGEX .*=.*)
foreach(version ${versions})
  string(REGEX REPLACE \(.*\)=\(.*\) "\\1" ver_name ${version})
  string(REGEX REPLACE \(.*\)=\(.*\) "\\2" ver_value ${version})
  set(VER_${ver_name} "${ver_value}" PARENT_SCOPE)
  if(ver_name MATCHES ^LIB)
    string(REGEX MATCH [0-9]+\\.[0-9]+ VER_${ver_name}_SOVERSION "${ver_value}")
    set(VER_${ver_name}_SOVERSION ${VER_${ver_name}_SOVERSION} PARENT_SCOPE)
  endif(ver_name MATCHES ^LIB)

  add_custom_command(
    OUTPUT ${PROJECT_INCLUDE_DIR}/version/${ver_name}.h
    WORKING_DIRECTORY ${PROJECT_INCLUDE_DIR}
    COMMAND ${CMAKE_COMMAND} -DVER_NAME=${ver_name} -DVER_VALUE=${ver_value} 
                     -P ${CMAKE_SOURCE_DIR}/cmake/create_version_include.cmake
    DEPENDS ${CMAKE_SOURCE_DIR}/versions
            ${CMAKE_SOURCE_DIR}/cmake/create_version_include.cmake
  )

  string(TOLOWER ${ver_name} ver_name_lower)
  string(REPLACE "_" "-" ver_name_lower ${ver_name_lower})
  add_custom_target(
    version-${ver_name_lower}
    DEPENDS ${PROJECT_INCLUDE_DIR}/version/${ver_name}.h
  )
endforeach(version)

