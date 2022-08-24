# - Check for the presence of DL
#
# The following variables are set when DL is found:
#  DL_FOUND        = Set to true, if all components of DL
#                    have been found.
#  DL_INCLUDE_DIRS = Include path for the header files of DL
#  DL_LIBRARIES    = Link these to use DL

if (DL_LIBRARIES AND DL_INCLUDE_DIRS)
  # in cache already
  set(DL_FOUND TRUE)
else (DL_LIBRARIES AND DL_INCLUDE_DIRS)
  ## -------------------------------------------------------------------------
  ## Check for the header files

  find_path (DL_INCLUDE_DIR
    NAMES dlfcn.h
    PATHS /usr/local/include /usr/include ${CMAKE_EXTRA_INCLUDES}
    )

  ## -------------------------------------------------------------------------
  ## Check for the library

  find_library (DL_LIBRARY
    NAMES dl
    PATHS /usr/local/lib /usr/lib /lib ${CMAKE_EXTRA_LIBRARIES}
    )

  ## -------------------------------------------------------------------------
  ## Actions taken when all components have been found

  if (DL_LIBRARY)
    set(DL_FOUND TRUE)
  endif (DL_LIBRARY)

  set(DL_INCLUDE_DIRS ${DL_INCLUDE_DIR})

  if (DL_FOUND)
    set(DL_LIBRARIES ${DL_LIBRARIES} ${DL_LIBRARY})
  endif (DL_FOUND)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(DL
    DEFAULT_MSG
    DL_LIBRARIES 
    DL_INCLUDE_DIRS
    )

  mark_as_advanced (
    HAVE_DL
    DL_LIBRARIES
    DL_INCLUDES
    )
endif (DL_LIBRARIES AND DL_INCLUDE_DIRS)
