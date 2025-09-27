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
# Available variants are passed one per argument on the format ID=NAME.
#
override proc play_help {args} {
  $SUPER

  foreach var [lrange $args 1 end] {
    set v [split $var =]
    playSilence 300
    playNumber [lindex $v 0]
    playSilence 100
    playMsg [lindex $v 1]
  }
}


proc play_standard {std} {
  playMsg $std
}


proc play_sel_call {std digits} {
  SelCall::setMode $std
  SelCall::play $digits
}


# end of namespace
}

#
# This file has not been truncated
#
