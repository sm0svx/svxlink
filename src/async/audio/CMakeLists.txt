option(USE_ALSA "Alsa audio support" ON)
option(USE_OSS "OSS audio support" ON)

# Find Speex
find_package(Speex)
if(SPEEX_FOUND)
  include_directories(${SPEEX_INCLUDE_DIRS})
  add_definitions(${SPEEX_DEFINITIONS})
  set(LIBS ${LIBS} ${SPEEX_LIBRARIES})
endif(SPEEX_FOUND)

# Find Opus
find_package(Opus)
if(Opus_FOUND)
  include_directories(${Opus_INCLUDE_DIRS})
  add_definitions("-DOPUS_MAJOR=${Opus_VERSION_MAJOR}")
  set(LIBS ${LIBS} ${Opus_LIBRARIES})
endif(Opus_FOUND)

find_package(GSM REQUIRED)
include_directories(${GSM_INCLUDE_DIR})
set(LIBS ${LIBS} ${GSM_LIBRARY})

set(LIBNAME asyncaudio)

set(EXPINC AsyncAudioSource.h AsyncAudioSink.h AsyncAudioProcessor.h
           AsyncAudioClipper.h AsyncAudioCompressor.h
           AsyncAudioFilter.h AsyncSigCAudioSink.h  AsyncSigCAudioSource.h
           AsyncAudioIO.h AsyncAudioSplitter.h AsyncAudioDelayLine.h
           AsyncAudioValve.h AsyncAudioAmp.h AsyncAudioSelector.h
           AsyncAudioPassthrough.h AsyncAudioMixer.h AsyncAudioFifo.h
           AsyncAudioDebugger.h AsyncAudioPacer.h AsyncAudioReader.h
           AsyncAudioDecimator.h AsyncAudioInterpolator.h
           AsyncAudioStreamStateDetector.h AsyncAudioEncoder.h
           AsyncAudioDecoder.h AsyncAudioRecorder.h
           AsyncAudioJitterFifo.h AsyncAudioDeviceFactory.h
           AsyncAudioDevice.h)

set(LIBSRC AsyncAudioSource.cpp AsyncAudioSink.cpp
           AsyncAudioProcessor.cpp AsyncAudioCompressor.cpp
           AsyncAudioFilter.cpp fidlib.c AsyncAudioDevice.cpp
           AsyncAudioIO.cpp AsyncAudioSplitter.cpp
           AsyncAudioDelayLine.cpp AsyncAudioSelector.cpp
           AsyncAudioMixer.cpp AsyncAudioFifo.cpp AsyncAudioPacer.cpp
           AsyncAudioReader.cpp AsyncAudioDecimator.cpp
           AsyncAudioInterpolator.cpp AsyncAudioDecoder.cpp
           AsyncAudioEncoder.cpp AsyncAudioEncoderS16.cpp
           AsyncAudioDecoderS16.cpp AsyncAudioEncoderGsm.cpp
           AsyncAudioDecoderGsm.cpp AsyncAudioRecorder.cpp
           AsyncAudioDeviceFactory.cpp AsyncAudioJitterFifo.cpp
           AsyncAudioDeviceUDP.cpp)

if(SPEEX_FOUND)
  set(LIBSRC ${LIBSRC} AsyncAudioEncoderSpeex.cpp AsyncAudioDecoderSpeex.cpp)
endif(SPEEX_FOUND)

if(OPUS_FOUND)
  set(LIBSRC ${LIBSRC} AsyncAudioEncoderOpus.cpp AsyncAudioDecoderOpus.cpp)
endif(OPUS_FOUND)

if(USE_ALSA)
  set(LIBSRC ${LIBSRC} AsyncAudioDeviceAlsa.cpp)
  find_package(ALSA REQUIRED)
  set(LIBS ${LIBS} ${ALSA_LIBRARIES})
  include_directories(${ALSA_INCLUDE_DIRS})
endif(USE_ALSA)

if(USE_OSS)
  set(LIBSRC ${LIBSRC} AsyncAudioDeviceOSS.cpp)
endif(USE_OSS)

set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Set up additional defines
add_definitions(-DT_LINUX)

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
