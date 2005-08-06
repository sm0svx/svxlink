###############################################################################
#
# RepeaterLogic event handlers
#
###############################################################################

namespace eval RepeaterLogic {

#
# Executed when the SvxLink software is started
# 
proc startup {} {
  Logic::startup;
  #playMsg "Core" "repeater";
}


#
# Executed when a specified module could not be found
#
proc no_such_module {module_id} {
  Logic::no_such_module $module_id;
}


#
# Executed when a manual identification is initiated with the * DTMF code
#
proc manual_identification {} {
  global mycall;
  global report_ctcss;
  global active_module;
  
  set Logic::prev_ident [clock seconds];
  
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
    append func $active_module "::status_report";
    if {"[info procs $func]" ne ""} {
      $func;
    }
    playSilence 250;
  }
}


#
# Executed when the squelch just have closed and the RGR_SOUND_DELAY timer has
# expired.
#
proc send_rgr_sound {} {
  Logic::send_rgr_sound;
}


#
# Executed when an empty macro command (i.e. D#) has been entered.
#
proc macro_empty {} {
  Logic::macro_empty;
}


#
# Executed when an entered macro command could not be found
#
proc macro_not_found {} {
  Logic::macro_not_found;
}


#
# Executed when a macro syntax error occurs (configuration error).
#
proc macro_syntax_error {} {
  Logic::macro_syntax_error;
}


#
# Executed when the specified module in a macro command is not found
# (configuration error).
#
proc macro_module_not_found {} {
  Logic::macro_module_not_found;
}


#
# Executed when the activation of the module specified in the macro command
# failed.
#
proc macro_module_activation_failed {} {
  Logic::macro_module_activation_failed;
}


#
# Executed when a macro command is executed that requires a module to
# be activated but another module is already active.
#
proc macro_another_active_module {} {
  Logic::macro_another_active_module;
}


#
# Executed when the IDENT_INTERVAL timer has expired.
#
proc periodic_identify {} {
  global mycall;
  
  set now [clock seconds];
  if {$now-$Logic::prev_ident < $Logic::min_time_between_ident} {
    return;
  }
  set Logic::prev_ident $now;
  
  spellWord $mycall;
  playMsg "Core" "repeater";
}


#
# Executed when an unknown DTMF command is entered
#
proc unknown_command {cmd} {
  Logic::unknown_command $cmd;
}


#
# Executed when an entered DTMF command failed
#
proc command_failed {cmd} {
  Logic::command_failed $cmd;
}


#
# Executed when a link to another logic core is activated.
#
proc activating_link {name} {
  Logic::activating_link $name;
}


#
# Executed when a link to another logic core is deactivated
#
proc deactivating_link {name} {
  Logic::deactivating_link $name;
}


#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#
proc link_not_active {name} {
  Logic::link_not_active $name;
}


#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#
proc link_already_active {name} {
  Logic::link_already_active $name;
}


#
# Executed once every whole minute
#
proc every_minute {} {
  Logic::every_minute;
}


#
# Executed when the repeater is activated
#
proc repeater_up {} {
  global mycall;
  global active_module;
  
  #playMsg "../extra-sounds" "attention";
  #playSilence 250;

  set now [clock seconds];
  if {$now-$Logic::prev_ident < $Logic::min_time_between_ident} {
    return;
  }
  set Logic::prev_ident $now;
  
  spellWord $mycall;
  playMsg "Core" "repeater";
  playSilence 250;

  if {$active_module != ""} {
    playMsg "Core" "active_module";
    playMsg $active_module "name";
  }
}


#
# Executed when the repeater is deactivated
#
proc repeater_down {} {
  global mycall;
  
  set now [clock seconds];
  if {$now-$Logic::prev_ident < $Logic::min_time_between_ident} {
    playTone 400 900 50
    playSilence 100
    playTone 360 900 50
    playSilence 500
    return;
  }
  set Logic::prev_ident $now;
    
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
    playTone 1100 [expr {round(pow($base, $i) * 800 / $max)}] 100;
    playTone 1200 [expr {round(pow($base, $i) * 800 / $max)}] 100;
  }
}



# end of namespace
}

#
# This file has not been truncated
#
