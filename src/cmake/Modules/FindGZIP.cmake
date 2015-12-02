IF(GZIP_TOOL)
  return()
endif(GZIP_TOOL)

FIND_PROGRAM(GZIP_TOOL
  NAMES gzip
  PATHS /bin /usr/bin /usr/local/bin
  DOC "The path to the gzip file compression tool"
  )

IF(GZIP_TOOL)
  MESSAGE("-- Found gzip: ${GZIP_TOOL}")
ELSE(GZIP_TOOL)
  MESSAGE("-- Unable to find the 'gzip' program (missing: GZIP_TOOL)")
ENDIF(GZIP_TOOL)

MARK_AS_ADVANCED(GZIP_TOOL)
