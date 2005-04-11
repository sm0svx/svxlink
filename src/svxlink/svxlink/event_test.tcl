#!/usr/bin/tclsh

proc playFile {filename} {
  puts "playFile(\"$filename\");";
  if {[file exists $filename] != 1} {
    puts "*** ERROR: file not found: $filename";
    exit 1;
  }
}


proc playTone {fq amp len} {
  puts "playTone($fq, $amp, $len);";
}


proc playSilence {milliseconds} {
  puts "playSilence($milliseconds);";
}


proc reportActiveModuleState {} {
  puts "reportActiveModuleState;";
}


proc SimplexLogic {} {
  puts "--- SimplexLogic_startup";
  SimplexLogic_startup;
  puts "";

  puts "--- SimplexLogic_no_such_module";
  SimplexLogic_no_such_module 1;
  puts "";

  puts "--- SimplexLogic_manual_identification";
  SimplexLogic_manual_identification;
  puts "";

  puts "--- SimplexLogic_send_rgr_sound";
  SimplexLogic_send_rgr_sound;
  puts "";

  puts "--- SimplexLogic_no_such_module";
  SimplexLogic_no_such_module 1;
  puts "";

  puts "--- SimplexLogic_macro_empty";
  SimplexLogic_macro_empty;
  puts "";

  puts "--- SimplexLogic_macro_not_found";
  SimplexLogic_macro_not_found;
  puts "";

  puts "--- SimplexLogic_macro_syntax_error";
  SimplexLogic_macro_syntax_error;
  puts "";

  puts "--- SimplexLogic_macro_module_not_found";
  SimplexLogic_macro_module_not_found;
  puts "";

  puts "--- SimplexLogic_macro_module_activation_failed";
  SimplexLogic_macro_module_activation_failed;
  puts "";

  puts "--- SimplexLogic_macro_another_active_module";
  SimplexLogic_macro_another_active_module;
  puts "";

  puts "--- SimplexLogic_periodic_identify";
  SimplexLogic_periodic_identify;
  puts "";
}


proc RepeaterLogic {} {
  puts "--- RepeaterLogic_startup";
  RepeaterLogic_startup;
  puts "";

  puts "--- RepeaterLogic_no_such_module";
  RepeaterLogic_no_such_module 1;
  puts "";

  puts "--- RepeaterLogic_manual_identification";
  RepeaterLogic_manual_identification;
  puts "";

  puts "--- RepeaterLogic_send_rgr_sound";
  RepeaterLogic_send_rgr_sound;
  puts "";

  puts "--- RepeaterLogic_no_such_module";
  RepeaterLogic_no_such_module 1;
  puts "";

  puts "--- RepeaterLogic_macro_empty";
  RepeaterLogic_macro_empty;
  puts "";

  puts "--- RepeaterLogic_macro_not_found";
  RepeaterLogic_macro_not_found;
  puts "";

  puts "--- RepeaterLogic_macro_syntax_error";
  RepeaterLogic_macro_syntax_error;
  puts "";

  puts "--- RepeaterLogic_macro_module_not_found";
  RepeaterLogic_macro_module_not_found;
  puts "";

  puts "--- RepeaterLogic_macro_module_activation_failed";
  RepeaterLogic_macro_module_activation_failed;
  puts "";

  puts "--- RepeaterLogic_macro_another_active_module";
  RepeaterLogic_macro_another_active_module;
  puts "";

  puts "--- RepeaterLogic_periodic_identify";
  RepeaterLogic_periodic_identify;
  puts "";
  
  puts "--- RepeaterLogic_repeater_up";
  RepeaterLogic_repeater_up;
  puts "";
  
  puts "--- RepeaterLogic_repeater_down";
  RepeaterLogic_repeater_down;
  puts "";
  
  puts "--- RepeaterLogic_repeater_idle";
  RepeaterLogic_repeater_idle;
  puts "";
}


proc Help {} {
  puts "--- Help_activating_module";
  Help_activating_module;
  puts "";

  puts "--- Help_deactivating_module";
  Help_deactivating_module;
  puts "";

  puts "--- Help_timeout";
  Help_timeout;
  puts "";

  puts "--- Help_play_help";
  Help_play_help;
  puts "";

  puts "--- Help_choose_module";
  Help_choose_module [list 0 Help 1 Parrot 2 EchoLink];
  puts "";

  puts "--- Help_no_such_module";
  Help_no_such_module 1;
  puts "";

  puts "--- Help_play_help";
  Help_play_help;
  puts "";
}


proc Parrot {} {
  puts "--- Parrot_activating_module";
  Parrot_activating_module;
  puts "";

  puts "--- Parrot_deactivating_module";
  Parrot_deactivating_module;
  puts "";

  puts "--- Parrot_timeout";
  Parrot_timeout;
  puts "";

  puts "--- Parrot_play_help";
  Parrot_play_help;
  puts "";

  puts "--- Parrot_spell_digits";
  Parrot_spell_digits "0123456789";
  puts "";
}


proc EchoLink {} {
  puts "--- EchoLink_activating_module";
  EchoLink_activating_module;
  puts "";

  puts "--- EchoLink_deactivating_module";
  EchoLink_deactivating_module;
  puts "";

  puts "--- EchoLink_timeout";
  EchoLink_timeout;
  puts "";

  puts "--- EchoLink_play_help";
  EchoLink_play_help;
  puts "";

  puts "--- EchoLink_list_connected_stations";
  EchoLink_list_connected_stations [list SM0SVX *ECHOTEST* SM3SVX-L SM3SVX-R];
  puts "";

  puts "--- EchoLink_directory_server_offline";
  EchoLink_directory_server_offline;
  puts "";

  puts "--- EchoLink_no_more_connections_allowed";
  EchoLink_no_more_connections_allowed;
  puts "";

  puts "--- EchoLink_status_report";
  EchoLink_status_report;
  puts "";

  puts "--- EchoLink_station_id_not_found";
  EchoLink_station_id_not_found 12345;
  puts "";

  puts "--- EchoLink_lookup_failed";
  EchoLink_lookup_failed 12345;
  puts "";

  puts "--- EchoLink_self_connect";
  EchoLink_self_connect;
  puts "";

  puts "--- EchoLink_already_connected_to";
  EchoLink_already_connected_to SM0SVX;
  puts "";

  puts "--- EchoLink_internal_error";
  EchoLink_internal_error;
  puts "";

  puts "--- EchoLink_connecting_to";
  EchoLink_connecting_to SM0SVX;
  puts "";

  puts "--- EchoLink_disconnected";
  EchoLink_disconnected SM0SVX;
  puts "";

  puts "--- EchoLink_remote_connected";
  EchoLink_remote_connected SM0SVX;
  puts "";

  puts "--- EchoLink_disconnected";
  EchoLink_disconnected SM0SVX;
  puts "";

  puts "--- EchoLink_connected";
  EchoLink_connected;
  puts "";

  puts "--- EchoLink_remote_greeting";
  EchoLink_remote_greeting;
  puts "";

  puts "--- EchoLink_reject_remote_connection";
  EchoLink_reject_remote_connection;
  puts "";

  puts "--- EchoLink_remote_timeout";
  EchoLink_remote_timeout;
  puts "";
}

if {$argc < 1} {
  puts "Usage: $argv0 <path to events.tcl>";
  exit 1;
}

set mycall "SM0XXX";
set report_ctcss 136.5;
set active_module "EchoLink";
set EchoLink_connected_stations 2;
set script_path [lindex $argv 0];
set is_core_event_handler 1;

source [lindex $argv 0];

SimplexLogic;
RepeaterLogic;
Help;
Parrot;
EchoLink;

