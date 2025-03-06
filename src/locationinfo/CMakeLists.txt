# The name of the library
set(LIBNAME locationinfo)

# Which include files to export to the global include directory
set(EXPINC LocationInfo.h AprsClient.h AprsTcpClient.h AprsUdpClient.h)

# What sources to compile for the library
set(LIBSRC LocationInfo.cpp AprsTcpClient.cpp AprsUdpClient.cpp)

# Which other libraries this library depends on
set(LIBS ${LIBS} asynccore)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Build only a static library
add_library(${LIBNAME} STATIC ${LIBSRC})
target_link_libraries(${LIBNAME} ${LIBS})

# Install targets
#install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
