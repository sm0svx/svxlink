###############################################################################
#
# UsrpLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[UsrpLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval UsrpLogic {

#
# Checking to see if this is the correct logic core
#
if {$logic_name != [namespace tail [namespace current]]} {
  return;
}


#
# Executed when an unknown command is received
#   cmd - The command string
#
proc unknown_command {cmd} {
  Logic::unknown_command $cmd;
}


#
# Executed when a received command fails
#
proc command_failed {cmd} {
  Logic::command_failed $cmd;
}


#
# Executed each time a Usrp Meta fram is received
#
proc usrp_metadata_received {call tg dmrid} {
  if {$dmrid > 0} {
    puts "Talker $call TG# $tg Dmr-ID $dmrid";
  } else {
    puts "Talker $call TG# $tg";
  }
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
proc talker_stop {call tg} {
  puts "Talker stop: $call TG# $tg";
}


# end of namespace
}


#
# This file has not been truncated
#
