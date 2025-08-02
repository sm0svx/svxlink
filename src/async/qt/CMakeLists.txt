set(LIBNAME asyncqt)

set(EXPINC AsyncQtApplication.h)

set(LIBSRC AsyncQtApplication.cpp AsyncQtDnsLookupWorker.cpp)

set(QTHEADERS AsyncQtApplication.h AsyncQtTimer.h AsyncQtDnsLookupWorker.h)

set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Find Qt6
find_package(Qt6Core QUIET)
if (Qt6Core_FOUND)
  message("-- Found Qt6 version ${Qt6Core_VERSION_STRING}")
  find_package(Qt6Gui REQUIRED)
  find_package(Qt6Widgets REQUIRED)
  find_package(Qt6Network REQUIRED)
  include_directories(${Qt6Core_INCLUDE_DIRS} ${Qt6Gui_INCLUDE_DIRS}
                      ${Qt6Widgets_INCLUDE_DIRS} ${Qt6Network_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${Qt6Core_LIBRARIES} ${Qt6Gui_LIBRARIES}
           ${Qt6Widgets_LIBRARIES} ${Qt6Network_LIBRARIES})

  # Generate targets for moc, uic and rcc
  QT6_WRAP_CPP(QTHEADERS_MOC ${QTHEADERS})
endif()

# If not Qt6 was found, try Qt5
if (NOT Qt6Core_FOUND)
# Find Qt5
find_package(Qt5Core QUIET)
if (Qt5Core_FOUND)
  message("-- Found Qt5 version ${Qt5Core_VERSION_STRING}")
  find_package(Qt5Gui REQUIRED)
  find_package(Qt5Widgets REQUIRED)
  find_package(Qt5Network REQUIRED)
  include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS}
                      ${Qt5Widgets_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES}
           ${Qt5Widgets_LIBRARIES} ${Qt5Network_LIBRARIES})
  QT5_WRAP_CPP(QTHEADERS_MOC ${QTHEADERS})
endif()

# If not Qt5 was found, try Qt4
if (NOT Qt5Core_FOUND)
  find_package(Qt4 4.2 REQUIRED QtCore QtGui QtNetwork)
  include(${QT_USE_FILE})
  set(LIBS ${LIBS} ${QT_LIBRARIES})
  QT4_WRAP_CPP(QTHEADERS_MOC ${QTHEADERS})
endif()
endif()

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
