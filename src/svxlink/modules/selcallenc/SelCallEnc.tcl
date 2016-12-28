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

#
# Check if this module is loaded in the current logic core
#
if {![info exists CFG_ID]} {
  return;
}

#
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]];

#
# An "overloaded" playMsg that eliminates the need to write the module name
# as the first argument.
#
#   msg - The message to play
#
proc playMsg {msg} {
  variable module_name
  ::playMsg $module_name $msg
}


#
# Executed when this module is being activated
#
proc activating_module {} {
  variable module_name
  Module::activating_module $module_name
}

#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  variable module_name
  Module::deactivating_module $module_name
}

#
# Exectuted when the inactivity timeout for this module has been expired.
#
proc timeout {} {
  variable module_name
  Module::timeout $module_name
}

#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  variable module_name
  variable variants

  Module::play_help $module_name
  
  # FIXME: We should not read the variants array directly from the module implementation
  foreach variant_id [lsort [array names variants]] {
    playSilence 300
    playNumber $variant_id
    playSilence 100
    playMsg $variants($variant_id)
  }
}


proc status_report {} {
  #printInfo "status report called..."
}


proc play_standard {std} {
  playMsg $std
}


proc play_sel_call {cmd} {
  variable variants

  # FIXME: We should not read the variants array directly from the module implementation
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
