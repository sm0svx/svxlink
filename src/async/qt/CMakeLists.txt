set(LIBNAME asyncqt)

set(EXPINC AsyncQtApplication.h)

set(LIBSRC AsyncQtApplication.cpp AsyncQtDnsLookupWorker.cpp)

set(QTHEADERS AsyncQtApplication.h AsyncQtTimer.h AsyncQtDnsLookupWorker.h)

set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Find Qt4
find_package(Qt4 4.2 REQUIRED QtCore QtGui QtNetwork)
include(${QT_USE_FILE})
set(LIBS ${LIBS} ${QT_LIBRARIES})
QT4_WRAP_CPP(QTHEADERS_MOC ${QTHEADERS})

# Build a shared library and a static library if configured
add_library(${LIBNAME} SHARED ${LIBSRC} ${QTHEADERS_MOC})
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
