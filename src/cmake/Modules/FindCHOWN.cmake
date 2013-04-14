IF(CHOWN_TOOL)
  return()
endif(CHOWN_TOOL)

FIND_PROGRAM(CHOWN_TOOL
  NAMES chown
  PATHS /bin /usr/bin /usr/local/bin
  )

IF(CHOWN_TOOL)
  MESSAGE("-- Found chown: ${CHOWN_TOOL}")
ENDIF(CHOWN_TOOL)
