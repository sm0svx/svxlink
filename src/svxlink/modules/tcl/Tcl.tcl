###############################################################################
#
# Tcl module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTcl] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Tcl {

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
  Module::activating_module $module_name;
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  Module::deactivating_module $module_name;
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  Module::timeout $module_name;
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  Module::play_help $module_name;
}


#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
proc dtmf_digit_received {char} {
  printInfo "DTMF digit received: $char";
}


#
# Executed when a DTMF command is received
#
proc dtmf_cmd_received {cmd} {
  printInfo "DTMF command received: $cmd";
}


#
# Executed when the squelch open or close. If it's open is_open is set to 1,
# otherwise it's set to 0.
#
proc squelch_open {is_open} {
  if {$is_open} {set str "OPEN"} else { set str "CLOSED"};
  printInfo "The squelch is $str";
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
# NOTE: Does not work right now.
#
#proc all_msgs_written {} {
  #printInfo "all_msgs_written called...";
#}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  printInfo "status_report called...";
}




# end of namespace
}


#
# This file has not been truncated
#
