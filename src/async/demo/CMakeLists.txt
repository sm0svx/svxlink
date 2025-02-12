set(CPPPROGS AsyncAudioIO_demo AsyncDnsLookup_demo AsyncFdWatch_demo
             AsyncTimer_demo AsyncTcpClient_demo AsyncUdpSocket_demo
             AsyncCppApplication_demo AsyncTcpServer_demo AsyncConfig_demo
             AsyncSerial_demo AsyncAtTimer_demo AsyncExec_demo
             AsyncPtyStreamBuf_demo AsyncMsg_demo AsyncFramedTcpServer_demo
             AsyncFramedTcpClient_demo AsyncAudioSelector_demo
             AsyncAudioFsf_demo AsyncHttpServer_demo AsyncFactory_demo
             AsyncAudioContainer_demo AsyncTcpPrioClient_demo
             AsyncStateMachine_demo AsyncPlugin_demo
             AsyncSslTcpServer_demo AsyncSslTcpClient_demo
             AsyncSslX509_demo AsyncDigest_demo
             )

set(QTPROGS AsyncQtApplication_demo)

if(LADSPA_FOUND)
  set(CPPPROGS ${CPPPROGS} AsyncAudioLADSPAPlugin_demo)
endif(LADSPA_FOUND)


# Build all demo applications
foreach(prog ${CPPPROGS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} ${LIBS} asynccpp asyncaudio asynccore)
endforeach(prog)

if(USE_QT)
  # Find Qt5
  find_package(Qt5Core QUIET)
  if (Qt5Core_FOUND)
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

  foreach(prog ${QTPROGS})
    add_executable(${prog} ${prog}.cpp)
    target_link_libraries(${prog} ${LIBS} asyncaudio asynccore asyncqt)
  endforeach(prog)
endif(USE_QT)

# Build the demo plugin used by the AsyncPlugin_demo application
add_library(DemoPlugin MODULE DemoPlugin.cpp)
set_target_properties(DemoPlugin PROPERTIES PREFIX "")
set_property(TARGET DemoPlugin PROPERTY NO_SONAME 1)
#target_link_libraries(DemoPlugin ${LIBS})

