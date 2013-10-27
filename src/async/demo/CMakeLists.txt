set(CPPPROGS AsyncAudioIO_demo AsyncDnsLookup_demo AsyncFdWatch_demo
             AsyncTimer_demo AsyncTcpClient_demo AsyncUdpSocket_demo
             AsyncCppApplication_demo AsyncTcpServer_demo AsyncConfig_demo
             AsyncSerial_demo AsyncAtTimer_demo AsyncExec_demo)


foreach(prog ${CPPPROGS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} ${LIBS} asynccpp asyncaudio asynccore)
endforeach(prog)

if(USE_QT)
  # Find Qt4
  find_package(Qt4 4.2 REQUIRED QtCore QtGui QtNetwork)
  include(${QT_USE_FILE})
  set(LIBS ${LIBS} ${QT_LIBRARIES})
  #QT4_WRAP_CPP(QTHEADERS_MOC ${QTHEADERS})

  add_executable(AsyncQtApplication_demo AsyncQtApplication_demo.cpp)
  target_link_libraries(AsyncQtApplication_demo ${LIBS} asyncaudio
                        asynccore asyncqt)
endif(USE_QT)

