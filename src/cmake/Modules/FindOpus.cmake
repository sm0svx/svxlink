# Find the opus library and include directory
#
#  OPUS_INCLUDE_DIRS   - The directory where opus.h can be found
#  OPUS_LIBRARIES      - Libraries to link with to use opus
#  OPUS_FOUND          - Set to true if the opus library is found

# Try to find the directory where the opus.h header file is located
find_path(OPUS_INCLUDE_DIR
  NAMES opus.h
  PATH_SUFFIXES opus
  DOC "Opus include directory"
)
mark_as_advanced(OPUS_INCLUDE_DIR)

# Try to find the opus library
find_library(OPUS_LIBRARY
  NAMES opus
  DOC "Opus library path"
)
mark_as_advanced(OPUS_LIBRARY)

# Handle the QUIETLY and REQUIRED arguments and set OPUS_FOUND to TRUE if 
# all listed variables are TRUE
include(${CMAKE_ROOT}/Modules/FindPackageHandleStandardArgs.cmake)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(
  Opus DEFAULT_MSG OPUS_LIBRARY OPUS_INCLUDE_DIR
)

set(OPUS_LIBRARIES ${OPUS_LIBRARY})
set(OPUS_INCLUDE_DIRS ${OPUS_INCLUDE_DIR})
