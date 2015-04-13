# - Try to find rtl-sdr
# Once done this will define
#
#  RTLSDR_FOUND - system has rtl-sdr
#  RTLSDR_INCLUDE_DIRS - the rtl-sdr include directory
#  RTLSDR_LIBRARIES - Link these to use rtl-sdr
#  RTLSDR_DEFINITIONS - Compiler switches required for using rtl-sdr
#

if (RTLSDR_LIBRARIES AND RTLSDR_INCLUDE_DIRS)
  # In cache already
  set(RTLSDR_FOUND TRUE)
else (RTLSDR_LIBRARIES AND RTLSDR_INCLUDE_DIRS)

  find_path(RTLSDR_INCLUDE_DIR
    NAMES rtl-sdr.h
    PATHS ${RTLSDR_DIR}/include
  )

  find_library(RTLSDR_LIBRARY
    NAMES rtlsdr
    PATHS ${RTLSDR_DIR}/build/src
  )

  set(RTLSDR_INCLUDE_DIRS
    ${RTLSDR_INCLUDE_DIR}
  )

  if (RTLSDR_LIBRARY)
    set(RTLSDR_LIBRARIES ${RTLSDR_LIBRARIES} ${RTLSDR_LIBRARY})
  endif (RTLSDR_LIBRARY)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(RtlSdr
    DEFAULT_MSG
    RTLSDR_LIBRARIES 
    RTLSDR_INCLUDE_DIRS
  )

  # Show the RTLSDR_INCLUDE_DIR and RTLSDR_LIBRARY variables only in
  # the advanced view
  mark_as_advanced(RTLSDR_INCLUDE_DIR RTLSDR_LIBRARY)

endif (RTLSDR_LIBRARIES AND RTLSDR_INCLUDE_DIRS)
