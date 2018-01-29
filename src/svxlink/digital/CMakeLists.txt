# The name of the library
set(LIBNAME digital)

# Which include files to export to the global include directory
set(EXPINC
  AfskDemodulator.h Synchronizer.h HdlcDeframer.h AfskModulator.h HdlcFramer.h
)

# What sources to compile for the library
set(LIBSRC
  AfskDemodulator.cpp Synchronizer.cpp HdlcDeframer.cpp AfskModulator.cpp
  HdlcFramer.cpp Fcs.cpp
)

# Which other libraries this library depends on
#set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Build only a static library
add_library(${LIBNAME} STATIC ${LIBSRC})
target_link_libraries(${LIBNAME} ${LIBS})

add_executable(afsk_test afsk_test.cpp)
target_link_libraries(afsk_test asyncaudio asynccpp asynccore digital trx svxmisc)

add_executable(cal_sound_card cal_sound_card.cpp)
target_link_libraries(cal_sound_card asyncaudio asynccpp asynccore)
