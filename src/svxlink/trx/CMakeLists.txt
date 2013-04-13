# The name of the library
set(LIBNAME trx)

# Which include files to export to the global include directory
set(EXPINC Rx.h Tx.h NetTrxMsg.h LocalRx.h)

# What sources to compile for the library
set(LIBSRC
  ToneDetector.cpp SwDtmfDecoder.cpp Rx.cpp LocalRx.cpp
  SquelchVox.cpp SigLevDetNoise.cpp NetRx.cpp Voter.cpp
  Tx.cpp LocalTx.cpp DtmfEncoder.cpp NetTx.cpp
  NetTrxTcpClient.cpp DtmfDecoder.cpp HwDtmfDecoder.cpp
  S54sDtmfDecoder.cpp PttCtrl.cpp MultiTx.cpp
  SigLevDetTone.cpp Sel5Decoder.cpp SwSel5Decoder.cpp
  SquelchEvDev.cpp Macho.cpp
)

# Which other libraries this library depends on
#set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Find the GCrypt library
find_package(GCrypt REQUIRED)
set(LIBS ${LIBS} ${GCRYPT_LIBRARIES})
include_directories(${GCRYPT_INCLUDE_DIRS})
add_definitions(${GCRYPT_DEFINITIONS})

# Build only a static library
add_library(${LIBNAME} STATIC ${LIBSRC}
            ${CMAKE_CURRENT_BINARY_DIR}/version/SVXLINK.h)
target_link_libraries(${LIBNAME} ${LIBS})
add_version_target(SVXLINK)

# Install targets
install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
