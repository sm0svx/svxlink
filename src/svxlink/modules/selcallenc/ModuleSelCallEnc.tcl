###############################################################################
#
# SelCallEnc module implementation
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
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]];


#
# Type ID to selcall type name mapping
#
array set variants {
  "01" "ZVEI1"
  "02" "ZVEI2"
  "03" "ZVEI3"
  "04" "PZVEI"
  "05" "DZVEI"
  "06" "EEA"
  "07" "CCIR1"
  "08" "CCIR2"
  "09" "VDEW"
  "10" "CCITT"
  "11" "NATEL"
  "12" "EIA"
  "13" "EURO"
  "14" "MODAT"
  "15" "PDZVEI"
  "16" "PCCIR"
  "17" "AUTOA"
  "18" "QC2"
}


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
  ::processEvent "${::logic_name}::${module_name}" "$ev"
}


#
# Executed when this module is being activated
#
proc activateInit {} {
  printInfo "Module activated"
}


#
# Executed when this module is being deactivated.
#
proc deactivateCleanup {} {
  #printInfo "Module deactivated"
}


# The amplitude of the selcall audio. Set by function setAmplitude.
#variable amplitude

# Tone array for the selected mode. Set by function setMode.
#variable tones

# The length of the tones. Set by function setMode.
#variable tone_length

# The length of the first tone in a sequence. Set by function setMode.
#variable first_tone_length


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
  variable variants

  printInfo "DTMF command received: $cmd";

  if {$cmd == "0"} {
    processEvent "play_help"
  } elseif {[string length $cmd] == 2} {
    processEvent "play_standard $variants($cmd)"
  } elseif {$cmd != ""} {
    processEvent "play_sel_call $cmd"
  } else {
    deactivateModule
  }
}


#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceivedWhenIdle {cmd} {
  variable variants

  printInfo "DTMF command received received when idle: $cmd";

  if {$cmd == "0"} {
    processEvent "play_help"
  } elseif {[string length $cmd] == 2} {
    processEvent "play_standard $variants($cmd)"
  } elseif {$cmd != ""} {
    processEvent "play_sel_call $cmd"
  }
}


#
# Executed when the squelch open or close.
#
#   is_open - Set to 1 if the squelch is open otherwise it's set to 0
#
proc squelchOpen {is_open} {
  #if {$is_open} {set str "OPEN"} else {set str "CLOSED"};
  #printInfo "The squelch is $str"
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc allMsgsWritten {} {
}


# end of namespace
}

#
# This file has not been truncated
#

