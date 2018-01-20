###############################################################################
#
# Trx module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTrx] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Trx {

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

# The currently setup frequency
variable current_fq 0

#
# An "overloaded" playMsg that eliminates the need to write the module name
# as the first argument.
#
proc playMsg {msg} {
  variable module_name;
  ::playMsg $module_name $msg;
}


#
# A convenience function for printing out information prefixed by the
# module name
#
proc printInfo {msg} {
  variable module_name;
  puts "$module_name: $msg";
}


#
# Executed when this module is being activated
#
proc activating_module {} {
  variable module_name;
  Module::activating_module $module_name;
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  variable module_name;
  Module::deactivating_module $module_name;
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  variable module_name;
  Module::timeout $module_name;
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  variable module_name;
  Module::play_help $module_name;
}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  #printInfo "status_report called..."
  variable current_fq
  if {$current_fq > 0} {
    playFrequency $current_fq
  }
}


proc set_frequency {fq} {
  #printInfo "### Set transceiver frequency to $fq Hz"
  variable current_fq
  set current_fq $fq
  if {$fq > 0} {
    playFrequency $fq
    playMsg "ok"
  }
}


proc no_matching_band {cmd} {
  #printInfo "### No matching band for command: $cmd"
  spellNumber $cmd
  playMsg "operation_failed"
}


proc failed_to_set_trx {cmd rx_name tx_name} {
  #printInfo "### Failed to set transceiver: cmd=$cmd RX=$rx_name TX=$tx_name"
  spellNumber $cmd
  playMsg "operation_failed"
}


proc play_current_fq {} {
  variable current_fq
  if {$current_fq > 0} {
    playFrequency $current_fq
  } else {
    playMsg "not_active"
  }
}

# end of namespace
}


#
# This file has not been truncated
#
