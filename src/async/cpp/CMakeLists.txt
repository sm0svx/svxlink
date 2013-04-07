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

# Set up additional defines
# FIXME: Do we need this?
add_definitions(-D_REENTRANT)

# Find librt
find_package(RT REQUIRED)
set(LIBS ${LIBS} ${RT_LIBRARIES})

# Build both a shared and a static library
add_library(${LIBNAME} SHARED ${LIBSRC})
set_target_properties(${LIBNAME} PROPERTIES VERSION ${VER_LIBASYNC}
                      SOVERSION ${VER_LIBASYNC_SOVERSION})
target_link_libraries(${LIBNAME} ${LIBS})

