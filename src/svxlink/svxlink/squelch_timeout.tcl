###############################################################################
#
# This TCL module implement a squelch timeout warning system. If the timeout
# set by the SQL_TIMEOUT configuration variable is exceeded, a warning sound
# will be emitted once per second.
# When the squelch close, or alternatively when the roger sound is played, a
# message indicating that a timeout has occurred will be played back.
#
# This file should be sourced into a logic core TCL namespace to enable the
# functionality.
#
###############################################################################

# Namespace for the squelch timeout feature
namespace eval squelch_timeout {

# Search for procedures in the parent namespace as well as in the current one
namespace path [namespace parent]

# The number of seconds until the squelch timeout warning will sound.
# If SQL_TIMEOUT is not set the rest of this file will be ignored.
variable sql_timeout [getConfigValue ${::logic_name} "SQL_TIMEOUT" 0]
if {$sql_timeout <= 0} {
  return
}

# The configured roger sound delay
variable rgr_sound_delay [getConfigValue ${::logic_name} "RGR_SOUND_DELAY" -1]

# The number of seconds that have elapsed since squelch open
variable time_elapsed 0


#
# Called once per second, when the squelch is open, to check if the warning
# sound should be emitted
#
proc check {} {
  variable sql_timeout
  variable time_elapsed

  #puts "### Squelch timeout time elapsed: $time_elapsed"
  if {[incr time_elapsed] >= $sql_timeout} {
    timeout_warning
  }
}


#
# Handle end of transmission. Depending of configuration this will be called on
# squelch close or when the roger sound is played.
#
proc end_handler {} {
  variable sql_timeout
  variable time_elapsed
  if {$time_elapsed >= $sql_timeout} {
    timeout_end
  }
  set time_elapsed 0
}


#
# Called when a squelch timeout warning sound should be emitted
#
proc timeout_warning {} {
  playTone 850 700 100
}


#
# Called when the squelch timeout condition ends, i.e. when the squelch close
#
proc timeout_end {} {
  playMsg "timeout"
}


# End of namespace squelch_timeout
}


#
# Executed each time the squelch is opened or closed
#   rx_id   - The ID of the RX that the squelch opened/closed on
#   is_open - Set to 1 if the squelch is open or 0 if it's closed
#
override proc squelch_open {rx_id is_open} {
  variable squelch_timeout::rgr_sound_delay

  $SUPER $rx_id $is_open

  if {$is_open} {
    addSecondTickSubscriber squelch_timeout::check
  } else {
    removeSecondTickSubscriber squelch_timeout::check
    if {$rgr_sound_delay < 0} {
      squelch_timeout::end_handler
    }
  }
}


#
# Executed when the squelch have just closed and the RGR_SOUND_DELAY timer has
# expired.
#
# This overridden version will reset the squelch timeout time elapsed counter.
#
override proc send_rgr_sound {} {
  squelch_timeout::end_handler
  $SUPER
}


#
# This file has not been truncated
#
