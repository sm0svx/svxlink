set(EXPINC common.h)

# Copy exported include files to the global include directory
foreach(incfile ${EXPINC})
  expinc(${incfile})
endforeach(incfile)

# Install targets
install(FILES ${EXPINC} DESTINATION ${SVX_INCLUDE_INSTALL_DIR})
