###############################################################################
#
# Generic Logic event handlers
#
###############################################################################

namespace eval Logic {

#
# A variable used to store a timestamp for the last identification.
#
variable prev_ident 0;

#
# A constant that indicates the minimum time to wait between two
# identifications. Manual identification is not affected.
#
variable min_time_between_ident 120;


#
# Executed when the SvxLink software is started
# 
proc startup {} {
  #global mycall;
  #playMsg "Core" "online";
  #spellWord $mycall;
}


#
# Executed when a specified module could not be found
#
proc no_such_module {module_id} {
  playMsg "Core" "no_such_module";
  playNumber $module_id;
}


#
# Executed when a manual identification is initiated with the * DTMF code
#
proc manual_identification {} {
  global mycall;
  global report_ctcss;
  global active_module;
  global prev_ident;
  
  set prev_ident [clock seconds];
  
  playMsg "Core" "online";
  spellWord $mycall;
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
    playSilence 250;
    append func $active_module "_status_report";
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
  playTone 440 700 100;
}


#
# Executed when an empty macro command (i.e. D#) has been entered.
#
proc macro_empty {} {
  playMsg "Core" "operation_failed";
}


#
# Executed when an entered macro command could not be found
#
proc macro_not_found {} {
  playMsg "Core" "operation_failed";
}


#
# Executed when a macro syntax error occurs (configuration error).
#
proc macro_syntax_error {} {
  playMsg "Core" "operation_failed";
}


#
# Executed when the specified module in a macro command is not found
# (configuration error).
#
proc macro_module_not_found {} {
  playMsg "Core" "operation_failed";
}


#
# Executed when the activation of the module specified in the macro command
# failed.
#
proc macro_module_activation_failed {} {
  playMsg "Core" "operation_failed";
}


#
# Executed when a macro command is executed that requires a module to
# be activated but another module is already active.
#
proc macro_another_active_module {} {
  global active_module;
  
  playMsg "Core" "operation_failed";
  playMsg "Core" "active_module";
  playMsg $active_module "name";
}


#
# Executed when the IDENT_INTERVAL timer has expired.
#
proc periodic_identify {} {
  global mycall;
  variable prev_ident;
  variable min_time_between_ident;
  
  set now [clock seconds];
  if {$now-$prev_ident < $min_time_between_ident} {
    return;
  }
  set prev_ident $now;

  spellWord $mycall;
}


#
# Executed when an unknown DTMF command is entered
#
proc unknown_command {cmd} {
  spellWord $cmd;
  playMsg "Core" "unknown_command";
}


#
# Executed when an entered DTMF command failed
#
proc command_failed {cmd} {
  spellWord $cmd;
  playMsg "Core" "operation_failed";
}


#
# Executed when a link to another logic core is activated.
#
proc activating_link {name} {
  playMsg "Core" "activating_link_to";
  spellWord $name;
}


#
# Executed when a link to another logic core is deactivated
#
proc deactivating_link {name} {
  playMsg "Core" "deactivating_link_to";
  spellWord $name;
}


#
# Executed when trying to deactivate a link to another logic core but the
# link is not currently active.
#
proc link_not_active {name} {
  playMsg "Core" "link_not_active";
  spellWord $name;
}


#
# Executed when trying to activate a link to another logic core but the
# link is already active.
#
proc link_already_active {name} {
  playMsg "Core" "link_already_active";
  spellWord $name;
}



# end of namespace
}

#
# This file has not been truncated
#
