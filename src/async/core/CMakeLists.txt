set(LIBNAME asynccore)

set(EXPINC AsyncApplication.h AsyncFdWatch.h AsyncTimer.h AsyncIpAddress.h
           AsyncTcpClient.h AsyncDnsLookup.h AsyncUdpSocket.h AsyncTcpServer.h
           AsyncTcpConnection.h AsyncConfig.h AsyncSerial.h AsyncFileReader.h
           AsyncAtTimer.h AsyncExec.h AsyncPty.h AsyncPtyStreamBuf.h AsyncMsg.h
           AsyncFramedTcpConnection.h AsyncTcpClientBase.h AsyncTcpServerBase.h
           AsyncHttpServerConnection.h AsyncFactory.h AsyncDnsResourceRecord.h
           AsyncTcpPrioClientBase.h AsyncTcpPrioClient.h AsyncStateMachine.h
           AsyncPlugin.h AsyncEncryptedUdpSocket.h
           AsyncSslContext.h AsyncSslKeypair.h AsyncSslCertSigningReq.h
           AsyncSslX509.h AsyncSslX509Extensions.h
           AsyncSslX509ExtSubjectAltName.h AsyncDigest.h)

set(LIBSRC AsyncApplication.cpp AsyncFdWatch.cpp AsyncTimer.cpp
           AsyncIpAddress.cpp AsyncDnsLookup.cpp AsyncTcpClientBase.cpp
           AsyncUdpSocket.cpp AsyncTcpServerBase.cpp
           AsyncTcpConnection.cpp AsyncConfig.cpp AsyncSerial.cpp
           AsyncSerialDevice.cpp AsyncFileReader.cpp
           AsyncAtTimer.cpp AsyncExec.cpp AsyncPty.cpp AsyncPtyStreamBuf.cpp
           AsyncFramedTcpConnection.cpp AsyncHttpServerConnection.cpp
           AsyncTcpPrioClientBase.cpp AsyncPlugin.cpp
           AsyncEncryptedUdpSocket.cpp)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

set(LIBS ${LIBS} -lcrypto -lssl)

# Find pthreads
find_package(Threads)
set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})

# Set up additional defines
# FIXME: Do we need this?
add_definitions(-D_REENTRANT)

# Find the dl library - only for Linux, not required for FreeBSD
if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  find_package(DL REQUIRED)
  set(LIBS ${LIBS} ${DL_LIBRARIES})
  include_directories(${DL_INCLUDES})
endif()

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
