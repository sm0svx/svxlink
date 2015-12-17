###############################################################################
#
# Frn module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleFrn] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Frn {

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
  printInfo "status_report called...";
}


#
# Executed when an entered command failed or have bad syntax.
#
proc command_failed {cmd} {
  spellWord $cmd;
  playMsg "operation_failed";
}
 
 
#
# Executed when an unrecognized command has been received.
#
proc unknown_command {cmd} {
  spellWord $cmd;
  playMsg "unknown_command";
}


#
# Executed when command to count nodes on the channel is called
#
proc count_clients {count_clients} {
  playNumber $count_clients;
  playSilence 50;
  playMsg "connected_clients";
  playSilence 250;
}


#
# Executed when the rf disable feature is activated or deactivated
#   status    - The current status of the feature (0=deactivated, 1=activated)
#   activate  - The requested new status of the feature
#               (0=deactivate, 1=activate)
#
proc rf_disable {status activate} {
  variable module_name;
  puts "$module_name: [expr {$activate ? "Activating" : "Deactivating"}]\
        listen only mode.";
  playMsg [expr {$activate ? "activating" : "deactivating"}];
  playMsg "rf_disable";
}

# end of namespace
}


#
# This file has not been truncated
#
