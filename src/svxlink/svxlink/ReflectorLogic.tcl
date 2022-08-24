###############################################################################
#
# ReflectorLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[ReflectorLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval ReflectorLogic {

# The currently selected TG. Variable set from application.
variable selected_tg 0

# The previously selected TG. Variable set from application.
variable previous_tg 0

# Timestamp for previous TG announcement
variable prev_announce_time 0

# The previously announced TG
variable prev_announce_tg 0

# The minimum time between announcements of the same TG.
# Change through ANNOUNCE_REMOTE_MIN_INTERVAL config variable.
variable announce_remote_min_interval 0

# This variable will be set to 1 if the QSY pending feature ("QSY on squelch
# activity") is active. See configuration variable QSY_PENDING_TIMEOUT.
variable qsy_pending_active 0

# This variable will be set to 1 if the connection to the reflector is
# established and to 0 if disconnected.
variable reflector_connection_established 0

#
# Checking to see if this is the correct logic core
#
if {$logic_name != [namespace tail [namespace current]]} {
  return;
}


#
# A helper function for announcing a talkgroup.
# If there is an audio clip matching the name talk_group-<tg> it will be played
# instead of spelling the digits. Look at the documentation for playMsg for
# more information on where to put the audio clip.
#
#   tg - The talkgroup to announce
#
proc say_talkgroup {tg} {
  if [playMsg "Core" "talk_group-$tg" 0] {
  } else {
    spellNumber $tg
  }
}


#
# Executed when an unknown command is received
#   cmd - The command string
#
proc unknown_command {cmd} {
  Logic::unknown_command $cmd;
}


#
# Executed when a received command fails
#
proc command_failed {cmd} {
  Logic::command_failed $cmd;
}


#
# Executed when the reflector connection status is updated
#
#   is_established - 0=disconnected, 1=established
#
proc reflector_connection_status_update {is_established} {
  variable reflector_connection_established
  if {$is_established != $reflector_connection_established} {
    set reflector_connection_established $is_established
    #playMsg "Core" "reflector"
    #if {$is_established} {
    #  playMsg "Core" "connected"
    #} else {
    #  playMsg "Core" "disconnected"
    #}
  }
}


#
# Executed when manual TG announcement is triggered
#
proc report_tg_status {} {
  variable selected_tg
  variable previous_tg
  variable prev_announce_time
  variable prev_announce_tg
  variable reflector_connection_established
  playSilence 100
  playMsg "Core" "reflector"
  if {$reflector_connection_established} {
    playMsg "Core" "connected"
  } else {
    playMsg "Core" "disconnected"
  }
  playSilence 200
  if {$selected_tg > 0} {
    set prev_announce_time [clock seconds]
    set prev_announce_tg $selected_tg
    playMsg "Core" "talk_group"
    say_talkgroup $selected_tg
  } else {
    playMsg "Core" "previous"
    playMsg "Core" "talk_group"
    say_talkgroup $previous_tg
  }
}


#
# Executed when a TG has been selected
# This function is called immediately when a change in talkgroup selection
# occurs. In constrast, the other more specific talkgroup selection event
# functions below is called with a delay in order to make announcement ordering
# more logical.
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_selected {new_tg old_tg} {
  #puts "### tg_selected #$new_tg (old #$old_tg)"
  # Reject incoming Echolink connections while a talkgroup is active
  #if {$new_tg != 0} {
  #  setConfigValue "ModuleEchoLink" "REJECT_INCOMING" "^.*$"
  #} else {
  #  setConfigValue "ModuleEchoLink" "REJECT_INCOMING" "^$"
  #}
}


#
# Executed when a TG has been selected due to local activity
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_local_activation {new_tg old_tg} {
  variable prev_announce_time
  variable prev_announce_tg
  variable selected_tg
  variable reflector_connection_established

  #puts "### tg_local_activation"
  if {$new_tg != $old_tg} {
    set prev_announce_time [clock seconds]
    set prev_announce_tg $new_tg
    playSilence 100
    if {!$reflector_connection_established} {
      playMsg "Core" "reflector"
      playMsg "Core" "disconnected"
      playSilence 200
    }
    playMsg "Core" "talk_group"
    say_talkgroup $new_tg
  }
}


#
# Executed when a TG has been selected due to remote activity
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_remote_activation {new_tg old_tg} {
  variable prev_announce_time
  variable prev_announce_tg
  variable announce_remote_min_interval

  #puts "### tg_remote_activation"
  set now [clock seconds];
  if {($new_tg == $prev_announce_tg) && \
      ($now - $prev_announce_time < $announce_remote_min_interval)} {
    return;
  }
  if {$new_tg != $old_tg} {
    set prev_announce_time $now
    set prev_announce_tg $new_tg
    playSilence 100
    playMsg "Core" "talk_group"
    say_talkgroup $new_tg
  }
}


#
# Executed when a TG has been selected due to remote activity on a prioritized
# monitored talk group while a lower prio talk group is selected
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_remote_prio_activation {new_tg old_tg} {
  tg_remote_activation $new_tg $old_tg
}


#
# Executed when a TG has been selected by DTMF command
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_command_activation {new_tg old_tg} {
  variable prev_announce_time
  variable prev_announce_tg
  variable reflector_connection_established

  #puts "### tg_command_activation"
  set prev_announce_time [clock seconds]
  set prev_announce_tg $new_tg
  playSilence 100
  if {!$reflector_connection_established} {
    playMsg "Core" "reflector"
    playMsg "Core" "disconnected"
    playSilence 200
  }
  playMsg "Core" "talk_group"
  say_talkgroup $new_tg
}


#
# Executed when a TG has been selected due to DEFAULT_TG configuration
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_default_activation {new_tg old_tg} {
  #variable prev_announce_time
  #variable prev_announce_tg
  #variable selected_tg
  #variable reflector_connection_established
  #puts "### tg_default_activation"
  #if {$new_tg != $old_tg} {
  #  set prev_announce_time [clock seconds]
  #  set prev_announce_tg $new_tg
  #  playSilence 100
  #  if {!$reflector_connection_established} {
  #    playMsg "Core" "reflector"
  #    playMsg "Core" "disconnected"
  #    playSilence 200
  #  }
  #  playMsg "Core" "talk_group"
  #  say_talkgroup $new_tg
  #}
}


#
# Executed when a TG QSY request have been acted upon
#
#   new_tg -- The talk group that has been activated
#   old_tg -- The talk group that was active
#
proc tg_qsy {new_tg old_tg} {
  variable prev_announce_time
  variable prev_announce_tg

  #puts "### tg_qsy"
  set prev_announce_time [clock seconds]
  set prev_announce_tg $new_tg
  playSilence 100
  playMsg "Core" "qsy"
  #playMsg "Core" "talk_group"
  say_talkgroup $new_tg
}


#
# Executed when a QSY is followed due to squelch open (see QSY_PENDING_TIMEOUT)
#
#   tg -- The talk group that has been activated
#
proc tg_qsy_on_sql {tg} {
  playSilence 100
  playMsg "Core" "qsy"
}


#
# Executed when a TG QSY request fails
#
# A TG QSY may fail for primarily two reasons, either no talk group is
# currently active or there is no connection to the reflector server.
#
proc tg_qsy_failed {} {
  #puts "### tg_qsy_failed"
  playSilence 100
  playMsg "Core" "qsy"
  playSilence 200
  playMsg "Core" "operation_failed"
}


#
# Executed when a TG QSY request is pending
#
# tg -- The talk group requested in the QSY
#
proc tg_qsy_pending {tg} {
  playSilence 100
  playMsg "Core" "qsy"
  say_talkgroup $tg
  playMsg "Core" "pending"
}


#
# Executed when a TG QSY request is ignored
#
# tg -- The talk group requested in the QSY
#
proc tg_qsy_ignored {tg} {
  variable qsy_pending_active
  playSilence 100
  if {!$qsy_pending_active} {
    playMsg "Core" "qsy"
    say_talkgroup $tg
  }
  playMsg "Core" "ignored"
  playSilence 500
  playTone 880 200 50
  playTone 659 200 50
  playTone 440 200 50
  playSilence 100
}


#
# Executed when a TG selection has timed out
#
#   new_tg -- Always 0
#   old_tg -- The talk group that was active
#
proc tg_selection_timeout {new_tg old_tg} {
  #puts "### tg_selection_timeout"
  if {$old_tg != 0} {
    playSilence 100
    playTone 880 200 50
    playTone 659 200 50
    playTone 440 200 50
    playSilence 100
  }
}


#
# Executed on talker start
#
#   tg        -- The talk group
#   callsign  -- The callsign of the talker node
#
proc talker_start {tg callsign} {
  #puts "### Talker start on TG #$tg: $callsign"
}


#
# Executed on talker stop
#
#   tg        -- The talk group
#   callsign  -- The callsign of the talker node
#
proc talker_stop {tg callsign} {
  #variable selected_tg
  #variable ::Logic::CFG_CALLSIGN
  #puts "### Talker stop on TG #$tg: $callsign"
  #if {($tg == $selected_tg) && ($callsign != $::Logic::CFG_CALLSIGN)} {
  #  playSilence 100
  #  playTone 440 200 50
  #  playTone 659 200 50
  #  playTone 880 200 50
  #}
}


#
# A talk group was added for temporary monitoring
#
#   tg -- The added talk group
#
proc tmp_monitor_add {tg} {
  #puts "### tmp_monitor_add: $tg"
  playSilence 100
  playMsg "Core" "monitor"
  say_talkgroup $tg
}


#
# A talk group was removed from temporary monitoring
#
#   tg -- The removed talk group
#
proc tmp_monitor_remove {tg} {
  #puts "### tmp_monitor_remove: $tg"
}


if [info exists ::Logic::CFG_ANNOUNCE_REMOTE_MIN_INTERVAL] {
  set announce_remote_min_interval $::Logic::CFG_ANNOUNCE_REMOTE_MIN_INTERVAL
}

if [info exists ::Logic::CFG_QSY_PENDING_TIMEOUT] {
  if {$::Logic::CFG_QSY_PENDING_TIMEOUT > 0} {
    set qsy_pending_active 1
  }
}


# end of namespace
}


#
# This file has not been truncated
#
