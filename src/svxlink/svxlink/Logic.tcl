###############################################################################
#
# Generic Logic event handlers
#
###############################################################################

# Source locale handling code
sourceTclWithOverrides "locale.tcl"

# Wrap all variables and functions in a namespace
namespace eval Logic {

# Enable finding commands in the parent namespace from this namespace
namespace path [namespace parent]

# Add the logic_name variable to the Logic namespace to support legacy TCL code
variable logic_name ${::logic_name}

# A variable used to store a timestamp for the last identification.
variable prev_ident 0

# A constant that indicates the minimum time in seconds to wait between two
# identifications. Manual and long identifications is not affected.
variable min_time_between_ident 120

# Short and long identification intervals. They are setup from config
# variables below.
variable long_ident_interval \
  [getConfigValue ${::logic_name} LONG_IDENT_INTERVAL 0]
variable short_ident_interval \
  [getConfigValue ${::logic_name} SHORT_IDENT_INTERVAL $long_ident_interval]

variable short_voice_id_enable  \
  [getConfigValue ${::logic_name} SHORT_VOICE_ID_ENABLE 1]
variable short_cw_id_enable     \
  [getConfigValue ${::logic_name} SHORT_CW_ID_ENABLE 0]
variable short_announce_enable  \
  [getConfigValue ${::logic_name} SHORT_ANNOUNCE_ENABLE 0]
variable short_announce_file    \
  [getConfigValue ${::logic_name} SHORT_ANNOUNCE_FILE ""]

variable long_voice_id_enable   \
  [getConfigValue ${::logic_name} LONG_VOICE_ID_ENABLE 1]
variable long_cw_id_enable      \
  [getConfigValue ${::logic_name} LONG_CW_ID_ENABLE 0]
variable long_announce_enable   \
  [getConfigValue ${::logic_name} LONG_ANNOUNCE_ENABLE 0]
variable long_announce_file     \
  [getConfigValue ${::logic_name} LONG_ANNOUNCE_FILE ""]

# The ident_only_after_tx variable indicates if identification is only to
# occur after the node has transmitted. The variable is setup below from the
# configuration variable with the same name.
# The need_ident variable indicates if identification is needed.
variable ident_only_after_tx \
  [getConfigValue ${::logic_name} IDENT_ONLY_AFTER_TX 0]
variable need_ident 0

# List of functions that should be called periodically. Use the
# addMinuteTickSubscriber and addSecondTickSubscriber functions to
# add subscribers.
variable minute_tick_subscribers [list]
variable second_tick_subscribers [list]

# Contains the ID of the last receiver that indicated squelch activity
variable sql_rx_id "?"

# Load TCL code modules
sourceTclWithOverrides "CW.tcl"


#
# An "overloaded" playMsg that eliminates the need to write "Core" as the first
# argument everywhere.
# For legacy code support, if more than one argument is given to the function
# it will call the original playMsg using all given arguments
#
proc playMsg {args} {
  if {[llength $args] == 1} {
    ::playMsg Core $args
  } else {
    ::playMsg {*}$args
  }
}


#
# A convenience function for printing out information prefixed by the
# module name
#
proc printInfo {msg} {
  puts "${::logic_name}: ${msg}"
}


#
# Executed when the SvxLink software is started
#
proc startup {} {
  #playMsg "online"
  #send_short_ident
  addMinuteTickSubscriber checkPeriodicIdentify
}


#
# Executed when a specified module could not be found
#   module_id - The numeric ID of the module
#
proc no_such_module {module_id} {
  playMsg "no_such_module"
  playNumber $module_id
}


#
# Executed when a manual identification is initiated with the * DTMF code
#
proc manual_identification {} {
  variable prev_ident

  set epoch [clock seconds]
  set hour [clock format $epoch -format "%k"]
  regexp {([1-5]?\d)$} [clock format $epoch -format "%M"] -> minute
  set prev_ident $epoch

  playMsg "online"
  spellWord ${::mycall}
  if {${::logic_type} == "Repeater"} {
    playMsg "repeater"
  }
  playSilence 250
  playMsg "the_time_is"
  playTime $hour $minute
  playSilence 250
  if {${::report_ctcss} > 0} {
    playMsg "pl_is"
    playFrequency ${::report_ctcss}
    playSilence 300
  }
  if {${::active_module} != ""} {
    playMsg "active_module"
    playMsg ${::active_module} "name"
    playSilence 250
    ${::active_module}::status_report
  } else {
    foreach module [split ${::loaded_modules} " "] {
      ${module}::status_report
    }
  }
  if {[lsearch [split ${::loaded_modules} " "] "Help"] != -1} {
    playMsg "press_0_for_help"
  }
  playSilence 250
}


#
# Executed when a short voice identification should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_short_voice_ident {hour minute} {
  printInfo "Playing short voice ID"
  spellWord ${::mycall}
  if {${::logic_type} == "Repeater"} {
    playMsg "repeater"
  }
}


#
# Executed when a short file announcement identification should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_short_announce_ident {hour minute} {
  variable short_announce_file

  if [file exist "$short_announce_file"] {
    printInfo "Playing short announce"
    playFile "$short_announce_file"
  }
}


#
# Executed when a short cw identification should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_short_cw_ident {hour minute} {
  printInfo "Playing short CW ID"
  if {${::logic_type} == "Repeater"} {
    set call "${::mycall}/R"
    CW::play $call
  } else {
    CW::play ${::mycall}
  }
}


#
# Executed when a short identification should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_short_ident {{hour -1} {minute -1}} {
  variable short_voice_id_enable
  variable short_announce_enable
  variable short_cw_id_enable

  printInfo "Sending short identification..."

  # Play voice id if enabled
  if {$short_voice_id_enable} {
    send_short_voice_ident $hour $minute
    playSilence 500
  }

  # Play announcement file if enabled
  if {$short_announce_enable} {
    send_short_announce_ident $hour $minute
    playSilence 500
  }

  # Play CW id if enabled
  if {$short_cw_id_enable} {
    send_short_cw_ident $hour $minute
    playSilence 500
  }
}


#
# Executed when a long voice identification (e.g. hourly) should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_long_voice_ident {hour minute} {
  printInfo "Playing Long voice ID"
  spellWord ${::mycall}
  if {${::logic_type} == "Repeater"} {
    playMsg "repeater"
  }
  playSilence 500
  playMsg "the_time_is"
  playSilence 100
  playTime $hour $minute
  playSilence 500

  # Call the "status_report" function in all modules if no module is active
  if {${::active_module} == ""} {
    foreach module [split ${::loaded_modules} " "] {
      ${module}::status_report
    }
  }
}


#
# Executed when a long file announcement identification (e.g. hourly) should be
# sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_long_announce_ident {hour minute} {
  variable long_announce_file

  printInfo "Playing long announce"
  if [file exist "$long_announce_file"] {
    playFile "$long_announce_file"
  }
}


#
# Executed when a long CW identification (e.g. hourly) should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_long_cw_ident {hour minute} {
  printInfo "Playing long CW ID"
  if {${::logic_type} == "Repeater"} {
    CW::play "${::mycall}/R"
  } else {
    CW::play ${::mycall}
  }
}


#
# Executed when a long identification (e.g. hourly) should be sent
#   hour    - The hour on which this identification occur
#   minute  - The minute on which this identification occur
#
proc send_long_ident {hour minute} {
  variable long_voice_id_enable
  variable long_announce_enable
  variable long_cw_id_enable

  printInfo "Sending long identification..."

  # Play the voice ID if enabled
  if {$long_voice_id_enable} {
    send_long_voice_ident $hour $minute
    playSilence 500
  }

  # Play announcement file if enabled
  if {$long_announce_enable} {
    send_long_announce_ident $hour $minute
    playSilence 500
  }

  # Play CW id if enabled
  if {$long_cw_id_enable} {
    send_long_cw_ident $hour $minute
    playSilence 500
  }
}


#
# Executed when the squelch have just closed and the RGR_SOUND_DELAY timer has
# expired.
#
proc send_rgr_sound {} {
  variable sql_rx_id

  if {$sql_rx_id != "?"} {
    # 200 CPM, 1000 Hz, -10 dBFS
    CW::play $sql_rx_id 200 1000 -10
    set sql_rx_id "?"
  } else {
    playTone 440 500 100
  }
  playSilence 100
}


#
# Executed when an empty macro command (i.e. D#) has been entered.
#
proc macro_empty {} {
  playMsg "operation_failed"
}


#
# Executed when an entered macro command could not be found
#
proc macro_not_found {} {
  playMsg "operation_failed"
}


#
# Executed when a macro syntax error occurs (configuration error).
#
proc macro_syntax_error {} {
  playMsg "operation_failed"
}


#
# Executed when the specified module in a macro command is not found
# (configuration error).
#
proc macro_module_not_found {} {
  playMsg "operation_failed"
}


#
# Executed when the activation of the module specified in the macro command
# failed.
#
proc macro_module_activation_failed {} {
  playMsg "operation_failed"
}


#
# Executed when a macro command is executed that requires a module to
# be activated but another module is already active.
#
proc macro_another_active_module {} {
  playMsg "operation_failed"
  playMsg "active_module"
  playMsg ${::active_module} "name"
}


#
# Executed when an unknown DTMF command is entered
#   cmd - The command string
#
proc unknown_command {cmd} {
  spellWord $cmd
  playMsg "unknown_command"
}


#
# Executed when an entered DTMF command failed
#   cmd - The command string
#
proc command_failed {cmd} {
  spellWord $cmd
  playMsg "operation_failed"
}


#
# Executed when a link to another logic core is activated.
#   name  - The name of the link
#
proc activating_link {name} {
  if {[string length $name] > 0} {
    playMsg "activating_link_to"
    spellWord $name
  }
}


#
# Executed when a link to another logic core is deactivated.
#   name  - The name of the link
#
proc deactivating_link {name} {
  if {[string length $name] > 0} {
    playMsg "deactivating_link_to"
    spellWord $name
  }
}


#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#   name  - The name of the link
#
proc link_not_active {name} {
  if {[string length $name] > 0} {
    playMsg "link_not_active_to"
    spellWord $name
  }
}


#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#   name  - The name of the link
#
proc link_already_active {name} {
  if {[string length $name] > 0} {
    playMsg "link_already_active_to"
    spellWord $name
  }
}


#
# Executed each time the transmitter is turned on or off
#   is_on - Set to 1 if the transmitter is on or 0 if it's off
#
proc transmit {is_on} {
  #printInfo "Turning the transmitter $is_on"
  variable prev_ident
  variable need_ident
  if {$is_on && ([clock seconds] - $prev_ident > 5)} {
    set need_ident 1
  }
}


#
# Executed each time the squelch is opened or closed
#   rx_id   - The ID of the RX that the squelch opened/closed on
#   is_open - Set to 1 if the squelch is open or 0 if it's closed
#
proc squelch_open {rx_id is_open} {
  variable sql_rx_id
  #printInfo "The squelch is $is_open on RX $rx_id"
  set sql_rx_id $rx_id
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
  #printInfo "DTMF digit \"$digit\" detected with duration $duration ms"
  return 0
}


#
# Executed when a DTMF command has been received
#   cmd - The command
#
# Return 1 to hide the command from further processing is SvxLink or
# return 0 to make SvxLink continue processing as normal.
#
# This function can be used to implement your own custom commands or to disable
# DTMF commands that you do not want users to execute.
proc dtmf_cmd_received {cmd} {
  # Example: Ignore all commands starting with 3 in the EchoLink module.
  #          Allow commands that have four or more digits.
  #if {${::active_module} == "EchoLink"} {
  #  if {[string length $cmd] < 4 && [string index $cmd 0] == "3"} {
  #    printInfo "Ignoring random connect command for module EchoLink: $cmd"
  #    return 1
  #  }
  #}

  # Handle the "force core command" mode where a command is forced to be
  # executed by the core command processor instead of by an active module.
  # The "force core command" mode is entered by prefixing a command by a star.
  #if {${::active_module} != "" && [string index $cmd 0] != "*"} {
  #  return 0
  #}
  #if {[string index $cmd 0] == "*"} {
  #  set cmd [string range $cmd 1 end]
  #}

  # Example: Custom command executed when DTMF 99 is received
  #if {$cmd == "99"} {
  #  printInfo "Executing external command"
  #  playMsg "online"
  #  exec ls &
  #  return 1
  #}

  return 0
}


#
# Executed once every whole minute. Don't put any code here directly
# Create a new function and add it to the timer tick subscriber list
# by using the function addMinuteTickSubscriber.
#
proc every_minute {} {
  variable minute_tick_subscribers
  #printInfo [clock format [clock seconds] -format "%Y-%m-%d %H:%M:%S"]
  foreach subscriber $minute_tick_subscribers {
    set func [dict get $subscriber func]
    set ns [dict get $subscriber ns]
    namespace eval $ns $func
  }
}


#
# Executed once every whole minute. Don't put any code here directly
# Create a new function and add it to the timer tick subscriber list
# by using the function addSecondTickSubscriber.
#
proc every_second {} {
  variable second_tick_subscribers
  #printInfo [clock format [clock seconds] -format "%Y-%m-%d %H:%M:%S"]
  foreach subscriber $second_tick_subscribers {
    set func [dict get $subscriber func]
    set ns [dict get $subscriber ns]
    namespace eval $ns $func
  }
}


#
# Deprecated: Use the addMinuteTickSubscriber function instead
#
proc addTimerTickSubscriber {func} {
  puts "*** WARNING: Calling deprecated TCL event handler addTimerTickSubcriber."
  puts "             Use addMinuteTickSubscriber instead"
  addMinuteTickSubscriber $func
}


#
# Use this function to add a function to the list of functions that
# should be executed once every whole minute. This is not an event
# function but rather a management function.
#
proc addMinuteTickSubscriber {func} {
  variable minute_tick_subscribers
  set ns [uplevel namespace current]
  lappend minute_tick_subscribers [dict create func $func ns $ns]
}


#
# Use this function to add a function to the list of functions that
# should be executed once every second. This is not an event
# function but rather a management function.
#
proc addSecondTickSubscriber {func} {
  variable second_tick_subscribers
  set ns [uplevel namespace current]
  lappend second_tick_subscribers [dict create func $func ns $ns]
}


#
# Should be executed once every whole minute to check if it is time to
# identify. Not exactly an event function. This function handle the
# identification logic and call the send_short_ident or send_long_ident
# functions when it is time to identify.
#
proc checkPeriodicIdentify {} {
  variable prev_ident
  variable short_ident_interval
  variable long_ident_interval
  variable min_time_between_ident
  variable ident_only_after_tx
  variable need_ident

  set now [clock seconds]
  set hour [clock format $now -format "%k"]
  regexp {([1-5]?\d)$} [clock format $now -format "%M"] -> minute

  set short_ident_now \
    [expr {($short_ident_interval != 0) && \
           (($hour * 60 + $minute) % $short_ident_interval == 0)}]
  set long_ident_now \
    [expr {($long_ident_interval != 0) && \
           (($hour * 60 + $minute) % $long_ident_interval == 0)}]

  if {$long_ident_now} {
    send_long_ident $hour $minute
    set prev_ident $now
    set need_ident 0
  } elseif {$short_ident_now} {
    if {$now - $prev_ident < $min_time_between_ident} {
      return
    }
    if {$ident_only_after_tx && !$need_ident} {
      return
    }

    send_short_ident $hour $minute
    set prev_ident $now
    set need_ident 0
  }
}


#
# Executed when the QSO recorder is being activated
#
proc activating_qso_recorder {} {
  playMsg "activating"
  playMsg "qso_recorder"
}


#
# Executed when the QSO recorder is being deactivated
#
proc deactivating_qso_recorder {} {
  playMsg "deactivating"
  playMsg "qso_recorder"
}


#
# Executed when trying to deactivate the QSO recorder even though it's
# not active
#
proc qso_recorder_not_active {} {
  playMsg "qso_recorder"
  playMsg "not_active"
}


#
# Executed when trying to activate the QSO recorder even though it's
# already active
#
proc qso_recorder_already_active {} {
  playMsg "qso_recorder"
  playMsg "already_active"
}


#
# Executed when the timeout kicks in to activate the QSO recorder
#
proc qso_recorder_timeout_activate {} {
  playMsg "timeout"
  playMsg "activating"
  playMsg "qso_recorder"
}


#
# Executed when the timeout kicks in to deactivate the QSO recorder
#
proc qso_recorder_timeout_deactivate {} {
  playMsg "timeout"
  playMsg "deactivating"
  playMsg "qso_recorder"
}


#
# Executed when the user is requesting a language change
#
proc set_language {lang_code} {
  printInfo "Setting language $lang_code (NOT IMPLEMENTED)"

}


#
# Executed when the user requests a list of available languages
#
proc list_languages {} {
  printInfo "Available languages: (NOT IMPLEMENTED)"
}


#
# Executed when the node is being brought online or offline
#
proc logic_online {online} {
  if {$online} {
    playMsg "online"
    spellWord ${::mycall}
    if {${::logic_type} == "Repeater"} {
      playMsg "repeater"
    }
  }
}


#
# Executed when a configuration variable is updated at runtime in the logic
# core
#
proc config_updated {tag value} {
  #printInfo "Configuration variable updated: $tag=$value"
}


#
# Executed when a DTMF command is received from another linked logic core
#
#   logic -- The name of the logic core
#   cmd   -- The received command
#
proc remote_cmd_received {logic cmd} {
  #printInfo "Remote command received from logic $logic: $cmd"
  #playDtmf "$cmd" "500" "50"
}


#
# Executed when a talkgroup is received from another linked logic core
#
#   logic -- The name of the logic core
#   tg    -- The received talkgroup
#
proc remote_received_tg_updated {logic tg} {
  #printInfo "Remote TG received from logic $logic: $tg"
  #if {$tg > 0} {
  #  playDtmf "1$tg" "500" "50"
  #}
}


# End of namespace Logic
}


proc sourceModuleTclHandlers {} {
  foreach module ${::loaded_modules} {
    sourceTclWithOverrides "${module}.tcl"
    set module_path "${::basedir}/modules.d/Module${module}.tcl"
    if [file exists "$module_path"] {
      sourceTcl "$module_path"
    }
  }
}
sourceModuleTclHandlers


#
# This file has not been truncated
#
