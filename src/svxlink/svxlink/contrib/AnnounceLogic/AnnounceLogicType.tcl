###############################################################################
#
# AnnounceLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[AnnounceLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval ${::logic_name} {

#
# Checking to see if this is the correct logic core
#
#if {$::logic_name != [namespace tail [namespace current]]} {
#  return;
#}


# Indicate that this is a core event handler
#variable is_core_event_handler 1


#
# Executed when the SvxLink software is started
#
proc startup {} {
  #append func $::logic_name "::checkPeriodicIdentify";
  #Logic::addMinuteTickSubscriber $func;
  #Logic::startup;
}


#
# Executed when a specified module could not be found
#
#proc no_such_module {module_id} {
#  Logic::no_such_module $module_id;
#}


#
# Executed when a manual identification is initiated with the * DTMF code
#
#proc manual_identification {} {
#  Logic::manual_identification;
#}


#
# Executed when the squelch just have closed and the RGR_SOUND_DELAY timer has
# expired.
#
#proc send_rgr_sound {} {
#  Logic::send_rgr_sound;
#}


#
# Executed when an empty macro command (i.e. D#) has been entered.
#
#proc macro_empty {} {
#  Logic::macro_empty;
#}


#
# Executed when an entered macro command could not be found
#
#proc macro_not_found {} {
#  Logic::macro_not_found;
#}


#
# Executed when a macro syntax error occurs (configuration error).
#
#proc macro_syntax_error {} {
#  Logic::macro_syntax_error;
#}


#
# Executed when the specified module in a macro command is not found
# (configuration error).
#
#proc macro_module_not_found {} {
#  Logic::macro_module_not_found;
#}


#
# Executed when the activation of the module specified in the macro command
# failed.
#
#proc macro_module_activation_failed {} {
#  Logic::macro_module_activation_failed;
#}


#
# Executed when a macro command is executed that requires a module to
# be activated but another module is already active.
#
#proc macro_another_active_module {} {
#  Logic::macro_another_active_module;
#}


#
# Executed when an unknown DTMF command is entered
#
#proc unknown_command {cmd} {
#  Logic::unknown_command $cmd;
#}


#
# Executed when an entered DTMF command failed
#
#proc command_failed {cmd} {
#  Logic::command_failed $cmd;
#}


#
# Executed when a link to another logic core is activated.
#   name  - The name of the link
#
#proc activating_link {name} {
#  Logic::activating_link $name;
#}


#
# Executed when a link to another logic core is deactivated.
#   name  - The name of the link
#
#proc deactivating_link {name} {
#  Logic::deactivating_link $name;
#}


#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#   name  - The name of the link
#
#proc link_not_active {name} {
#  Logic::link_not_active $name;
#}


#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#   name  - The name of the link
#
#proc link_already_active {name} {
#  Logic::link_already_active $name;
#}


#
# Executed once every whole minute
#
proc every_minute {} {
#  Logic::every_minute;
}


#
# Executed once every second
#
#proc every_second {} {
#  Logic::every_second;
#}


#
# Executed each time the transmitter is turned on or off
#
#proc transmit {is_on} {
#  Logic::transmit $is_on;
#}


#
# Executed each time the squelch is opened or closed
#
#proc squelch_open {rx_id is_open} {
#  Logic::squelch_open $rx_id $is_open;
#}


#
# Executed once every whole minute to check if it's time to identify
#
#proc checkPeriodicIdentify {} {
#  Logic::checkPeriodicIdentify;
#}


#
# Executed when a DTMF digit has been received
#   digit     - The detected DTMF digit
#   duration  - The duration, in milliseconds, of the digit
#
# Return 1 to hide the digit from further processing in SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
#proc dtmf_digit_received {digit duration} {
#  return [Logic::dtmf_digit_received $digit $duration];
#}


#
# Executed when a DTMF command has been received
#   cmd - The command
#
# Return 1 to hide the command from further processing is SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
#proc dtmf_cmd_received {cmd} {
#  return [Logic::dtmf_cmd_received $cmd];
#}


#
# Executed when the QSO recorder is being activated
#
#proc activating_qso_recorder {} {
#  Logic::activating_qso_recorder;
#}


#
# Executed when the QSO recorder is being deactivated
#
#proc deactivating_qso_recorder {} {
#  Logic::deactivating_qso_recorder;
#}


#
# Executed when trying to deactivate the QSO recorder even though it's
# not active
#
#proc qso_recorder_not_active {} {
#  Logic::qso_recorder_not_active;
#}


#
# Executed when trying to activate the QSO recorder even though it's
# already active
#
#proc qso_recorder_already_active {} {
#  Logic::qso_recorder_already_active;
#}


#
# Executed when the timeout kicks in to activate the QSO recorder
#
#proc qso_recorder_timeout_activate {} {
#  Logic::qso_recorder_timeout_activate
#}


#
# Executed when the timeout kicks in to deactivate the QSO recorder
#
#proc qso_recorder_timeout_deactivate {} {
#  Logic::qso_recorder_timeout_deactivate
#}


#
# Executed when the user is requesting a language change
#
#proc set_language {lang_code} {
#  Logic::set_language "$lang_code";
#}


#
# Executed when the user requests a list of available languages
#
#proc list_languages {} {
#  Logic::list_languages
#}


#
# Executed when the node is being brought online after being offline
#
#proc logic_online {online} {
#  Logic::logic_online $online
#}


#
# Executed when a configuration variable is updated at runtime in the logic
# core
#
#proc config_updated {tag value} {
#  Logic::config_updated "$tag" "$value"
#}


#
# Executed when a DTMF command is received from another linked logic core
#
#   logic -- The name of the logic core
#   cmd   -- The received command
#
#proc remote_cmd_received {logic cmd} {
#  Logic::remote_cmd_received "$logic" "$cmd"
#}


#
# Executed when a talkgroup is received from another linked logic core
#
#   logic -- The name of the logic core
#   tg    -- The received talkgroup
#
#proc remote_received_tg_updated {logic tg} {
#  Logic::remote_received_tg_updated "$logic" "$tg"
#}


#
# Executed when the pre broadcast notification is played
#
proc announce_prenotification {} {
  puts "Playing pre broadcast notification";
  playMsg "AnnounceLogic" "prenotify";
}


#
# Executed when the broadcast is played
#
proc announce_qst {} {
  puts "Playing broadcast";
  playMsg "AnnounceLogic" "Broadcast";
}

# end of namespace
}


#
# This file has not been truncated
#
