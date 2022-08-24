# Locate libraries
# This module defines
# GSM_LIBRARY, the name of the library to link against
# GSM_FOUND, if false, do not try to link
# GSM_INCLUDE_DIR, where to find header
#

set( GSM_FOUND "NO" )

find_path( GSM_INCLUDE_DIR gsm.h
  HINTS
  PATH_SUFFIXES gsm
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include
  /usr/include
  /sw/include
  /opt/local/include
  /opt/csw/include
  /opt/include
  /mingw
)

find_library( GSM_LIBRARY
  NAME gsm
  HINTS
  PATH_SUFFIXES lib64 lib
  PATHS
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
  /mingw
)

mark_as_advanced(GSM_INCLUDE_DIR GSM_LIBRARY)

if(GSM_LIBRARY)
set( GSM_FOUND "YES" )
endif(GSM_LIBRARY)
