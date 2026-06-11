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

# Unit tests: every *Test.cpp here is auto-registered (see SvxLinkAddTests).
# Adding a new test = drop in a new *Test.cpp file; no edits needed here.
svxlink_add_tests(LIBS ${LIBNAME})

# Install targets
install(TARGETS ${LIBNAME} DESTINATION ${LIB_INSTALL_DIR})
install(FILES ${EXPINC} DESTINATION ${SVX_INCLUDE_INSTALL_DIR})
