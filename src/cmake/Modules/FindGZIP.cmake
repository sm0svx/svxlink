FIND_PROGRAM(GZIP_TOOL
  NAMES gzip
  PATHS /bin /usr/bin /usr/local/bin
  DOC "The path to the gzip file compression tool"
  )


IF(NOT GZIP_TOOL)
  MESSAGE(FATAL_ERROR "Unable to find 'gzip' program")
ENDIF(NOT GZIP_TOOL)

MARK_AS_ADVANCED(GZIP_TOOL)
