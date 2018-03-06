# The name of the library
set(LIBNAME trx)

# Which include files to export to the global include directory
set(EXPINC Rx.h Tx.h NetTrxMsg.h LocalRx.h Modulation.h)

# What sources to compile for the library
set(LIBSRC
  ToneDetector.cpp Dh1dmSwDtmfDecoder.cpp Rx.cpp LocalRx.cpp
  SquelchVox.cpp SigLevDetNoise.cpp NetRx.cpp Voter.cpp
  Tx.cpp LocalTx.cpp DtmfEncoder.cpp NetTx.cpp
  NetTrxTcpClient.cpp DtmfDecoder.cpp HwDtmfDecoder.cpp
  S54sDtmfDecoder.cpp PttCtrl.cpp MultiTx.cpp
  SigLevDetTone.cpp Sel5Decoder.cpp SwSel5Decoder.cpp
  SquelchEvDev.cpp Macho.cpp SquelchGpio.cpp Ptt.cpp
  PttGpio.cpp PttSerialPin.cpp PttPty.cpp
  PtyDtmfDecoder.cpp LocalRxBase.cpp Ddr.cpp RtlSdr.cpp RtlTcp.cpp
  WbRxRtlSdr.cpp SigLevDet.cpp SigLevDetDdr.cpp
  SvxSwDtmfDecoder.cpp LocalRxSim.cpp SigLevDetSim.cpp
  AfskDtmfDecoder.cpp SigLevDetAfsk.cpp Modulation.cpp
)
include (CheckSymbolExists)
CHECK_SYMBOL_EXISTS(HIDIOCGRAWINFO linux/hidraw.h HAS_HIDRAW_SUPPORT)
if (HAS_HIDRAW_SUPPORT)
  set (LIBSRC ${LIBSRC} PttHidraw.cpp SquelchHidraw.cpp)
  add_definitions(-DHAS_HIDRAW_SUPPORT)
endif (HAS_HIDRAW_SUPPORT)

# Which other libraries this library depends on
set(LIBS ${LIBS} digital)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Find rtl-sdr
find_package(RtlSdr)
if (RTLSDR_FOUND)
  set(LIBS ${LIBS} ${RTLSDR_LIBRARIES})
  include_directories(${RTLSDR_INCLUDE_DIRS})
  add_definitions(${RTLSDR_DEFINITIONS} -DHAS_RTLSDR_SUPPORT)
  set(LIBSRC ${LIBSRC} RtlUsb.cpp)

  # We also need pthreads when using the RtlUsb class
  find_package(Threads REQUIRED)
  set(LIBS ${LIBS} ${CMAKE_THREAD_LIBS_INIT})
  add_definitions(-D_REENTRANT)
else (RTLSDR_FOUND)
  message(
    "--   The rtl-sdr library is an optional dependency.\n"
    "--   Direct USB access to RTL2832u DVB-T/SDR dongles will be\n"
    "--   unavailable. Access through rtl_tcp will still work though."
  )
endif (RTLSDR_FOUND)

# Add targets for version files
set(VERSION_DEPENDS)
add_version_target(SVXLINK VERSION_DEPENDS)

# Build only a static library
add_library(${LIBNAME} STATIC ${LIBSRC} ${VERSION_DEPENDS})
target_link_libraries(${LIBNAME} ${LIBS})

add_executable(DtmfDecoderTest DtmfDecoderTest.cpp)
target_link_libraries(DtmfDecoderTest ${LIBNAME} asynccore asyncaudio)

# Install targets
#install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
