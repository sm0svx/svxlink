IF(GROFF_TOOL)
  return()
endif(GROFF_TOOL)

FIND_PROGRAM(GROFF_TOOL
  NAMES groff
  PATHS /bin /usr/bin /usr/local/bin
  DOC "The path to the groff document formatting tool"
  )

IF(GROFF_TOOL)
  MESSAGE("-- Found groff: ${GROFF_TOOL}")
ELSE(GROFF_TOOL)
  MESSAGE(FATAL_ERROR "Unable to find the 'groff' program which is used to build the UNIX manual pages")
ENDIF(GROFF_TOOL)

MARK_AS_ADVANCED(GROFF_TOOL)
