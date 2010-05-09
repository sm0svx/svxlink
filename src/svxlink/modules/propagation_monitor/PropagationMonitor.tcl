###############################################################################
#
# PropagationMonitor module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTcl] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval PropagationMonitor {

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
# Executed when a DTMF digit (0-9, A-F, *, #) is received
#
proc dtmf_digit_received {char duration} {
  printInfo "DTMF digit $char received with duration $duration milliseconds";
}


#
# Executed when a DTMF command is received
#
proc dtmf_cmd_received {cmd} {
  printInfo "DTMF command received: $cmd";
}


#
# Executed when a DTMF command is received in idle mode. That is, a command is
# received when this module has not been activated first.
#
proc dtmf_cmd_received_when_idle {cmd} {
  printInfo "DTMF command received while idle: $cmd";
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
proc all_msgs_written {} {
  #printInfo "all_msgs_written called...";
}


#
# Executed when the state of this module should be reported on the radio
# channel. The rules for when this function is called are:
#
# When a module is active:
# * At manual identification the status_report function for the active module is
#   called.
# * At periodic identification no status_report function is called.
#
# When no module is active:
# * At both manual and periodic (long variant) identification the status_report
#   function is called for all modules.
#
proc status_report {} {
  #printInfo "status_report called...";
}


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


proc play_alert_sound {} {
  for {set i 0} {$i < 3} {set i [expr $i + 1]} {
    playTone 440 500 100
    playTone 880 500 100
    playTone 440 500 100
    playTone 880 500 100
    playSilence 600
  }
  playSilence 1000
}


proc say_band {band} {
  if [regexp {^(\d+)(c?m)$} $band -> number unit] {
    if {[string length $number] == 2} {
      playTwoDigitNumber $number
    } else {
      playNumber $number
    }
    playMsg unit_$unit
  }
}


proc say_locator {loc} {
  if [regexp {^(\w\w)(?:(\d\d)(\w\w)?)?$} $loc -> part1 part2 part3] {
    spellWord $part1
    playNumber $part2
    spellWord $part3
  }
}


proc handle_dxrobot {msg_file} {
  set subject [subject_of_msg "$msg_file"]
  #puts $subject
  regexp {^\w+ (.*): (.*?)(?: - (.*))?$} $subject -> time alerttxt info
  set hour [clock format [clock scan $time] -format "%H"]
  set min [clock format [clock scan $time] -format "%M"]
  #puts "time=$hour:$min"
  #puts "alerttxt=$alerttxt"
  #puts "info=$info"

  play_alert_sound
  playSilence 500
  playTime $hour $min
  playSilence 200
  printInfo $subject
  if {$alerttxt == "VHF Aurora Alert"} {
    playMsg "aurora_alert"
    playSilence 500
    playMsg "aurora_alert"
  } elseif {$alerttxt == "144 MHz E-skip Alert"} {
    playMsg "eskip_alert"
    playSilence 500
    playMsg "eskip_alert"
    if [regexp {band open from (.+) to (.+)} $info -> from to] {
      playSilence 500
      playMsg "band_open_from"
      spellWord $from
      playSilence 100
      playMsg "to"
      playSilence 100
      spellWord $to
    }
  } else {
    printInfo "*** WARNING: Unknown alert text: \"$alerttxt\""
    playMsg "unknown"
  }
  playSilence 200
}


proc handle_vhfdx {msg_file} {
  set subject [subject_of_msg "$msg_file"]
  printInfo $subject

  # Example: Sporadic-E opening on 6m. Best estimated MUF 108 MHz above JN66
  set match [regexp \
	{^Sporadic-E opening on (\d+c?m)\. Best estimated MUF (\d+) MHz above (\w\w\d\d)$} \
	$subject -> band muf locator]
  if {$match} {
    play_alert_sound
    for {set i 0} {$i < 2} {set i [expr $i + 1]} {
      playMsg sporadic_e_opening
      say_band $band
      playSilence 500
      playMsg MUF
      playSilence 100
      playNumber $muf
      playMsg unit_MHz
      playSilence 200
      playMsg above
      playSilence 200
      say_locator $locator
      playSilence 1000
    }
    return
  }

  # Example: Possible Sporadic-E from JO89 on 6m. Try towards LQ28 (13�)
  set match [regexp \
	{^Possible Sporadic-E from (\w\w\d\d) on (\d+c?m)\. Try towards (\w\w\d\d) (\d+.)$} \
	$subject -> from_loc band to_loc direction]
  if {$match} {
    play_alert_sound
    for {set i 0} {$i < 2} {set i [expr $i + 1]} {
      playMsg possible_sporadic_e_opening
      say_band $band
      playSilence 100
      playMsg between
      playSilence 200
      say_locator $from_loc
      playSilence 200
      playMsg and
      playSilence 200
      say_locator $to_loc
      playSilence 1000
    }
    return
  }

  # Example: Multi-hop sporadic-E opening on 6m.
  set match [regexp \
	{^Multi-hop sporadic-E opening on (\d+c?m)\.$} \
	$subject -> band]
  if {$match} {
    play_alert_sound
    for {set i 0} {$i < 2} {set i [expr $i + 1]} {
      playMsg multi_hop
      playMsg sporadic_e_opening
      say_band $band
      playSilence 1000
    }
    return
  }

  # Example: Tropo opening on 2m. up to 998 km. between RZ6BU(KN84PV) and TA3AX(KN30IJ)
  set match [regexp \
	{^Tropo opening on (\d+c?m)\. up to (\d+) km. between (.*?)\((\w\w\d\d(?:\w\w)?)\) and (.*?)\((\w\w\d\d(?:\w\w)?)\)$} \
	$subject -> band range call1 loc1 call2 loc2]
  if {$match} {
    play_alert_sound
    for {set i 0} {$i < 2} {set i [expr $i + 1]} {
      playMsg tropo_opening
      say_band $band
      playSilence 200
      playMsg between
      say_locator $loc1
      playSilence 200
      playMsg and
      playSilence 200
      say_locator $loc2
      playSilence 1000
    }
    return
  }

  # Example: Aurora active on 6m. down to 57� of Lat. / SM7GVF(JO77GA)
  set match [regexp \
	{^Aurora active on (\d+c?m)\. down to (\d+).* of Lat. / (.*?)\((\w\w\d\d\w\w)\)$} \
	$subject -> band lat call loc]
  if {$match} {
    play_alert_sound
    for {set i 0} {$i < 2} {set i [expr $i + 1]} {
      playMsg aurora_opening
      say_band $band
      playSilence 200
      playMsg down_to_lat
      playNumber $lat
      playMsg unit_deg
      playSilence 1000
    }
    return
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
    #puts "$msg_file"
    handle_$dir "$msg_file"
    set target "[file dirname $msg_file]/archive/[file tail $msg_file]"
    file delete "$target"
    file rename "$msg_file" "$target"
  }
}


proc check_for_alerts {} {
  #printInfo "Checking for new Alerts"
  check_dir vhfdx
  check_dir dxrobot
}


if {![file exists $CFG_SPOOL_DIR/dxrobot/archive]} {
  file mkdir $CFG_SPOOL_DIR/dxrobot/archive
}

if {![file exists $CFG_SPOOL_DIR/vhfdx/archive]} {
  file mkdir $CFG_SPOOL_DIR/vhfdx/archive
}

append func $module_name "::check_for_alerts";
Logic::addTimerTickSubscriber $func;



# end of namespace
}


#
# This file has not been truncated
#
