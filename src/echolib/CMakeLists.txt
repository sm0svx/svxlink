# Find the GSM codec library and include directory
find_package(GSM REQUIRED)
include_directories(${GSM_INCLUDE_DIR})
set(LIBS ${LIBS} ${GSM_LIBRARY})

# Find Speex
# Do a quiet find here since the Async library already do a verbose find
find_package(Speex QUIET)
if(Speex_FOUND AND Speex_VERSION_MAJOR)
  include_directories(${Speex_INCLUDE_DIRS})
  add_definitions(${Speex_DEFINITIONS})
  add_definitions("-DSPEEX_MAJOR=${Speex_VERSION_MAJOR}")
  set(LIBS ${LIBS} ${Speex_LIBRARIES})
endif(Speex_FOUND AND Speex_VERSION_MAJOR)

set(LIBNAME echolib)

set(INSTALL_INC EchoLinkDirectory.h EchoLinkDispatcher.h EchoLinkQso.h
  EchoLinkStationData.h EchoLinkProxy.h)
set(EXPINC ${INSTALL_INC} rtp.h)

set(LIBSRC EchoLinkDirectory.cpp EchoLinkQso.cpp rtpacket.cpp
  EchoLinkDispatcher.cpp EchoLinkStationData.cpp EchoLinkProxy.cpp
  EchoLinkDirectoryCon.cpp md5.c)

set(LIBS ${LIBS} asynccore asyncaudio)

set(EXECUTABLES EchoLinkDispatcher_demo EchoLinkDirectory_demo
                EchoLinkQso_demo)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Build a shared library and a static library if configured
add_library(${LIBNAME} SHARED ${LIBSRC})
set_target_properties(${LIBNAME} PROPERTIES VERSION ${VER_LIBECHOLIB}
  SOVERSION ${VER_LIBECHOLIB_SOVERSION})
target_link_libraries(${LIBNAME} ${LIBS})
if (BUILD_STATIC_LIBS)
  add_library(${LIBNAME}_static STATIC ${LIBSRC})
  set_target_properties(${LIBNAME}_static PROPERTIES OUTPUT_NAME ${LIBNAME})
  target_link_libraries(${LIBNAME}_static ${LIBS})
endif(BUILD_STATIC_LIBS)

# Find the popt library
find_package(Popt REQUIRED)
include_directories(${POPT_INCLUDE_DIRS})
add_definitions(${POPT_DEFINITIONS})

# Build applications
foreach(exe ${EXECUTABLES})
  add_executable(${exe} ${exe}.cpp)
  target_link_libraries(${exe} ${LIBS} echolib asynccpp asyncaudio asynccore)
endforeach(exe)

add_executable(echolib_test echolib_test.cpp EchoLinkQsoTest.cpp)
target_link_libraries(echolib_test ${LIBS} ${POPT_LIBRARIES} echolib asynccpp
                        asyncaudio asynccore)

# Install files
install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
if (BUILD_STATIC_LIBS)
  install(TARGETS ${LIBNAME}_static DESTINATION ${LIB_INSTALL_DIR})
endif(BUILD_STATIC_LIBS)
install(FILES ${INSTALL_INC} DESTINATION ${SVX_INCLUDE_INSTALL_DIR})
