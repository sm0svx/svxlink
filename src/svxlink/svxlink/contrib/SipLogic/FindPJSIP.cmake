include (FindPkgConfig)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(PJSIP REQUIRED libpjproject)
endif()

include_directories(${PJSIP_INCLUDE_DIRS})
link_directories(${PJSIP_LIBRARY_DIRS})

mark_as_advanced(pjsip_LIBRARY_DIRS pjsip_LIBRARIES)
