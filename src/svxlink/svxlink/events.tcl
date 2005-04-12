###############################################################################
#
# Local functions
#
###############################################################################

proc playMsg {context msg} {
  global basedir;
  if [file exists "$basedir/$context/$msg.raw"] {
    playFile "$basedir/$context/$msg.raw";
  } else {
    playFile "$basedir/Default/$msg.raw";
  }
}


proc spellWord {word} {
  set word [string tolower $word];
  for {set i 0} {$i < [string length $word]} {set i [expr $i + 1]} {
    playMsg "Default" "phonetic_[string index $word $i]";
  }
}


proc playNumber {number} {
  for {set i 0} {$i < [string length $number]} {set i [expr $i + 1]} {
    set ch [string index $number $i];
    if {$ch == "."} {
      playMsg "Default" "decimal";
    } else {
      playMsg "Default" "$ch";
    }
  }
}



###############################################################################
#
# SimplexLogic event handlers
#
###############################################################################

proc SimplexLogic_startup {} {
  #global mycall;
  #playMsg "Core" "online";
  #spellWord $mycall;
}


proc SimplexLogic_no_such_module {module_id} {
  playMsg "Core" "no_such_module";
  playNumber $module_id;
}


proc SimplexLogic_manual_identification {} {
  global mycall;
  global report_ctcss;
  global active_module;
  global prev_ident;
  
  set prev_ident [clock seconds];
  
  #playFile "";
  playMsg "Core" "online";
  spellWord $mycall;
  playSilence 250;
  if {$report_ctcss > 0} {
    playMsg "Core" "pl_is";
    playNumber $report_ctcss;
    playMsg "Core" "hz";
    playSilence 250;
  }
  if {$active_module != ""} {
    playMsg "Core" "active_module";
    playMsg $active_module "name";
    playSilence 250;
    append func $active_module "_status_report";
    if {"[info procs $func]" ne ""} {
      $func;
    }
    playSilence 250;
  }
  #playFile "";
}


proc SimplexLogic_send_rgr_sound {} {
  playTone 440 700 100;
}


proc SimplexLogic_no_such_module {module_id} {
  playNumber $module_id;
  playMsg "Core" "no_such_module";
}


proc SimplexLogic_macro_empty {} {
  playMsg "Core" "operation_failed";
}


proc SimplexLogic_macro_not_found {} {
  playMsg "Core" "operation_failed";
}


proc SimplexLogic_macro_syntax_error {} {
  playMsg "Core" "operation_failed";
}


proc SimplexLogic_macro_module_not_found {} {
  playMsg "Core" "operation_failed";
}


proc SimplexLogic_macro_module_activation_failed {} {
  playMsg "Core" "operation_failed";
}


proc SimplexLogic_macro_another_active_module {} {
  global active_module;
  
  playMsg "Core" "operation_failed";
  playMsg "Core" "active_module";
  playMsg $active_module "name";
}


proc SimplexLogic_periodic_identify {} {
  global mycall;
  global prev_ident;
  global min_time_between_ident;
  
  set now [clock seconds];
  if {$now-$prev_ident < $min_time_between_ident} {
    return;
  }
  set prev_ident $now;

  spellWord $mycall;
}





###############################################################################
#
# RepeaterLogic event handlers
#
###############################################################################

proc RepeaterLogic_startup {} {
  #global mycall;
  #playMsg "Core" "online";
  #spellWord $mycall;
  #playMsg "Core" "repeater";
}


proc RepeaterLogic_no_such_module {module_id} {
  SimplexLogic_no_such_module $module_id;
}


proc RepeaterLogic_manual_identification {} {
  global mycall;
  global report_ctcss;
  global active_module;
  global prev_ident;
  
  set prev_ident [clock seconds];
  
  playMsg "Core" "online";
  spellWord $mycall;
  playMsg "Core" "repeater";
  playSilence 250;
  if {$report_ctcss > 0} {
    playMsg "Core" "pl_is";
    playNumber $report_ctcss;
    playMsg "Core" "hz";
    playSilence 250;
  }
  if {$active_module != ""} {
    playMsg "Core" "active_module";
    playMsg $active_module "name";
    append func $active_module "_status_report";
    if {"[info procs $func]" ne ""} {
      $func;
    }
    playSilence 250;
  }
}


proc RepeaterLogic_send_rgr_sound {} {
  SimplexLogic_send_rgr_sound;
}


proc RepeaterLogic_no_such_module {module_id} {
  SimplexLogic_no_such_module $module_id;
}


proc RepeaterLogic_macro_empty {} {
  SimplexLogic_macro_empty;
}


proc RepeaterLogic_macro_not_found {} {
  SimplexLogic_macro_not_found;
}


proc RepeaterLogic_macro_syntax_error {} {
  SimplexLogic_macro_syntax_error;
}


proc RepeaterLogic_macro_module_not_found {} {
  SimplexLogic_macro_module_not_found;
}


proc RepeaterLogic_macro_module_activation_failed {} {
  SimplexLogic_macro_module_activation_failed;
}


proc RepeaterLogic_macro_another_active_module {} {
  SimplexLogic_macro_another_active_module;
}


proc RepeaterLogic_periodic_identify {} {
  global mycall;
  global prev_ident;
  global min_time_between_ident;
  
  set now [clock seconds];
  if {$now-$prev_ident < $min_time_between_ident} {
    return;
  }
  set prev_ident $now;
  
  spellWord $mycall;
  playMsg "Core" "repeater";
}


proc RepeaterLogic_repeater_up {} {
  global mycall;
  global active_module;
  global prev_ident;
  global min_time_between_ident;
  
  set now [clock seconds];
  if {$now-$prev_ident < $min_time_between_ident} {
    return;
  }
  set prev_ident $now;
  
  playMsg "../extra-sounds" "attention";
  playSilence 250;

  spellWord $mycall;
  playMsg "Core" "repeater";
  playSilence 250;

  if {$active_module != ""} {
    playMsg "Core" "active_module";
    playMsg $active_module "name";
  }
}


proc RepeaterLogic_repeater_down {} {
  global mycall;
  global prev_ident;
  global min_time_between_ident;
  
  set now [clock seconds];
  if {$now-$prev_ident < $min_time_between_ident} {
    playTone 400 900 50
    playSilence 100
    playTone 360 900 50
    playSilence 500
    return;
  }
  set prev_ident $now;
    
  spellWord $mycall;
  playMsg "Core" "repeater";
  playSilence 250;

  playMsg "../extra-sounds" "beback";
}


proc RepeaterLogic_repeater_idle {} {
  set iterations 8;
  set base 2;
  set max [expr {pow($base, $iterations)}];
  for {set i $iterations} {$i>0} {set i [expr $i - 1]} {
    playTone 1100 [expr {round(pow($base, $i) * 800 / $max)}] 100;
    playTone 1200 [expr {round(pow($base, $i) * 800 / $max)}] 100;
  }
}



###############################################################################
#
# Generic module event handlers
#
###############################################################################

proc Module_activating_module {module_name} {
  playMsg "Default" "activating_module";
  playSilence 100;
  playMsg $module_name "name";
  playSilence 200;
}


proc Module_deactivating_module {module_name} {
  playMsg "Default" "deactivating_module";
  playSilence 100;
  playMsg $module_name "name";
  playSilence 200;
}


proc Module_timeout {module_name} {
  playMsg "Default" "timeout";
  playSilence 100;
}


proc Module_play_help {module_name} {
  playMsg $module_name "help";
}




###############################################################################
#
# Help module event handlers
#
###############################################################################

proc Help_activating_module {} {
  Module_activating_module "Help";
}


proc Help_deactivating_module {} {
  Module_deactivating_module "Help";
}


proc Help_timeout {} {
  Module_timeout "Help";
}


proc Help_play_help {} {
  Module_play_help "Help";
}


proc Help_choose_module {module_list} {
  playMsg "Help" "choose_module";
  foreach {module_id module_name} "$module_list" {
    playNumber $module_id;
    playSilence 50;
    playMsg $module_name "name";
    playSilence 200;
  }
}


proc Help_no_such_module {module_id} {
  playNumber $module_id;
  playMsg "Help" "no_such_module";
}



###############################################################################
#
# Parrot module event handlers
#
###############################################################################

proc Parrot_activating_module {} {
  Module_activating_module "Parrot";
}


proc Parrot_deactivating_module {} {
  Module_deactivating_module "Parrot";
}


proc Parrot_timeout {} {
  Module_timeout "Parrot";
}


proc Parrot_play_help {} {
  Module_play_help "Parrot";
}


proc Parrot_spell_digits {digits} {
  spellWord $digits;
  playSilence 500;
}


###############################################################################
#
# EchoLink module event handlers
#
###############################################################################

proc EchoLink_activating_module {} {
  Module_activating_module "EchoLink";
}


proc EchoLink_deactivating_module {} {
  Module_deactivating_module "EchoLink";
}


proc EchoLink_timeout {} {
  Module_timeout "EchoLink";
}


proc EchoLink_play_help {} {
  Module_play_help "EchoLink";
}


proc spellEchoLinkCallsign {call} {
  global basedir;
  if [regexp {^(\w+)-L$} $call ignored callsign] {
    spellWord $callsign;
    playMsg "EchoLink" "link";
  } elseif [regexp {^(\w+)-R$} $call ignored callsign] {
    spellWord $callsign;
    playMsg "EchoLink" "repeater";
  } elseif [regexp {^\*(\w+)\*$} $call ignored name] {
    playMsg "EchoLink" "conference";
    playSilence 50;
    if [file exists "$basedir/EchoLink/conf-$call.raw"] {
      playFile "$basedir/EchoLink/conf-$call.raw";
    } else {
      spellWord $name;
    }
  } else {
    spellWord $call;
  }
}


proc EchoLink_list_connected_stations {connected_stations} {
  playNumber [llength $connected_stations];
  playSilence 50;
  playMsg "EchoLink" "connected_stations";
  foreach {call} "$connected_stations" {
    spellEchoLinkCallsign $call;
    playSilence 250;
  }
}


proc EchoLink_directory_server_offline {} {
  playMsg "EchoLink" "directory_server_offline";
}


proc EchoLink_no_more_connections_allowed {} {
  # FIXME: Change the message to something that makes more sense...
  playMsg "EchoLink" "link_busy";
}


proc EchoLink_status_report {} {
  global EchoLink_connected_stations;
  playNumber $EchoLink_connected_stations;
  playMsg "EchoLink" "connected_stations";
}


proc EchoLink_station_id_not_found {station_id} {
  playNumber $station_id;
  playMsg "EchoLink" "not_found";
}


proc EchoLink_lookup_failed {station_id} {
  playMsg "EchoLink" "operation_failed";
}


proc EchoLink_self_connect {} {
  playMsg "EchoLink" "operation_failed";
}


proc EchoLink_already_connected_to {call} {
  playMsg "EchoLink" "already_connected_to";
  playSilence 50;
  spellEchoLinkCallsign $call;
}


proc EchoLink_internal_error {} {
  playMsg "EchoLink" "operation_failed";
}


proc EchoLink_connecting_to {call} {
  playMsg "EchoLink" "connecting_to";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


proc EchoLink_disconnected {call} {
  spellEchoLinkCallsign $call;
  playMsg "EchoLink" "disconnected";
  playSilence 500;
}


proc EchoLink_remote_connected {call} {
  playMsg "EchoLink" "connected";
  spellEchoLinkCallsign $call;
  playSilence 500;
}


proc EchoLink_connected {} {
  playMsg "EchoLink" "connected";
  playSilence 500;
}


proc EchoLink_link_inactivity_timeout {} {
  playMsg "EchoLink" "timeout";
}


#
# The three events below are for remote EchoLink announcements.
#

proc EchoLink_remote_greeting {} {
  playSilence 1000;
  playMsg "EchoLink" "greeting";
}


proc EchoLink_reject_remote_connection {} {
  playSilence 1000;
  playMsg "EchoLink" "reject_connection";
  playSilence 1000;
}


proc EchoLink_remote_timeout {} {
  playMsg "EchoLink" "timeout";
  playSilence 1000;
}



###############################################################################
#
# Main program
#
###############################################################################

set basedir [file dirname $script_path];
set prev_ident 0;
set min_time_between_ident 120;

if [info exists is_core_event_handler] {
  puts "Event handler script successfully loaded.";
}

