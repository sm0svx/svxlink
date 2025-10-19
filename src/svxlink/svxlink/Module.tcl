###############################################################################
#
# Generic module event handlers
#
###############################################################################

# Enable calling procedures in the logic core namespace without qualification
namespace path ::${::logic_name}

#
# This is the namespace in which all functions and variables below will exist.
#
namespace eval Module {

# Enable calling procedures in the logic core namespace without qualification
namespace path ::${::logic_name}

# Extract the module name from the parent namespace
set module_name [namespace tail [namespace parent]];

#
# An "overloaded" playMsg that eliminates the need to write the module name
# as the first argument.
# For legacy code support, if more than one argument is given to the function
# it will call the original playMsg using all given arguments
#
proc playMsg {args} {
  variable module_name;
  if {[llength $args] == 1} {
    ::playMsg $module_name $args
  } else {
    ::playMsg {*}$args
  }
}


#
# A convenience function for printing out information prefixed by the
# module name
#
#proc printInfo {msg} {
#  variable module_name;
#  puts "$module_name: $msg";
#}


#
# Play a range of subcommand description files. The file names must be on the
# format <basename><command number>[ABCD*#]. The last characters are optional.
# Each matching sound clip will be played in sub command number order, prefixed
# with the command number.
#
#   context   - The context to look for the sound files in (e.g Default,
#               Parrot etc).
#   basename  - The common basename for the sound clips to find.
#   header    - A header sound clip to play first
#
proc playSubcommands {context basename {header ""}} {
  set subcmds [glob -nocomplain "$::langdir/$context/$basename*.{wav,raw,gsm}"]
  if {[llength $subcmds] > 0} {
    if {$header != ""} {
      playSilence 500
      playMsg $context $header
    }

    append match_exp {^.*/} $basename {(\d+)([ABCD*#]*)\.}
    foreach subcmd [lsort $subcmds] {
      if [regexp $match_exp $subcmd -> number chars] {
        playSilence 200
        playNumber $number
        spellWord $chars
        playSilence 200
        playFile $subcmd
      }
    }
  }
}


#
# Executed when a module is being activated
#
proc activating_module {} {
  ::playMsg "Default" "activating";
  playSilence 100;
  playMsg "name";
  playSilence 200;
}


#
# Executed when a module is being deactivated.
#
proc deactivating_module {} {
  ::playMsg "Default" "deactivating";
  playSilence 100;
  playMsg "name";
  playSilence 200;
}


#
# Executed when the inactivity timeout for a module has expired.
#
proc timeout {} {
  ::playMsg "Default" "timeout";
  playSilence 100;
}


#
# Executed when playing of the help message for a module has been requested.
#
proc play_help {} {
  variable module_name
  playMsg "help"
  playSubcommands $module_name help_subcmd "sub_commands_are"
}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  #printInfo "status_report called...";
}


#
# Called when an illegal command has been entered
#
#   cmd - The received command
#
proc unknown_command {cmd} {
  variable module_name
  if {${::active_module} != ${module_name}} {
    playMsg "name"
    playSilence 200
  }
  playNumber $cmd
  playMsg "unknown_command"
}


# End of namespace
}


#
# This file has not been truncated
#
