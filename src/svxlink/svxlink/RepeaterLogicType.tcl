###############################################################################
#
# RepeaterLogic event handlers
#
# This file is sourced by all repeater logics and contains the TCL code that
# are in common to all repeater logics.
#
###############################################################################

# The namespace is automatically set to the logic core name so there is no need
# to change it unless you have a good reason
namespace eval ${::logic_name} {

# Mix in ("inherit") generic logic TCL code
sourceTclWithOverrides "Logic.tcl"
mixin Logic


# This variable indicates if the repeater is up or not
variable repeater_is_up 0;


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
  global mycall;
  global active_module;
  variable repeater_is_up;
  variable Logic::prev_ident
  variable Logic::min_time_between_ident

  set repeater_is_up 1;

  if {($reason != "SQL_OPEN") && ($reason != "CTCSS_OPEN") &&
      ($reason != "SQL_RPT_REOPEN")} {
    set now [clock seconds];
    if {$now-$prev_ident < $min_time_between_ident} {
      return;
    }
    set prev_ident $now;

    spellWord $mycall;
    playMsg "Core" "repeater";
    playSilence 250;

    if {$active_module != ""} {
      playMsg "Core" "active_module";
      playMsg $active_module "name";
    }
  }
}


#
# Executed when the repeater is deactivated
#   reason  - The reason why the repeater was deactivated
#             IDLE         - The idle timeout occured
#             SQL_FLAP_SUP - Closed due to interference
#
proc repeater_down {reason} {
  global mycall;
  variable repeater_is_up;
  variable Logic::prev_ident
  variable Logic::min_time_between_ident

  set repeater_is_up 0;

  if {$reason == "SQL_FLAP_SUP"} {
    playSilence 500;
    playMsg "Core" "interference";
    playSilence 500;
    return;
  }

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

  #playMsg "../extra-sounds" "shutdown";
}


#
# Executed when there has been no activity on the repeater for
# IDLE_SOUND_INTERVAL milliseconds. This function will be called each
# IDLE_SOUND_INTERVAL millisecond until there is activity or the repeater
# is deactivated.
#
proc repeater_idle {} {
  set iterations 8;
  set base 2;
  set max [expr {pow($base, $iterations)}];
  for {set i $iterations} {$i>0} {set i [expr $i - 1]} {
    playTone 1100 [expr {round(pow($base, $i) * 150 / $max)}] 100;
    playTone 1200 [expr {round(pow($base, $i) * 150 / $max)}] 100;
  }
}


#
# Executed if the repeater opens but the squelch never opens again.
# This is probably someone who opens the repeater but do not identify.
#
proc identify_nag {} {
  playSilence 500;
  playMsg "Core" "please_identify";
  playSilence 500;
}


# end of namespace
}

#
# This file has not been truncated
#
