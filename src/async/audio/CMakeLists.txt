option(USE_ALSA "Alsa audio support" ON)
option(USE_OSS "OSS audio support" ON)

# Find Speex
find_package(Speex)
if(Speex_FOUND)
  if(DEFINED Speex_VERSION_MAJOR)
    include_directories(${Speex_INCLUDE_DIRS})
    add_definitions(${Speex_DEFINITIONS})
    add_definitions("-DSPEEX_MAJOR=${Speex_VERSION_MAJOR}")
    set(LIBS ${LIBS} ${Speex_LIBRARIES})
  else()
    message(WARNING
      "Found Speex but version could not be resolved. "
      "Will proceed without Speex.")
  endif()
else(Speex_FOUND)
  message("--   Speex is an optional dependency. The build will complete")
  message("--   without it but support for the Speex audio codec will")
  message("--   be unavailable.")
endif(Speex_FOUND)

# Find Opus
find_package(Opus)
if(Opus_FOUND)
  if(DEFINED Opus_VERSION_MAJOR)
    include_directories(${Opus_INCLUDE_DIRS})
    add_definitions(${Opus_DEFINITIONS})
    add_definitions("-DOPUS_MAJOR=${Opus_VERSION_MAJOR}")
    set(LIBS ${LIBS} ${Opus_LIBRARIES})
  else()
    message(WARNING
      "Found Opus but version could not be resolved. "
      "Will proceed without Opus.")
  endif()
else(Opus_FOUND)
  message("--   Opus is an optional dependency. The build will complete")
  message("--   without it but support for the Opus audio codec will")
  message("--   be unavailable.")
endif(Opus_FOUND)

# Find OGG
find_package(OGG)
if(OGG_FOUND)
  if(DEFINED OGG_VERSION_MAJOR)
    include_directories(${OGG_INCLUDE_DIRS})
    add_definitions(${OGG_DEFINITIONS})
    add_definitions("-DOGG_MAJOR=${OGG_VERSION_MAJOR}")
    set(LIBS ${LIBS} ${OGG_LIBRARIES})
  else()
    message(WARNING
      "Found OGG but version could not be resolved. "
      "Will proceed without Opus.")
  endif()
else(OGG_FOUND)
  message("--   OGG is an optional dependency. The build will complete")
  message("--   without it but support for the OGG audio format will")
  message("--   be unavailable.")
endif(OGG_FOUND)

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
           AsyncAudioDevice.h AsyncAudioNoiseAdder.h AsyncAudioGenerator.h
           AsyncAudioFsf.h AsyncAudioContainer.h AsyncAudioContainerWav.h
           AsyncAudioContainerPcm.h
           )

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
           AsyncAudioDeviceUDP.cpp AsyncAudioNoiseAdder.cpp
           AsyncAudioFsf.cpp AsyncAudioContainer.cpp AsyncAudioContainerWav.cpp
           AsyncAudioContainerPcm.cpp
           )

if(Speex_FOUND)
  set(LIBSRC ${LIBSRC} AsyncAudioEncoderSpeex.cpp AsyncAudioDecoderSpeex.cpp)
endif(Speex_FOUND)

if(Opus_FOUND)
  set(LIBSRC ${LIBSRC} AsyncAudioEncoderOpus.cpp AsyncAudioDecoderOpus.cpp)
endif(Opus_FOUND)

if(OGG_FOUND AND Opus_FOUND)
  set(EXPINC ${EXPINC} AsyncAudioContainerOpus.h)
  set(LIBSRC ${LIBSRC} AsyncAudioContainerOpus.cpp)
endif(OGG_FOUND AND Opus_FOUND)

if(LADSPA_FOUND)
  set(EXPINC ${EXPINC} AsyncAudioLADSPAPlugin.h)
  set(LIBSRC ${LIBSRC} AsyncAudioLADSPAPlugin.cpp)
endif(LADSPA_FOUND)

if(USE_ALSA)
  set(LIBSRC ${LIBSRC} AsyncAudioDeviceAlsa.cpp)
  find_package(ALSA REQUIRED QUIET)
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
