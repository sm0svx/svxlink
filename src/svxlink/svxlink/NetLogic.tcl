###############################################################################
#
# NetLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[RepeaterLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval NetLogic {

#
# Checking to see if this is the current logic core type
#
if {$logic_name != [namespace tail [namespace current]]} {
  return;
}

#
# This variable indicates if the repeater is up or not
#
variable repeater_is_up 0;

#
# Executed when the SvxLink software is started
#
proc startup {} {
  global logic_name;
  append func $logic_name "::checkPeriodicIdentify";
  Logic::addTimerTickSubscriber $func;
  Logic::startup;
}

#
# Executed when a specified module could not be found
#
proc no_such_module {module_id} {
}

#
# Executed when a manual identification is initiated with the * DTMF code
#
proc manual_identification {} {
}

#
# Executed when the squelch just have closed and the RGR_SOUND_DELAY timer has
# expired.
#
proc send_rgr_sound {} {
}

#
# Executed when an empty macro command (i.e. D#) has been entered.
#
proc macro_empty {} {
}

#
# Executed when an entered macro command could not be found
#
proc macro_not_found {} {
}

#
# Executed when a macro syntax error occurs (configuration error).
#
proc macro_syntax_error {} {
}

#
# Executed when the specified module in a macro command is not found
# (configuration error).
#
proc macro_module_not_found {} {
}

#
# Executed when the activation of the module specified in the macro command
# failed.
#
proc macro_module_activation_failed {} {
}

#
# Executed when a macro command is executed that requires a module to
# be activated but another module is already active.
#
proc macro_another_active_module {} {
}

#
# Executed when an unknown DTMF command is entered
#
proc unknown_command {cmd} {
}

#
# Executed when an entered DTMF command failed
#
proc command_failed {cmd} {
}

#
# Executed when a link to another logic core is activated.
#   name  - The name of the link
#
proc activating_link {name} {
}

#
# Executed when a link to another logic core is deactivated.
#   name  - The name of the link
#
proc deactivating_link {name} {
}

#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#   name  - The name of the link
#
proc link_not_active {name} {
}

#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#   name  - The name of the link
#
proc link_already_active {name} {
}

#
# Executed each time the transmitter is turned on or off
#
proc transmit {is_on} {
}

#
# Executed each time the squelch is opened or closed
#
proc squelch_open {rx_id is_open} {
}

#
# Executed once every whole minute
#
proc every_minute {} {
  Logic::every_minute;
}

#
# Executed when the repeater is activated
#   reason  - The reason why the repeater was activated
#	      SQL_CLOSE	      - Open on squelch, close flank
#	      SQL_OPEN	      - Open on squelch, open flank
#	      CTCSS_CLOSE     - Open on CTCSS, squelch close flank
#	      CTCSS_OPEN      - Open on CTCSS, squelch open flank
#	      TONE	      - Open on tone burst (always on squelch close)
#	      DTMF	      - Open on DTMF digit (always on squelch close)
#	      MODULE	      - Open on module activation
#	      AUDIO	      - Open on incoming audio (module or logic linking)
#	      SQL_RPT_REOPEN  - Reopen on squelch after repeater down
#
proc repeater_up {reason} {
}

#
# Executed when the repeater is deactivated
#   reason  - The reason why the repeater was deactivated
#             IDLE         - The idle timeout occured
#             SQL_FLAP_SUP - Closed due to interference
#
proc repeater_down {reason} {
}

#
# Executed when there has been no activity on the repeater for
# IDLE_SOUND_INTERVAL milliseconds. This function will be called each
# IDLE_SOUND_INTERVAL millisecond until there is activity or the repeater
# is deactivated.
#
proc repeater_idle {} {
}

#
# Executed once every whole minute to check if it's time to identify
#
proc checkPeriodicIdentify {} {
}

#
# Executed when a DTMF digit has been received
#   digit     - The detected DTMF digit
#   duration  - The duration, in milliseconds, of the digit
#
# Return 1 to hide the digit from further processing in SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
proc dtmf_digit_received {digit duration} {
}

#
# Executed when a DTMF command has been received
#   cmd - The command
#
# Return 1 to hide the command from further processing is SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
proc dtmf_cmd_received {cmd} {
}

#
# Executed when the QSO recorder is being activated
#
proc activating_qso_recorder {} {
}

#
# Executed when the QSO recorder is being deactivated
#
proc deactivating_qso_recorder {} {
}

#
# Executed when trying to deactivate the QSO recorder even though it's
# not active
#
proc qso_recorder_not_active {} {
}

#
# Executed when trying to activate the QSO recorder even though it's
# already active
#
proc qso_recorder_already_active {} {
}

#
# Executed when the timeout kicks in to activate the QSO recorder
#
proc qso_recorder_timeout_activate {} {
}

#
# Executed when the timeout kicks in to deactivate the QSO recorder
#
proc qso_recorder_timeout_deactivate {} {
}

#
# Executed if the repeater opens but the squelch never opens again.
# This is probably someone who opens the repeater but do not identify.
#
proc identify_nag {} {
}

#
# Executed when the user is requesting a language change
#
proc set_language {lang_code} {
}

#
# Executed when the user requests a list of available languages
#
proc list_languages {} {
}

#
# Executed when the node is being brought online or offline
#
proc logic_online {online} {
}

# end of namespace
}

#
# This file has not been truncated
#
