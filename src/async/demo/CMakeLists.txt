set(CPPPROGS AsyncAudioIO_demo AsyncDnsLookup_demo AsyncFdWatch_demo
             AsyncTimer_demo AsyncTcpClient_demo AsyncUdpSocket_demo
             AsyncCppApplication_demo AsyncTcpServer_demo AsyncConfig_demo
             AsyncSerial_demo AsyncAtTimer_demo AsyncExec_demo
             AsyncPtyStreamBuf_demo)


foreach(prog ${CPPPROGS})
  add_executable(${prog} ${prog}.cpp)
  target_link_libraries(${prog} ${LIBS} asynccpp asyncaudio asynccore)
endforeach(prog)

if(USE_QT)
  # Find Qt5
  find_package(Qt5 REQUIRED Core Gui Network)
  include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5Gui_INCLUDE_DIRS} ${Qt5Network_INCLUDE_DIRS})
  set(LIBS ${LIBS} ${Qt5Core_LIBRARIES} ${Qt5Gui_LIBRARIES} ${Qt5Network_LIBRARIES})

  add_executable(AsyncQtApplication_demo AsyncQtApplication_demo.cpp)
  target_link_libraries(AsyncQtApplication_demo ${LIBS} asyncaudio
                        asynccore asyncqt)
endif(USE_QT)

