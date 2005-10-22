###############################################################################
#
# This is the main file for the SvxLink TCL script event handling subsystem.
# It provides some basic functions for playing sounds and spelling words to
# the event handling functions. The event handling functions is read from
# a subdirectory called "events.d".
#
###############################################################################

#
# Play a message in a certain context. A context can for example be Core,
# EchoLink, Help, Parrot etc. If a sound is not found in the specified context,
# a search in the "Default" context is done.
#
proc playMsg {context msg} {
  global basedir;
  if [file exists "$basedir/$context/$msg.raw"] {
    playFile "$basedir/$context/$msg.raw";
  } else {
    playFile "$basedir/Default/$msg.raw";
  }
}


#
# Spell the specified word using a phonetic alphabet
#
proc spellWord {word} {
  set word [string tolower $word];
  for {set i 0} {$i < [string length $word]} {set i [expr $i + 1]} {
    set char [string index $word $i];
    if {$char == "*"} {
      playMsg "Default" "star";
    } elseif {$char == "/"} {
      playMsg "Default" "slash";
    } elseif {$char == "-"} {
      playMsg "Default" "dash";
    } else {
      playMsg "Default" "phonetic_$char";
    }
  }
}


#
# Spell the specified number
#
proc playNumber {number} {
  for {set i 0} {$i < [string length $number]} {set i [expr $i + 1]} {
    set ch [string index $number $i];
    if {$ch == "."} {
      playMsg "Default" "decimal";
    } else {
      playMsg "Default" "$ch";
    }
  }
}



###############################################################################
#
# Main program
#
###############################################################################

set basedir [file dirname $script_path];

foreach {file} [glob -directory $basedir/events.d *.tcl] {
  source $file;
}

if [info exists is_core_event_handler] {
  puts "Event handler script successfully loaded.";
}


#
# This file has not been truncated
#
