###############################################################################
#
# Local functions
#
###############################################################################

set basedir "/home/blomman/.svxlink/sounds";

proc playMsg {context msg} {
  global basedir;
  if [file exists "$basedir/$context/$msg.raw"] {
    playFile "$basedir/$context/$msg.raw";
  } else {
    playFile "$basedir/Default/$msg.raw";
  }
}



###############################################################################
#
# SimplexLogic event handlers
#
###############################################################################

proc SimplexLogic_startup {} {
  global mycall;
  playMsg "Core" "online";
  spellWord $mycall;
}


proc SimplexLogic_no_such_module {module_id} {
  playMsg "Core" "no_such_module";
  playNumber $module_id;
}


proc SimplexLogic_manual_identification {} {
  global mycall;
  global report_ctcss;
  global active_module;
  
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
    reportActiveModuleState;
    playSilence 250;
  }
  #playFile "";
}


proc SimplexLogic_send_rgr_sound {} {
  playMsg "Core" "blip";
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
    reportActiveModuleState;
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
  spellWord $mycall;
  playMsg "Core" "repeater";
}


proc RepeaterLogic_repeater_up {} {
  global mycall;
  global active_module;
  
  playFile "/home/blomman/.svxlink/extra-sounds/attention.raw";
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
  
  spellWord $mycall;
  playMsg "Core" "repeater";
  playSilence 250;

  playFile "/home/blomman/.svxlink/extra-sounds/beback.raw";
}


proc RepeaterLogic_repeater_idle {} {
  playMsg "Core" "repeater_idle";
}



###############################################################################
#
# Help module event handlers
#
###############################################################################

proc Help_activating_module {} {
  playMsg "Core" "activating_module";
  playSilence 100;
  playMsg "Help" "name";
}


proc Help_deactivating_module {} {
  playMsg "Core" "deactivating_module";
  playSilence 100;
  playMsg "Help" "name";
}


proc Help_timeout {} {
  playMsg "Core" "timeout";
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


proc Help_play_help {} {
  playMsg "Help" "help";
}



###############################################################################
#
# Parrot module event handlers
#
###############################################################################

proc Parrot_activating_module {} {
  playMsg "Core" "activating_module";
  playSilence 100;
  playMsg "Parrot" "name";
}


proc Parrot_deactivating_module {} {
  playMsg "Core" "deactivating_module";
  playSilence 100;
  playMsg "Parrot" "name";
}


proc Parrot_timeout {} {
  playMsg "Core" "timeout";
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
  playMsg "Core" "activating_module";
  playSilence 100;
  playMsg "EchoLink" "name";
}


proc EchoLink_deactivating_module {} {
  playMsg "Core" "deactivating_module";
  playSilence 100;
  playMsg "EchoLink" "name";
}


proc EchoLink_timeout {} {
  playMsg "Core" "timeout";
}


proc spellCallsign {call} {
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
    spellCallsign $call;
  }
}


proc EchoLink_directory_server_offline {} {
  playMsg "EchoLink" "directory_server_offline";
}


proc EchoLink_no_more_connections_allowed {} {
  # FIXME: Change the message to something that makes more sense...
  playMsg "EchoLink" "link_busy";
}


proc EchoLink_status_report {connected_stations} {
  playNumber $connected_stations;
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
  spellCallsign $call;
}


proc EchoLink_internal_error {} {
  playMsg "EchoLink" "operation_failed";
}


proc EchoLink_connecting_to {call} {
  playMsg "EchoLink" "connecting_to";
  spellCallsign $call;
  playSilence 500;
}


proc EchoLink_disconnected {call} {
  spellCallsign $call;
  playMsg "EchoLink" "disconnected";
  playSilence 500;
}


proc EchoLink_link_inactivity_timeout {} {
  playMsg "EchoLink" "timeout";
}



###############################################################################
#
# Main program
#
###############################################################################

puts "Event handler script successfully loaded.";


