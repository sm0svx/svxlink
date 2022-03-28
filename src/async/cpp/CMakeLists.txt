set(LIBNAME asynccpp)

set(EXPINC AsyncCppApplication.h)

set(LIBSRC AsyncCppApplication.cpp AsyncCppDnsLookupWorker.cpp)

set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Find pthreads
find_package(Threads)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

# Find libresolv for real?
set(LIBS ${LIBS} resolv)

# Set up additional defines
# FIXME: Do we need this?
add_definitions(-D_REENTRANT)

# Find librt
find_package(RT REQUIRED)
set(LIBS ${LIBS} ${RT_LIBRARIES})

# Build a shared library and a static library if configured
add_library(${LIBNAME} SHARED ${LIBSRC})
set_target_properties(${LIBNAME} PROPERTIES VERSION ${VER_LIBASYNC}
                      SOVERSION ${VER_LIBASYNC_SOVERSION})
target_link_libraries(${LIBNAME} ${LIBS})
if (BUILD_STATIC_LIBS)
  add_library(${LIBNAME}_static STATIC ${LIBSRC})
  set_target_properties(${LIBNAME}_static PROPERTIES OUTPUT_NAME ${LIBNAME})
  target_link_libraries(${LIBNAME}_static ${LIBS})
endif(BUILD_STATIC_LIBS)

# Install files
install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
if (BUILD_STATIC_LIBS)
  install(TARGETS ${LIBNAME}_static DESTINATION ${LIB_INSTALL_DIR})
endif(BUILD_STATIC_LIBS)
install(FILES ${EXPINC} DESTINATION ${SVX_INCLUDE_INSTALL_DIR})
