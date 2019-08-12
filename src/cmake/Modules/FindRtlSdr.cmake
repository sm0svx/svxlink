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
  find_package(PkgConfig)
  if(CMAKE_VERSION VERSION_LESS 2.8.2)
    pkg_check_modules(PC_LIBUSB libusb-1.0)
    pkg_check_modules(PC_RTLSDR librtlsdr)
  else()
    pkg_check_modules(PC_LIBUSB QUIET libusb-1.0)
    pkg_check_modules(PC_RTLSDR QUIET librtlsdr)
  endif()

  find_path(LIBUSB_INCLUDE_DIR
    NAMES libusb.h
    PATHS ${PC_LIBUSB_INCLUDE_DIRS}
    DOC "libusb include directory path"
  )

  find_library(LIBUSB_LIBRARY
    NAMES usb-1.0
    PATHS ${PC_LIBUSB_LIBRARY_DIRS}
    DOC "libusb library path"
  )

  find_path(RTLSDR_INCLUDE_DIR
    NAMES rtl-sdr.h
    PATHS ${PC_RTLSDR_INCLUDE_DIRS} ${RTLSDR_DIR}/include
    DOC "librtlsdr include directory path"
  )

  find_library(RTLSDR_LIBRARY
    NAMES rtlsdr
    PATHS ${PC_RTLSDR_LIBRARY_DIRS} ${RTLSDR_DIR}/build/src ${RTLSDR_DIR}/src/.libs
    DOC "librtlsdr library path"
  )

  set(RTLSDR_INCLUDE_DIRS
    ${RTLSDR_INCLUDE_DIR} ${LIBUSB_INCLUDE_DIR}
  )

  if (RTLSDR_LIBRARY AND LIBUSB_LIBRARY)
    set(RTLSDR_LIBRARIES ${RTLSDR_LIBRARIES} ${RTLSDR_LIBRARY} ${LIBUSB_LIBRARY})
  endif (RTLSDR_LIBRARY AND LIBUSB_LIBRARY)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(RtlSdr
    DEFAULT_MSG
    RTLSDR_LIBRARIES 
    RTLSDR_INCLUDE_DIRS
  )

  # Show the RTLSDR_INCLUDE_DIR and RTLSDR_LIBRARY variables only in
  # the advanced view
  mark_as_advanced(RTLSDR_INCLUDE_DIR RTLSDR_LIBRARY LIBUSB_INCLUDE_DIR LIBUSB_LIBRARY)

endif (RTLSDR_LIBRARIES AND RTLSDR_INCLUDE_DIRS)
