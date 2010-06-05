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
# An "overloaded" playMsg that eliminiates the need to write the module name
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
# Exectuted when the inactivity timeout for this module has been expired.
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

# The amplitude of the selcall audio. Set by function setAmplitude.
#variable amplitude;

# Tone array for the selected mode. Set by function setMode.
#variable tones;

# The length of the tones. Set by function setMode.
#variable tone_length;

# The length of the first tone in a sequence. Set by function setMode.
#variable first_tone_length;


#
# Executed when a DTMF digit (0-9, A-D, *, #) is received
#
proc dtmf_digit_received {char duration} {
  #printInfo "DTMF digit $char received with duration $duration milliseconds";
}

#
# Executed when a DTMF command  is received
#
proc dtmf_cmd_received {cmd} {
  printInfo "DTMF command received: $cmd";
  if {$cmd == "0"} {
    playMsg "help";
  } elseif {$cmd != ""} {
    playSelCall $cmd;
  } else {
    deactivateModule;
  }
}

proc dtmf_cmd_received_when_idle {cmd} {
  printInfo "DTMF command received received when idle: $cmd";
  if {$cmd == "0"} {
    playMsg "help";
  } elseif {$cmd != ""} {
    playSelCall $cmd;
  }
}

proc all_msgs_written {} {
}

proc status_report {} {
  #printInfo "status report called..."
}

proc squelch_open {is_open} {
  #if {$is_open} {set str "OPEN"} else {set str "CLOSED"};
  #printInfo "The squelch is $str";
}

proc playSelCall {cmd} {
  set variant(01) "ZVEI1";
  set variant(02) "ZVEI2";
  set variant(03) "ZVEI3";
  set variant(04) "PZVEI";
  set variant(05) "DZVEI";
  set variant(06) "EEA";
  set variant(07) "CCIR1";
  set variant(08) "CCIR2";
  set variant(09) "VDEW";
  set variant(10) "CCITT";
  set variant(11) "NATEL";
  set variant(12) "EIA";
  set variant(13) "EURO";
  set variant(14) "MODAT";
  set variant(15) "PDZVEI";
  set variant(16) "PCCIR";
  set variant(17) "AUTOA";
  if {[string range $cmd 0 1] > 16 } {
    playMsg "operation_failed";
  } else {
    SelCall::setMode $variant([string range $cmd 0 1]);
    SelCall::play [string range $cmd 2 end];
  }
}


# end of namespace
}

#
# This file has not been truncated
#

