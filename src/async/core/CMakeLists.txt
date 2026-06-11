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
           AsyncSslX509ExtSubjectAltName.h AsyncDigest.h
           AsyncConfigBackend.h AsyncConfigSource.h)

# Base library sources
set(LIBSRC_BASE AsyncApplication.cpp AsyncFdWatch.cpp AsyncTimer.cpp
           AsyncIpAddress.cpp AsyncDnsLookup.cpp AsyncTcpClientBase.cpp
           AsyncUdpSocket.cpp AsyncTcpServerBase.cpp
           AsyncTcpConnection.cpp AsyncConfig.cpp AsyncSerial.cpp
           AsyncSerialDevice.cpp AsyncFileReader.cpp
           AsyncAtTimer.cpp AsyncExec.cpp AsyncPty.cpp AsyncPtyStreamBuf.cpp
           AsyncFramedTcpConnection.cpp AsyncHttpServerConnection.cpp
           AsyncTcpPrioClientBase.cpp AsyncPlugin.cpp
           AsyncEncryptedUdpSocket.cpp
           AsyncConfigBackend.cpp AsyncConfigSource.cpp AsyncFileConfigBackend.cpp)

# Initialize library sources with base sources
set(LIBSRC ${LIBSRC_BASE})

# Database backend detection and configuration
message(STATUS "Detecting database backend support...")

# Find SQLite3
find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
  pkg_check_modules(SQLITE3 QUIET sqlite3)
endif()

if(NOT SQLITE3_FOUND)
  find_path(SQLITE3_INCLUDE_DIR sqlite3.h)
  find_library(SQLITE3_LIBRARY sqlite3)
  if(SQLITE3_INCLUDE_DIR AND SQLITE3_LIBRARY)
    set(SQLITE3_FOUND TRUE)
    set(SQLITE3_LIBRARIES ${SQLITE3_LIBRARY})
    set(SQLITE3_INCLUDE_DIRS ${SQLITE3_INCLUDE_DIR})
  endif()
endif()

# Find MySQL/MariaDB
if(PKG_CONFIG_FOUND)
  pkg_check_modules(MYSQL QUIET mysqlclient)
  if(NOT MYSQL_FOUND)
    pkg_check_modules(MYSQL QUIET mariadb)
  endif()
endif()

if(NOT MYSQL_FOUND)
  find_path(MYSQL_INCLUDE_DIR mysql/mysql.h)
  find_library(MYSQL_LIBRARY mysqlclient)
  if(NOT MYSQL_LIBRARY)
    find_library(MYSQL_LIBRARY mariadb)
  endif()
  if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    set(MYSQL_FOUND TRUE)
    set(MYSQL_LIBRARIES ${MYSQL_LIBRARY})
    set(MYSQL_INCLUDE_DIRS ${MYSQL_INCLUDE_DIR})
  endif()
endif()

# Find PostgreSQL
if(PKG_CONFIG_FOUND)
  pkg_check_modules(POSTGRESQL QUIET libpq)
endif()

if(NOT POSTGRESQL_FOUND)
  find_path(POSTGRESQL_INCLUDE_DIR libpq-fe.h)
  find_library(POSTGRESQL_LIBRARY pq)
  if(POSTGRESQL_INCLUDE_DIR AND POSTGRESQL_LIBRARY)
    set(POSTGRESQL_FOUND TRUE)
    set(POSTGRESQL_LIBRARIES ${POSTGRESQL_LIBRARY})
    set(POSTGRESQL_INCLUDE_DIRS ${POSTGRESQL_INCLUDE_DIR})
  endif()
endif()

# Add backend sources and libraries based on availability
if(SQLITE3_FOUND)
  message(STATUS "  SQLite3: Found")
  add_definitions(-DHAS_SQLITE_SUPPORT)
  set(LIBSRC ${LIBSRC} AsyncSQLiteConfigBackend.cpp)
  set(EXPINC ${EXPINC} AsyncSQLiteConfigBackend.h)
  set(LIBS ${LIBS} ${SQLITE3_LIBRARIES})
  include_directories(${SQLITE3_INCLUDE_DIRS})
else()
  message(STATUS "  SQLite3: Not found")
endif()

if(MYSQL_FOUND)
  message(STATUS "  MySQL/MariaDB: Found")
  add_definitions(-DHAS_MYSQL_SUPPORT)
  set(LIBSRC ${LIBSRC} AsyncMySQLConfigBackend.cpp)
  set(EXPINC ${EXPINC} AsyncMySQLConfigBackend.h)
  set(LIBS ${LIBS} ${MYSQL_LIBRARIES})
  include_directories(${MYSQL_INCLUDE_DIRS})
else()
  message(STATUS "  MySQL/MariaDB: Not found")
endif()

if(POSTGRESQL_FOUND)
  message(STATUS "  PostgreSQL: Found")
  add_definitions(-DHAS_POSTGRESQL_SUPPORT)
  set(LIBSRC ${LIBSRC} AsyncPostgreSQLConfigBackend.cpp)
  set(EXPINC ${EXPINC} AsyncPostgreSQLConfigBackend.h)
  set(LIBS ${LIBS} ${POSTGRESQL_LIBRARIES})
  include_directories(${POSTGRESQL_INCLUDE_DIRS})
else()
  message(STATUS "  PostgreSQL: Not found")
endif()

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Find OpenSSL
#set(LIBS ${LIBS} -lcrypto -lssl)
find_package(OpenSSL REQUIRED)
set(LIBS ${LIBS} ${OPENSSL_LIBRARIES})

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
