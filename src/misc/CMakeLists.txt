set(LIBNAME svxmisc)
set(EXPINC common.h CppStdCompat.h LogWriter.h)
set(LIBSRC common.cpp LogWriter.cpp)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Build a static library
add_library(${LIBNAME} STATIC ${LIBSRC})
set_target_properties(${LIBNAME} PROPERTIES OUTPUT_NAME ${LIBNAME})
target_link_libraries(${LIBNAME} ${LIBS})

# Unit test for the common string utilities
add_executable(CommonTest CommonTest.cpp)
target_link_libraries(CommonTest ${LIBNAME})
add_test(NAME CommonTest COMMAND CommonTest)

# Install targets
install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
install(FILES ${EXPINC} DESTINATION ${SVX_INCLUDE_INSTALL_DIR})
