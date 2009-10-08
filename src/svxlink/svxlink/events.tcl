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
  set candidates [glob -nocomplain "$basedir/$context/$msg.{wav,raw,gsm}" "$basedir/Default/$msg.{wav,raw,gsm}"];
  if { [llength $candidates] > 0 } {
    playFile [lindex $candidates 0];
  } else {
    puts "*** WARNING: Could not find audio clip \"$msg\" in context \"$context\"";
  }
  #if [file exists "$basedir/$context/$msg.raw"] {
  #  playFile "$basedir/$context/$msg.raw";
  #} else {
  #  playFile "$basedir/Default/$msg.raw";
  #}
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
    } elseif {[regexp {[a-z0-9]} $char]} {
      playMsg "Default" "phonetic_$char";
    }
  }
}


#
# Spell the specified number digit for digit
#
proc spellNumber {number} {
  for {set i 0} {$i < [string length $number]} {set i [expr $i + 1]} {
    set ch [string index $number $i];
    if {$ch == "."} {
      playMsg "Default" "decimal";
    } else {
      playMsg "Default" "$ch";
    }
  }
}


#
# Say the specified two digit number (00 - 99)
#
proc playTwoDigitNumber {number} {
  if {[string length $number] != 2} {
    puts "*** WARNING: Function playTwoDigitNumber received a non two digit number: $number";
    return;
  }
  
  set first [string index $number 0];
  if {($first == "0") || ($first == "O")} {
    playMsg "Default" $first;
    playMsg "Default" [string index $number 1];
  } elseif {$first == "1"} {
    playMsg "Default" $number;
  } elseif {[string index $number 1] == "0"} {
    playMsg "Default" $number;
  } else {
    playMsg "Default" "[string index $number 0]X";
    playMsg "Default" "[string index $number 1]";
  }
}


#
# Say the specified three digit number (000 - 999)
#
proc playThreeDigitNumber {number} {
  if {[string length $number] != 3} {
    puts "*** WARNING: Function playThreeDigitNumber received a non three digit number: $number";
    return;
  }
  
  set first [string index $number 0];
  if {($first == "0") || ($first == "O")} {
    spellNumber $number
  } else {
    append first "00";
    playMsg "Default" $first;
    if {[string index $number 1] != "0"} {
      playMsg "Default" "and"
      playTwoDigitNumber [string range $number 1 2];
    } elseif {[string index $number 2] != "0"} {
      playMsg "Default" "and"
      playMsg "Default" [string index $number 2];
    }
  }
}


#
# Say a number as intelligent as posible. Examples:
#
#	1	- one
#	24	- twentyfour
#	245	- twohundred and fourtyfive
#	1234	- twelve thirtyfour
#	12345	- onehundred and twentythree fourtyfive
#	136.5	- onehundred and thirtysix decimal five
#
proc playNumber {number} {
  if {[regexp {(\d+)\.(\d+)?} $number -> integer fraction]} {
    playNumber $integer;
    playMsg "Default" "decimal";
    spellNumber $fraction;
    return;
  }

  while {[string length $number] > 0} {
    set len [string length $number];
    if {$len == 1} {
      playMsg "Default" $number;
      set number "";
    } elseif {$len % 2 == 0} {
      playTwoDigitNumber [string range $number 0 1];
      set number [string range $number 2 end];
    } else {
      playThreeDigitNumber [string range $number 0 2];
      set number [string range $number 3 end];
    }
  }
}


proc playTime {hour minute} {
  set hour [string trimleft $hour " "];
  set minute [string trimleft $minute " "];
  
  if {$hour < 12} {
    set ampm "AM";
    if {$hour == 0} {
      set hour 12;
    }
  } else {
    set ampm "PM";
    if {$hour > 12} {
      set hour [expr $hour - 12];
    }
  };
  
  playMsg "Default" [expr $hour];

  if {$minute != 0} {
    if {[string length $minute] == 1} {
      set minute "O$minute";
    }
    playTwoDigitNumber $minute;
  }
  
  playSilence 100;
  playMsg "Core" $ampm;
}


###############################################################################
#
# Main program
#
###############################################################################

set basedir [file dirname $script_path];

# Source all tcl files in the events.d directory
# FIXME: This is a dirty fix to make the PropagationMonitor module to
#        load after Logic.tcl. The "lsort" should not be needed when
#        TCL module loading have been properly implemented.
foreach {file} [lsort [glob -directory $basedir/events.d *.tcl]] {
  source $file;
}

# Source all files in the events.d/local directory
foreach {file} [glob -nocomplain -directory $basedir/events.d/local *.tcl] {
  source $file;
}

if [info exists is_core_event_handler] {
  puts "Event handler script successfully loaded.";
}


#
# This file has not been truncated
#
