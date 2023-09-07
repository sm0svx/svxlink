###############################################################################
#
# This is the main file for the SvxLink TCL script event handling subsystem.
# It loads the event handling scripts and provides some basic functions for
# playing sounds to. The event handling functions are read from the following
# subdirectories:
#
#   events.d        - Main event script directory
#   events.d/local  - Local modifications to event handling scripts
#
# The same structure is also available, if needed, in the sound clip
# directories for each language.
#
###############################################################################

#
# Play a message in a certain context. A context can for example be Core,
# EchoLink, Help, Parrot etc. If a sound is not found in the specified context,
# a search in the "Default" context is done.
#
# It's also possible to have local overrides by putting files under a "local"
# directory either directly under the "sounds" directory or under the language
# pack directory. For example if context is "Core" and the language is set to
# "en_US" the following paths will be searched:
#
#   .../sounds/en_US/local/Core/
#   .../sounds/local/Core/
#   .../sounds/en_US/Core/
#   .../sounds/en_US/local/Default/
#   .../sounds/local/Default/
#   .../sounds/en_US/Default/
#
#   context   - The context to look for the sound files in (e.g Default,
#               Parrot etc).
#   msg       - The basename of the file to play
#   warn      - Set to 0 to not print a warning if no sound clip was found
#
proc playMsg {context msg {warn 1}} {
  global basedir
  global langdir

  set candidates [glob -nocomplain \
      "$langdir/local/$context/$msg.{wav,raw,gsm}" \
      "$basedir/sounds/local/$context/$msg.{wav,raw,gsm}" \
      "$langdir/$context/$msg.{wav,raw,gsm}" \
      "$langdir/local/Default/$msg.{wav,raw,gsm}" \
      "$basedir/sounds/local/Default/$msg.{wav,raw,gsm}" \
      "$langdir/Default/$msg.{wav,raw,gsm}"];
  if {[llength $candidates] > 0} {
    playFile [lindex $candidates 0];
  } else {
    if {$warn} {
      puts "*** WARNING: Could not find audio clip \"$msg\" in context \"$context\"";
    }
    return 0
  }
  return 1
}


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
  global basedir
  global langdir

  set subcmds [glob -nocomplain "$langdir/$context/$basename*.{wav,raw,gsm}"]
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
# Process the given event.
# All TCL modules should use this function instead of calling playMsg etc
# directly. The module code should only contain the logic, not the handling
# of the event.
#
#   module - The module to process the event in
#   ev     - The event to process
#
proc processEvent {module ev} {
  append func $module "::" $ev
  eval "$func"
}


#
# Read and execute (source) a TCL file
# Check if the given TCL file is readable before trying to source it. Print
# a warning if the file is not readable.
#
#   filename - The file to read
#
proc sourceTcl {filename} {
  if [file readable $filename] {
    source $filename
  } else {
    puts "*** WARNING: Could not load TCL event file: $filename"
  }
}


#
# Get a variable and if it does not exist return the default value
#
#   varname - The name of the variable to get
#   default - The default value to set if the variable is undefined
#
proc getVar {varname default} {
  upvar $varname var
  expr {[info exists var] ? $var : $default}
}


###############################################################################
#
# Main program
#
###############################################################################

set basedir [file dirname $script_path];
if [info exists Logic::CFG_DEFAULT_LANG] {
  set lang $Logic::CFG_DEFAULT_LANG
} else {
  set lang "en_US"
}

set langdir "$basedir/sounds/$lang"

# Source all tcl files in the events.d directory.
# This directory contains the main event handlers.
foreach {file} [glob -directory $basedir/events.d *.tcl] {
  sourceTcl $file
}

# Source all files in the events.d/local directory.
# This directory contains local modifications to the main event handlers.
foreach {file} [glob -nocomplain -directory $basedir/events.d/local *.tcl] {
  sourceTcl $file;
}

# Source all tcl files in the language specific events.d directory.
# This directory contains the main event handlers.
foreach {file} [glob -nocomplain -directory $langdir/events.d *.tcl] {
  sourceTcl $file;
}

# Source all files in the language specific events.d/local directory.
# This directory contains local modifications to the main event handlers.
foreach {file} [glob -nocomplain -directory $langdir/events.d/local *.tcl] {
  sourceTcl $file;
}

# Source all files in the modules.d directory.
# This directory contains modules written in TCL.
foreach {file} [glob -directory $basedir/modules.d *.tcl] {
  sourceTcl $file;
}

if [info exists is_core_event_handler] {
  puts "$logic_name: Event handler script successfully loaded.";
}


#
# This file has not been truncated
#
