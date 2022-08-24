set(LIBNAME svxmisc)
set(EXPINC common.h CppStdCompat.h)
set(LIBSRC common.cpp)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Build a static library
add_library(${LIBNAME} STATIC ${LIBSRC})
set_target_properties(${LIBNAME} PROPERTIES OUTPUT_NAME ${LIBNAME})
target_link_libraries(${LIBNAME} ${LIBS})

# Install targets
install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
install(FILES ${EXPINC} DESTINATION ${SVX_INCLUDE_INSTALL_DIR})
