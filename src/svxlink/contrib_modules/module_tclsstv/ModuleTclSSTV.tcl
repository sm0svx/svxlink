###############################################################################
#
# Tcl module implementation
#
###############################################################################
 
#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTcl] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval TclSSTV {
 
#
# Check if this module is loaded in the current logic core
#
if {![info exists CFG_ID]} {
  return;
}
 
#
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]]
 
 
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
  ::processEvent "$module_name" "$ev"
}
 
 
 
#
# Executed when this module is being activated
#
proc activateInit {} {
  printInfo "Module activated"
 
 
 #::playMsg "TclSSTV" "commande";
 
}
 
 
 
#
# Executed when this module is being activated.
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
# Executed when this module is being deactivated.
#
proc deactivateCleanup {} {
  printInfo "Module deactivated"
 
}
 
 
#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  variable module_name;
  Module::timeout $module_name;
}
 
# Envoi audio SSTV
proc send_SSTV {} {
 
  printInfo "Lancement SSTV"
  ::playMsg "TclSSTV" "demarrage_sstv";
   exec wget http://webcam.langmatt.net/image.jpg --output-document=/tmp/phot.jpg --output-file=/tmp/log_wget.txt
   exec python /usr/bin/go_sstv.py
   exec pisstv /tmp/phot2.jpg 8000
   exec cp /tmp/phot2.jpg.wav "$langdir/TclSSTV/last_pict.wav
        ::playMsg "TclSSTV" "last_pict";
 
  ::playMsg "TclSSTV" "fin_image";
  printInfo "Fin transmission SSTV"
  return 1
 
}
 
###TEST Meteo
proc read_weather {} {
printInfo "Lecture meteo"
source "/usr/share/svxlink/modules.d/Meteo.tcl"
#playMsg "temperature_is"
playNumber $temp_bale ;
 
 
#set output [exec python /home/pi/code_jour/trait.py]
#source "/home/pi/code_jour/codes.tcl"
 
#    puts $output
#playNumber  $code_B
return 1
}
 
#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
#   char - The received DTMF digit
#   duration - The duration of the received DTMF digit
#
proc dtmfDigitReceived {char duration} {
  printInfo "DTMF digit $char received with duration $duration milliseconds"
 
}
 
 
#
# Executed when a DTMF command is received
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceived {cmd} {
  printInfo "DTMF command received: $cmd"
 
  if {$cmd == "0"} {
    processEvent "play_help"
  } elseif {$cmd == ""} {
    deactivateModule;
  }  elseif {$cmd == "1"} {
    send_SSTV;
  }  elseif {$cmd == "2"} {
    read_weather;
  }
  else {
    processEvent "unknown_command $cmd"
}
}
 
 
#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
#   cmd - The received DTMF command
#
proc dtmfCmdReceivedWhenIdle {cmd} {
  printInfo "DTMF command received when idle: $cmd"
 
}
 
 
#
# Executed when the squelch open or close.
#
#   is_open - Set to 1 if the squelch is open otherwise it's set to 0
#
proc squelchOpen {is_open} {
  if {$is_open} {set str "OPEN"} else { set str "CLOSED"}
  printInfo "The squelch is $str"
 
}
 
 
#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc allMsgsWritten {} {
  printInfo "allMsgsWritten called..."
 
}
 
 
 
# end of namespace
}
 
 
#
# This file has not been truncated
#