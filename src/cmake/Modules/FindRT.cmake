# - Check for the presence of RT
#
# The following variables are set when RT is found:
#  RT_FOUND        = Set to true, if all components of RT
#                    have been found.
#  RT_INCLUDE_DIRS = Include path for the header files of RT
#  RT_LIBRARIES    = Link these to use RT

if (RT_LIBRARIES AND RT_INCLUDE_DIRS)
  # in cache already
  set(RT_FOUND TRUE)
else (RT_LIBRARIES AND RT_INCLUDE_DIRS)
  ## -------------------------------------------------------------------------
  ## Check for the header files

  find_path (RT_INCLUDE_DIR
    NAMES time.h
    PATHS /usr/local/include /usr/include ${CMAKE_EXTRA_INCLUDES}
    )

  ## -------------------------------------------------------------------------
  ## Check for the library

  find_library (RT_LIBRARY
    NAMES rt
    PATHS /usr/local/lib /usr/lib /lib ${CMAKE_EXTRA_LIBRARIES}
    )

  ## -------------------------------------------------------------------------
  ## Actions taken when all components have been found

  if (RT_LIBRARY)
    set(RT_FOUND TRUE)
  endif (RT_LIBRARY)

  set(RT_INCLUDE_DIRS ${RT_INCLUDE_DIR})

  if (RT_FOUND)
    set(RT_LIBRARIES ${RT_LIBRARIES} ${RT_LIBRARY})
  endif (RT_FOUND)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(RT
    DEFAULT_MSG
    RT_LIBRARIES 
    RT_INCLUDE_DIRS
    )

  mark_as_advanced (
    HAVE_RT
    RT_LIBRARIES
    RT_INCLUDES
    )
endif (RT_LIBRARIES AND RT_INCLUDE_DIRS)
