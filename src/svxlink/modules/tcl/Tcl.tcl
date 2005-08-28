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
# Executed when this module is being activated
#
proc activating_module {} {
  Module::activating_module "Tcl";
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  Module::deactivating_module "Tcl";
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  Module::timeout "Tcl";
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  Module::play_help "Tcl";
}


#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
proc dtmf_digit_received {char} {
  puts "DTMF digit received: $char";
}


#
# Executed when a DTMF command is received
#
proc dtmf_cmd_received {cmd} {
  puts "DTMF command received: $cmd";
}


#
# Executed when the squelch open or close. If it's open is_open is set to 1,
# otherwise it's set to 0.
#
proc squelch_open {is_open} {
  if {$is_open} {set str "OPEN"} else { set str "CLOSED"};
  puts "ModuleTcl: The squelch is $str";
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
# NOTE: Does not work right now.
#
#proc all_msgs_written {} {
  #puts "ModuleTcl: all_msgs_written called...";
#}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  puts "ModuleTcl: status_report called...";
}




# end of namespace
}


#
# This file has not been truncated
#
