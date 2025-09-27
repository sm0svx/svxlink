
# Extract the module name from the current namespace
set module_name [namespace tail [namespace current]];

# Enable lookup of commands in the logic core namespace
namespace path ::${::logic_name}


#
# A convenience function for printing out information prefixed by the
# module name
#
#   msg - The message to print
#
proc printInfo {msg} {
  variable module_name
  puts "$module_name: $msg"
}


#
# A convenience function for calling an event handler
#
#   ev - The event string to execute
#
proc processEvent {ev} {
  variable module_name
  ::processEvent "${module_name}" "$ev"
}


#
# Executed when this module is being activated
#
proc activateInit {} {
  #printInfo "Module activated"
}


#
# Executed when this module is being deactivated.
#
proc deactivateCleanup {} {
  #printInfo "Module deactivated"
}


#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
#   char - The received DTMF digit
#   duration - The duration of the received DTMF digit
#
proc dtmfDigitReceived {char duration} {
  #printInfo "DTMF digit $char received with duration $duration milliseconds"
}


#
# Executed when a DTMF command is received
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceived {cmd} {
  #printInfo "DTMF command received: $cmd"
  if {$cmd == "0"} {
    processEvent "play_help"
  } elseif {$cmd == ""} {
    deactivateModule
  } else {
    processEvent "unknown_command ${cmd}"
  }
}


#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceivedWhenIdle {cmd} {
  #printInfo "DTMF command received while idle: $cmd";
}


#
# Executed when the squelch open or close.
#
#   is_open - Set to 1 if the squelch is open otherwise it's set to 0
#
proc squelchOpen {is_open} {
  #if {$is_open} {set str "OPEN"} else {set str "CLOSED"};
  #printInfo "The squelch is $str";
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc allMsgsWritten {} {
  #printInfo "all_msgs_written called...";
}


