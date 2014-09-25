FIND_PROGRAM(GROFF_TOOL
  NAMES groff
  PATHS /bin /usr/bin /usr/local/bin
  DOC "The path to the groff document formatting tool"
  )


IF(NOT GROFF_TOOL)
  MESSAGE(FATAL_ERROR "Unable to find 'groff' program")
ENDIF(NOT GROFF_TOOL)

MARK_AS_ADVANCED(GROFF_TOOL)
