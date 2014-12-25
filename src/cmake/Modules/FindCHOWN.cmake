IF(CHOWN_TOOL)
  return()
endif(CHOWN_TOOL)

FIND_PROGRAM(CHOWN_TOOL
  NAMES chown
  PATHS /bin /usr/bin /usr/local/bin
  DOC "The path to the chown tool, which is used to change owner and group on file system objects"
  )

IF(CHOWN_TOOL)
  MESSAGE("-- Found chown: ${CHOWN_TOOL}")
ELSE(CHOWN_TOOL)
  MESSAGE("-- Unable to find the 'chown' program")
ENDIF(CHOWN_TOOL)

MARK_AS_ADVANCED(CHOWN_TOOL)
