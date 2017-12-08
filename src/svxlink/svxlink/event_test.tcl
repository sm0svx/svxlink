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


proc activateModule {name} {
  global active_module;
  puts "--- ${name}::activating_module";
  ${name}::activating_module;
  puts "";
  set active_module $name;
}


proc deactivateModule {} {
  global active_module;
  puts "deactivateModule";
  ${active_module}::deactivating_module;
  set active_module "";
}


proc recordStart {filename} {
  global rec_file;
  puts "recordStart \"$filename\"";
  set rec_file [open $filename w];
}


proc recordStop {} {
  global rec_file;
  puts "recordStop";
  close $rec_file;
}


proc execTest {cmd} {
  puts "--- $cmd";
  eval $cmd;
  puts "";
}


proc SimplexLogic {} {
  puts "--- SimplexLogic::startup";
  SimplexLogic::startup;
  puts "";

  puts "--- SimplexLogic::no_such_module";
  SimplexLogic::no_such_module 1;
  puts "";

  puts "--- SimplexLogic::manual_identification";
  SimplexLogic::manual_identification;
  puts "";

  puts "--- SimplexLogic::send_send_courtesy_tone";
  SimplexLogic::send_send_courtesy_tone;
  puts "";

  #puts "--- SimplexLogic::no_such_module";
  #SimplexLogic::no_such_module 1;
  #puts "";

  puts "--- SimplexLogic::macro_empty";
  SimplexLogic::macro_empty;
  puts "";

  puts "--- SimplexLogic::macro_not_found";
  SimplexLogic::macro_not_found;
  puts "";

  puts "--- SimplexLogic::macro_syntax_error";
  SimplexLogic::macro_syntax_error;
  puts "";

  puts "--- SimplexLogic::macro_module_not_found";
  SimplexLogic::macro_module_not_found;
  puts "";

  puts "--- SimplexLogic::macro_module_activation_failed";
  SimplexLogic::macro_module_activation_failed;
  puts "";

  puts "--- SimplexLogic::macro_another_active_module";
  SimplexLogic::macro_another_active_module;
  puts "";

  puts "--- SimplexLogic::unknown_command";
  SimplexLogic::unknown_command 123;
  puts "";

  puts "--- SimplexLogic::command_failed";
  SimplexLogic::command_failed 123;
  puts "";

  puts "--- SimplexLogic::activating_link";
  SimplexLogic::activating_link "TEST";
  puts "";

  puts "--- SimplexLogic::deactivating_link";
  SimplexLogic::deactivating_link "TEST";
  puts "";

  puts "--- SimplexLogic::link_not_active";
  SimplexLogic::link_not_active "TEST";
  puts "";

  puts "--- SimplexLogic::link_already_active";
  SimplexLogic::link_already_active "TEST";
  puts "";

  puts "--- SimplexLogic::every_minute";
  SimplexLogic::every_minute;
  puts "";
}


proc DuplexLogic {} {
  puts "--- DuplexLogic::startup";
  DuplexLogic::startup;
  puts "";

  puts "--- DuplexLogic::no_such_module";
  DuplexLogic::no_such_module 1;
  puts "";

  puts "--- DuplexLogic::manual_identification";
  DuplexLogic::manual_identification;
  puts "";

  puts "--- DuplexLogic::send_send_courtesy_tone";
  DuplexLogic::send_send_courtesy_tone;
  puts "";

  #puts "--- DuplexLogic::no_such_module";
  #DuplexLogic::no_such_module 1;
  #puts "";

  puts "--- DuplexLogic::macro_empty";
  DuplexLogic::macro_empty;
  puts "";

  puts "--- DuplexLogic::macro_not_found";
  DuplexLogic::macro_not_found;
  puts "";

  puts "--- DuplexLogic::macro_syntax_error";
  DuplexLogic::macro_syntax_error;
  puts "";

  puts "--- DuplexLogic::macro_module_not_found";
  DuplexLogic::macro_module_not_found;
  puts "";

  puts "--- DuplexLogic::macro_module_activation_failed";
  DuplexLogic::macro_module_activation_failed;
  puts "";

  puts "--- DuplexLogic::macro_another_active_module";
  DuplexLogic::macro_another_active_module;
  puts "";

  puts "--- DuplexLogic::unknown_command";
  DuplexLogic::unknown_command 123;
  puts "";

  puts "--- DuplexLogic::command_failed";
  DuplexLogic::command_failed 123;
  puts "";

  puts "--- DuplexLogic::activating_link";
  DuplexLogic::activating_link "TEST";
  puts "";

  puts "--- DuplexLogic::deactivating_link";
  DuplexLogic::deactivating_link "TEST";
  puts "";

  puts "--- DuplexLogic::link_not_active";
  DuplexLogic::link_not_active "TEST";
  puts "";

  puts "--- DuplexLogic::link_already_active";
  DuplexLogic::link_already_active "TEST";
  puts "";

  puts "--- DuplexLogic::every_minute";
  DuplexLogic::every_minute;
  puts "";

  puts "--- DuplexLogic::repeater_up";
  DuplexLogic::repeater_up;
  puts "";
  
  puts "--- DuplexLogic::repeater_down";
  DuplexLogic::repeater_down;
  puts "";
  
  puts "--- DuplexLogic::repeater_idle";
  DuplexLogic::repeater_idle;
  puts "";
}


proc Help {} {
  puts "--- Help::activating_module";
  Help::activating_module;
  puts "";

  puts "--- Help::deactivating_module";
  Help::deactivating_module;
  puts "";

  puts "--- Help::timeout";
  Help::timeout;
  puts "";

  puts "--- Help::play_help";
  Help::play_help;
  puts "";

  puts "--- Help::choose_module";
  Help::choose_module [list 0 Help 1 Parrot 2 EchoLink];
  puts "";

  puts "--- Help::no_such_module";
  Help::no_such_module 1;
  puts "";
}


proc Parrot {} {
  puts "--- Parrot::activating_module";
  Parrot::activating_module;
  puts "";

  puts "--- Parrot::deactivating_module";
  Parrot::deactivating_module;
  puts "";

  puts "--- Parrot::timeout";
  Parrot::timeout;
  puts "";

  puts "--- Parrot::play_help";
  Parrot::play_help;
  puts "";

  puts "--- Parrot::spell_digits";
  Parrot::spell_digits "0123456789";
  puts "";
}


proc EchoLink {} {
  puts "--- EchoLink::activating_module";
  EchoLink::activating_module;
  puts "";

  puts "--- EchoLink::deactivating_module";
  EchoLink::deactivating_module;
  puts "";

  puts "--- EchoLink::timeout";
  EchoLink::timeout;
  puts "";

  puts "--- EchoLink::play_help";
  EchoLink::play_help;
  puts "";

  puts "--- EchoLink::list_connected_stations";
  EchoLink::list_connected_stations [list SM0SVX *ECHOTEST* SM3SVX-L SM3SVX-R];
  puts "";

  puts "--- EchoLink::directory_server_offline";
  EchoLink::directory_server_offline;
  puts "";

  puts "--- EchoLink::no_more_connections_allowed";
  EchoLink::no_more_connections_allowed;
  puts "";

  puts "--- EchoLink::status_report";
  EchoLink::status_report;
  puts "";

  puts "--- EchoLink::station_id_not_found";
  EchoLink::station_id_not_found 12345;
  puts "";

  puts "--- EchoLink::lookup_failed";
  EchoLink::lookup_failed 12345;
  puts "";

  puts "--- EchoLink::self_connect";
  EchoLink::self_connect;
  puts "";

  puts "--- EchoLink::already_connected_to";
  EchoLink::already_connected_to SM0SVX;
  puts "";

  puts "--- EchoLink::internal_error";
  EchoLink::internal_error;
  puts "";

  puts "--- EchoLink::connecting_to";
  EchoLink::connecting_to SM0SVX;
  puts "";

  puts "--- EchoLink_disconnected";
  EchoLink::disconnected SM0SVX;
  puts "";

  puts "--- EchoLink::remote_connected";
  EchoLink::remote_connected SM0SVX;
  puts "";

  puts "--- EchoLink::connected";
  EchoLink::connected;
  puts "";

  puts "--- EchoLink::link_inactivity_timeout";
  EchoLink::link_inactivity_timeout;
  puts "";

  puts "--- EchoLink::remote_greeting";
  EchoLink::remote_greeting;
  puts "";

  puts "--- EchoLink::reject_remote_connection";
  EchoLink::reject_remote_connection;
  puts "";

  puts "--- EchoLink::remote_timeout";
  EchoLink::remote_timeout;
  puts "";
}


proc TclVoiceMail {} {
  namespace eval Logic {
    variable CFG_CALLSIGN "TEST";
  }
  namespace eval TclVoiceMail {
    variable module_name [namespace tail [namespace current]];
    variable users;
    variable CFG_ID 3;
    variable ::Logic::CFG_CALLSIGN;

    set users(000) "call=TEST pass=123 email=root";

    activateModule $module_name;
    execTest "${module_name}::dtmf_digit_received 1";
    execTest "${module_name}::dtmf_cmd_received \"\"";

    activateModule $module_name;
    execTest "${module_name}::dtmf_cmd_received \"00012\"";
    execTest "${module_name}::dtmf_cmd_received \"000123\"";

    execTest "${module_name}::dtmf_cmd_received \"0\"";
    execTest "${module_name}::dtmf_cmd_received \"1\"";

    execTest "${module_name}::dtmf_cmd_received \"2\"";
    execTest "${module_name}::dtmf_cmd_received \"0000\"";
    execTest "${module_name}::squelch_open \"1\"";
    execTest "${module_name}::dtmf_cmd_received \"\"";
    execTest "${module_name}::squelch_open \"0\"";

    execTest "${module_name}::dtmf_cmd_received \"2\"";
    execTest "${module_name}::dtmf_cmd_received \"000\"";
    execTest "${module_name}::squelch_open \"1\"";
    execTest "${module_name}::dtmf_cmd_received \"\"";
    execTest "${module_name}::squelch_open \"0\"";

    execTest "${module_name}::dtmf_cmd_received \"2000\"";
    execTest "${module_name}::squelch_open \"1\"";
    execTest "${module_name}::squelch_open \"0\"";
    execTest "${module_name}::squelch_open \"1\"";
    execTest "${module_name}::squelch_open \"0\"";

    execTest "${module_name}::dtmf_cmd_received \"1\"";
    execTest "${module_name}::dtmf_cmd_received \"3\"";
    execTest "${module_name}::dtmf_cmd_received \"2\"";
    execTest "${module_name}::squelch_open \"1\"";
    execTest "${module_name}::squelch_open \"0\"";
    execTest "${module_name}::squelch_open \"1\"";
    execTest "${module_name}::squelch_open \"0\"";

    execTest "${module_name}::status_report";

    execTest "${module_name}::dtmf_cmd_received \"1\"";
    execTest "${module_name}::dtmf_cmd_received \"0\"";
    execTest "${module_name}::dtmf_cmd_received \"1\"";

    execTest "${module_name}::timeout";
    execTest "${module_name}::dtmf_cmd_received \"\"";

    file delete "$recdir/TEST";
  }
}


if {$argc < 1} {
  puts "Usage: $argv0 <path to events.tcl>";
  exit 1;
}

set mycall "SM0XXX";
set report_ctcss 136.5;
set active_module "EchoLink";
set script_path [lindex $argv 0];
set is_core_event_handler 1;

source [lindex $argv 0];

set EchoLink::num_connected_stations 2;

SimplexLogic;
DuplexLogic;
Help;
Parrot;
EchoLink;
TclVoiceMail;

