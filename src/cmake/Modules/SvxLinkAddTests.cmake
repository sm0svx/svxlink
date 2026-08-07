# SvxLinkAddTests.cmake
#
# Conflict-free unit-test registration for SvxLink.
#
# svxlink_add_tests() auto-discovers every *Test.cpp file in the calling
# directory and registers each as a CTest test named after the file (without
# extension). Adding a new unit test then only requires dropping a new
# <Name>Test.cpp file into the directory -- no edits to this file or to any
# shared CMakeLists.txt are needed, so independent branches/PRs that each add
# a test never conflict with one another.
#
# Common link libraries for the directory are passed via LIBS:
#   svxlink_add_tests(LIBS asynccore asynccpp)
#
# A test that needs extra sources or libraries beyond the directory default
# can declare them in an optional sidecar file <Name>Test.deps.cmake placed
# next to the test source. The sidecar is include()d before the target is
# created and may set:
#   <Name>Test_EXTRA_SRCS  - extra source files to compile into the test
#   <Name>Test_EXTRA_LIBS  - extra libraries to link
# Because the sidecar is a separate per-test file, it is also conflict-free.

function(svxlink_add_tests)
  cmake_parse_arguments(SAT "" "" "LIBS" ${ARGN})

  file(GLOB _svxlink_test_srcs CONFIGURE_DEPENDS
       "${CMAKE_CURRENT_SOURCE_DIR}/*Test.cpp")

  foreach(_src ${_svxlink_test_srcs})
    get_filename_component(_name "${_src}" NAME_WE)

    set(${_name}_EXTRA_SRCS "")
    set(${_name}_EXTRA_LIBS "")
    set(_sidecar "${CMAKE_CURRENT_SOURCE_DIR}/${_name}.deps.cmake")
    if(EXISTS "${_sidecar}")
      include("${_sidecar}")
    endif()

    add_executable(${_name} "${_src}" ${${_name}_EXTRA_SRCS})
    if(SAT_LIBS OR ${_name}_EXTRA_LIBS)
      target_link_libraries(${_name} ${SAT_LIBS} ${${_name}_EXTRA_LIBS})
    endif()
    add_test(NAME ${_name} COMMAND ${_name})
  endforeach()
endfunction()
