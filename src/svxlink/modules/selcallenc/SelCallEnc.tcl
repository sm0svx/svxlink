###############################################################################
#
# SelCallEnc module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleSelCallEnc] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval SelCallEnc {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module

# Source the SelCall sending functions
sourceTclWithOverrides "SelCall.tcl"


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  variable variants

  Module::play_help

  # FIXME: We should not read the variants array directly from the module
  # implementation
  foreach variant_id [lsort [array names variants]] {
    playSilence 300
    playNumber $variant_id
    playSilence 100
    playMsg $variants($variant_id)
  }
}


proc play_standard {std} {
  playMsg $std
}


proc play_sel_call {cmd} {
  variable variants

  # FIXME: We should not read the variants array directly from the module
  # implementation
  if {[string range $cmd 0 1] > [array size variants]} {
    playMsg "operation_failed"
  } else {
    SelCall::setMode $variants([string range $cmd 0 1])
    SelCall::play [string range $cmd 2 end]
  }
}


# end of namespace
}

#
# This file has not been truncated
#
