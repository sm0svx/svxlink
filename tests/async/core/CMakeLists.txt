##############################################################################
# Tests for src/async/core/
##############################################################################

# Determine which Catch2 link target to use.  v3 ships Catch2::Catch2WithMain;
# v2 ships it from 2.13 onward.  Fall back to defining CATCH_CONFIG_MAIN when
# neither exists (very old v2).
if(TARGET Catch2::Catch2WithMain)
  set(CATCH2_MAIN_TARGET Catch2::Catch2WithMain)
elseif(TARGET Catch2::Catch2)
  # Older v2 — provide main via the header macro in the source file
  set(CATCH2_MAIN_TARGET Catch2::Catch2)
  add_compile_definitions(CATCH_PROVIDE_MAIN_VIA_MACRO)
else()
  message(FATAL_ERROR "Catch2 targets not found — cannot build tests.")
endif()

# AsyncConfigBackend test binary
add_executable(AsyncConfigBackend_test AsyncConfigBackend_test.cpp)

target_include_directories(AsyncConfigBackend_test PRIVATE
  ${PROJECT_INCLUDE_DIR}              # installed/exported headers (AsyncConfig.h etc.)
  ${CMAKE_SOURCE_DIR}/async/core      # non-exported headers (AsyncFileConfigBackend.h etc.)
  ${CMAKE_BINARY_DIR}                 # generated config.h
)

target_link_libraries(AsyncConfigBackend_test
  PRIVATE
    asynccore
    ${CATCH2_MAIN_TARGET}
)

# Propagate the HAS_*_SUPPORT defines that asynccore's CMakeLists sets via
# add_definitions().  Because tests/ is processed after async/core/, the
# directory has already been configured and its COMPILE_DEFINITIONS are
# readable here.
get_directory_property(_asynccore_dir_defs
  DIRECTORY ${CMAKE_SOURCE_DIR}/async/core
  COMPILE_DEFINITIONS)
if(_asynccore_dir_defs)
  target_compile_definitions(AsyncConfigBackend_test PRIVATE ${_asynccore_dir_defs})
endif()

add_test(
  NAME    AsyncConfigBackend_test
  COMMAND AsyncConfigBackend_test --reporter compact
)
