FIND_PROGRAM(GZIP_TOOL
  NAMES gzip
  PATHS /bin /usr/bin /usr/local/bin
  )


IF(NOT GZIP_TOOL)
  MESSAGE(FATAL_ERROR "Unable to find 'gzip' program")
ENDIF(NOT GZIP_TOOL)
