###############################################################################
#
# PropagationMonitor module implementation
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModulePropagationMonitor] section in the configuration file. The name may
# be changed but it must be changed in both places.
#
namespace eval PropagationMonitor {

# Enable lookup of commands in the logic core namespace
namespace path ::${::logic_name}


#
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]];


#
# A convenience function for printing out information prefixed by the
# module name
#
proc printInfo {msg} {
  variable module_name;
  puts "$module_name: $msg";
}


#
# A convenience function for calling an event handler
#
proc processEvent {ev} {
  variable module_name
  ::processEvent "${::logic_name}::$module_name" "$ev"
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
  printInfo "Module deactivated"
}


#
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
proc dtmfDigitReceived {char duration} {
  printInfo "DTMF digit $char received with duration $duration milliseconds";
}


#
# Executed when a DTMF command is received
#
proc dtmfCmdReceived {cmd} {
  #printInfo "DTMF command received: $cmd";
  if {$cmd == "0"} {
    processEvent "play_help"
  } elseif {$cmd == ""} {
    deactivateModule
  } else {
    processEvent "unknown_command $cmd"
  }
}


#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
proc dtmfCmdReceivedWhenIdle {cmd} {
  #printInfo "DTMF command received while idle: $cmd";
}


#
# Executed when the squelch open or close. If it's open is_open is set to 1,
# otherwise it's set to 0.
#
proc squelchOpen {is_open} {
  if {$is_open} {set str "OPEN"} else {set str "CLOSED"};
  printInfo "The squelch is $str";
}


#
# Executed when all announcement messages has been played.
# Note that this function also may be called even if it wasn't this module
# that initiated the message playing.
#
proc allMsgsWritten {} {
  #printInfo "all_msgs_written called...";
}


#
# Extract the message subject from the given message file
#
proc subject_of_msg {msg_file} {
  set subject ""
  set chan [open "$msg_file"]
  while {[gets $chan line] >= 0} {
    if [regexp {^Subject: (.*)$} $line -> subject] {
      break
    }
  }
  close $chan
  return "$subject"
}


proc handle_dxrobot {msg_file} {
  set subject [subject_of_msg "$msg_file"]
  printInfo $subject
  
  if {![regexp {^(\w{3}) (\d{4}) (\w+): (.*?)(?: - (.*))?$} $subject -> dow time tz alerttxt info]} {
    printInfo "*** WARNING: Unknown message format for DXRobot alert in $msg_file: $subject"
    return
  }
  
  set hour [clock format [clock scan $time] -format "%H"]
  set min [clock format [clock scan $time] -format "%M"]
  
  # Example: Sat 0237 UTC: VHF AURORA WARNING
  if {$alerttxt == "VHF AURORA WARNING"} {
    processEvent "dxrobot_aurora_alert $hour $min"
    return
  }

  # Example: Wed 1102 UTC: 50 MHz E-skip Alert - 6 m Band open from OE4VIE to LA5YJ
  set match [regexp {^(\d+) MHz E-skip Alert$} $alerttxt -> fq_band]
  if {$match} {
    if [regexp {^(\d+) (\w+) Band open from (.+) to (.+)$} $info -> band_num band_unit from to] {
      set band "${band_num}${band_unit}"
      processEvent "dxrobot_eskip_alert $hour $min $band $from $to"
    } else {
      set band "[expr {300/$fq_band}]m"
      processEvent "dxrobot_eskip_alert $hour $min $band"
    }
    return
  }

  printInfo "*** WARNING: Unknown DXRobot alert encountered in $msg_file: $subject"
}


proc handle_vhfdx {msg_file} {
  set subject [subject_of_msg "$msg_file"]
  printInfo $subject

  # Example: Sporadic-E opening on 6m. Best estimated MUF 108 MHz above JN66
  set match [regexp \
	{^Sporadic-E opening on (\d+c?m)\. Best estimated MUF (\d+) MHz above (\w\w\d\d)$} \
	$subject -> band muf locator]
  if {$match} {
    processEvent "vhfdx_sporadic_e_opening $band $muf $locator"
    return
  }

  # Example: Possible Sporadic-E from JO89 on 6m. Try towards LQ28 (13º)
  # Example: Possible Sporadic-E from JO89 on 6m. Try towards JN61 (191 degrees)
  set match [regexp \
	{^Possible Sporadic-E from (\w\w\d\d) on (\d+c?m)\. Try towards (\w\w\d\d) \((\d+) degrees\)$} \
	$subject -> from_loc band to_loc direction]
  if {$match} {
    processEvent "vhfdx_possible_sporadic_e_opening $band $from_loc $to_loc $direction"
    return
  }

  # Example: Multi-hop sporadic-E opening on 6m.
  set match [regexp \
	{^Multi-hop sporadic-E opening on (\d+c?m)\.$} \
	$subject -> band]
  if {$match} {
    processEvent "vhfdx_multi_hop_sporadic_e_opening $band"
    return
  }

  # Example: Tropo opening on 2m. up to 998 km. between RZ6BU(KN84PV) and TA3AX(KN30IJ)
  set match [regexp \
	{^Tropo opening on (\d+c?m)\. up to (\d+) km. between (.*?)\((\w\w\d\d(?:\w\w)?)\) and (.*?)\((\w\w\d\d(?:\w\w)?)\)$} \
	$subject -> band range call1 loc1 call2 loc2]
  if {$match} {
    processEvent "vhfdx_tropo_opening $band $range $call1 $loc1 $call2 $loc2"
    return
  }

  # Example: Aurora active on 6m. down to 55 deg. of Lat. / LY2X/P(KO25MO)
  # Example: Aurora active on 6m. down to 56 deg. of Lat. / OZ4VV(JO46)
  # Example: Aurora active on 6m. down to 57º of Lat. / SM7GVF(JO77GA)
  set match [regexp \
	{^Aurora active on (\d+c?m)\. down to (\d+).* of Lat. / (.*?)\((\w\w\d\d(?:\w\w)?)\)$} \
	$subject -> band lat call loc]
  if {$match} {
    processEvent "vhfdx_aurora_opening $band $lat $call $loc"
    return
  }

  # Example: FAI active on 2m.
  set match [regexp \
	{^FAI active on (\d+c?m)\.$} \
	$subject -> band ]
  if {$match} {
    processEvent "vhfdx_fai_active $band"
    return;
  }

  # Example: TEP opening on 6m.
  set match [regexp \
	{^TEP opening on (\d+c?m)\.$} \
	$subject -> band ]
  if {$match} {
    processEvent "vhfdx_tep_opening $band"
    return;
  }

  # Example: F2 opening on 6m.
  set match [regexp \
	{^F2 opening on (\d+c?m)\.$} \
	$subject -> band ]
  if {$match} {
    processEvent "vhfdx_f2_opening $band"
    return;
  }

  # DX-Sherlock warnings configuration change
  # DX-Sherlock new password
  if {($subject == "DX-Sherlock warnings configuration change") ||
      ($subject == "DX-Sherlock new password")} {
    return;
  }

  printInfo "*** WARNING: Unknown VHFDX alert encountered in $msg_file: $subject"
}


proc check_dir {dir} {
  variable CFG_SPOOL_DIR

  foreach msg_file [glob -nocomplain -directory "$CFG_SPOOL_DIR/$dir" msg.*] {
    handle_$dir "$msg_file"
    set target "[file dirname $msg_file]/archive/[file tail $msg_file]"
    file delete "$target"
    file rename "$msg_file" "$target"
  }
}


proc check_for_alerts {} {
  check_dir vhfdx
  check_dir dxrobot
}


if {![file exists $CFG_SPOOL_DIR/dxrobot/archive]} {
  file mkdir $CFG_SPOOL_DIR/dxrobot/archive
}

if {![file exists $CFG_SPOOL_DIR/vhfdx/archive]} {
  file mkdir $CFG_SPOOL_DIR/vhfdx/archive
}

Logic::addMinuteTickSubscriber check_for_alerts



# end of namespace
}


#
# This file has not been truncated
#
