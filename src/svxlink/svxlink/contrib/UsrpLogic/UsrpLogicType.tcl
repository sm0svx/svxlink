###############################################################################
#
# UsrpLogic event handlers
#
# This file is sourced by all reflector logics and contains the TCL code that
# are in common to all reflector logics.
#
###############################################################################

# The namespace is automatically set to the logic core name so there is no need
# to change it unless you have a good reason
namespace eval ${::logic_name} {

# Source basic logic functionality. Mix in ("inherit") generic logic TCL code
sourceTclWithOverrides "LogicBase.tcl"
mixin Logic

#
# Checking to see if this is the correct logic core
#
#if {$logic_name != [namespace tail [namespace current]]} {
#  return;
#}


#
# Executed when an unknown command is received
#   cmd - The command string
#
proc unknown_command {cmd} {
}


#
# Executed when a received command fails
#
proc command_failed {cmd} {
}


#
# Executed each time a Usrp Meta frame is received
# and contains station data
#
proc usrp_stationdata_received {call tgid dmrid} {
  set dmr "";
  set tg "";
  if {$dmrid > 0} {
    set dmr "Dmr-ID $dmrid";
  }
  if {$tgid > 0} {
    set tg "TG# $tgid";
  }
  puts "Talker $call $dmr $tg";
}


#
# Executed when the own transmission starts
#
proc transmission_start {tg} {
  puts "Transmission starts (TG# $tg)";
}


#
# Executed when the own transmission stops
#
proc transmission_stop {tg} {
  puts "Transmission stops (TG# $tg)";
}


#
# Executed when the remote transmision has stopped
#
proc talker_stop {tg {call "unknown"}} {
  puts "Talker stop: $call TG# $tg";
}


#
# Executed when a switch between different modes has initiated
#
proc switch_to_mode {mode} {
  puts "Send request for $mode";
}


#
# Executed when the analog_bridge confirm to switch the mode
#
proc setting_mode {mode} {
  puts "New mode: $mode";
}


#
# Executed when json data has received, e.g.:
# {"call":"SP2ONG","name":"Waldek"}
#
proc usrp_jsondata_received {jsondata} {
  puts "JSON: $jsondata";
}

# end of namespace
}


#
# This file has not been truncated
#
