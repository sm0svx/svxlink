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

#
# Checking to see if this is the correct logic core
#
if {$logic_name != [namespace tail [namespace current]]} {
  return;
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
# Executed when manual TG announcement is triggered
#
proc report_tg_status {} {
  variable selected_tg
  variable previous_tg
  variable prev_announce_time
  variable prev_announce_tg
  playSilence 100
  if {$selected_tg > 0} {
    set prev_announce_time [clock seconds]
    set prev_announce_tg $selected_tg
    playMsg "Core" "talk_group"
    spellNumber $selected_tg
  } else {
    playMsg "Core" "previous"
    playMsg "Core" "talk_group"
    spellNumber $previous_tg
  }
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

  #puts "### tg_local_activation"
  if {$new_tg != $old_tg} {
    set prev_announce_time [clock seconds]
    set prev_announce_tg $new_tg
    playSilence 100
    playMsg "Core" "talk_group"
    spellNumber $new_tg
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
    spellNumber $new_tg
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

  #puts "### tg_command_activation"
  set prev_announce_time [clock seconds]
  set prev_announce_tg $new_tg
  playSilence 100
  playMsg "Core" "talk_group"
  spellNumber $new_tg
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
  #puts "### tg_default_activation"
  #if {$new_tg != $old_tg} {
  #  set prev_announce_time [clock seconds]
  #  set prev_announce_tg $new_tg
  #  playSilence 100
  #  playMsg "Core" "talk_group"
  #  spellNumber $new_tg
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
  spellNumber $new_tg
}


#
# Executed when a TG QSY request fails
#
# A TG QSY may fail for primarily two reasons, either no talk group is
# currently active or there is no connection to the reflector server.
#
proc tg_qsy_failed {} {
  #puts "### tg_qsy_idle"
  playSilence 100
  playMsg "Core" "qsy"
  playSilence 200
  playMsg "Core" "operation_failed"
}


#
# Executed when a TG QSY request is ignored
#
# tg -- The talk group requested in the QSY
#
proc tg_qsy_ignored {tg} {
  #puts "### tg_qsy_ignored"
  playSilence 100
  playMsg "Core" "qsy"
  spellNumber $tg
  playMsg "Core" "ignored"
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
  spellNumber $tg
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


# end of namespace
}


#
# This file has not been truncated
#
