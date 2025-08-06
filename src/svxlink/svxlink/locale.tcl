###############################################################################
#
# Locale specific functions for playing back time, numbers and spelling words.
# Often, the functions in this file are the only ones that have to be
# reimplemented for a new language pack.
#
###############################################################################

#
# Spell the specified word using phonetic alphabet or plain letters depending
# on the setting of the PHONETIC_SPELLING configuration variable.
#
#   word -- The word to spell
#
proc spellWord {word} {
  set phonetic_spelling [getConfigValue $::logic_name PHONETIC_SPELLING 1]
  set word [string tolower $word];
  for {set i 0} {$i < [string length $word]} {set i [expr $i + 1]} {
    set char [string index $word $i];
    if {[regexp {[a-z0-9]} $char]} {
      if {$phonetic_spelling == 0} {
        playMsg "Default" "$char";
      } else {
        playMsg "Default" "phonetic_$char";
      }
    } elseif {$char == "/"} {
      playMsg "Default" "slash";
    } elseif {$char == "-"} {
      playMsg "Default" "dash";
    } elseif {$char == "*"} {
      playMsg "Default" "star";
    }
  }
}


#
# Spell the specified number digit for digit
#
# This is a rather stupid function that just read out the digits one by one in
# a given number. There is no check that it's a valid number.
#
proc spellNumber {number} {
  for {set i 0} {$i < [string length $number]} {set i [expr $i + 1]} {
    set ch [string index $number $i];
    if {$ch == "."} {
      playMsg "Default" "decimal"
    } elseif {$ch == "+"} {
      playMsg "Default" "plus"
    } elseif {$ch == "-"} {
      playMsg "Default" "minus"
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
  if {($first == "0") || ($first == "o")} {
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
  if {($first == "0") || ($first == "o")} {
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
# Say a number as intelligently as possible.
#
# Any leading or trailing whitespace is ignored.
# If a number is zero, the sign will be ignored.
# For numbers beginning with a point, a zero is prepended.
# Numbers >= 1000 will be split into three and two digit groups.
# Any leading zeros will be preserved.
#
# Examples:
#
#	1	- one
#	24	- twentyfour
#	245	- twohundred and fourtyfive
#	1234	- twelve thirtyfour
#	12345	- onehundred and twentythree fourtyfive
#	136.5	- onehundred and thirtysix point five
#	007.123	- zero zero seven point one two three
#	.123	- zero point one two three
#	-1	- minus one
#	-0.0	- zero point zero
#	+1.5	- plus one point five
#
proc playNumber {number} {
  if {![regexp {^\s*([+-])?(\d*)(?:\.(\d+))?\s*$} $number \
        -> sign integer fraction]} {
    puts "*** ERROR\[playNumber\]: Invalid number '$number'"
    return
  }

  if {[string length "$integer"] == 0} {
    set integer "0"
  }

  if [expr double("$integer.$fraction") != 0.0] {
    if {$sign == "+"} {
      playMsg "Default" "plus"
    } elseif  {$sign == "-"} {
      playMsg "Default" "minus"
    }
  }

  if {$fraction != ""} {
    playNumber $integer;
    playMsg "Default" "decimal";
    spellNumber $fraction;
    return;
  }

  while {[string length $integer] > 0} {
    set len [string length $integer];
    if {$len == 1} {
      playMsg "Default" $integer;
      set integer "";
    } elseif {$len % 2 == 0} {
      playTwoDigitNumber [string range $integer 0 1];
      set integer [string range $integer 2 end];
    } else {
      playThreeDigitNumber [string range $integer 0 2];
      set integer [string range $integer 3 end];
    }
  }
}


#
# Say the time specified by function arguments "hour" and "minute".
#
proc playTime {hour minute} {
  set time_format [getConfigValue $::logic_name TIME_FORMAT 12]

  # Strip white space and leading zeros. Check ranges.
  if {[scan $hour "%d" hour] != 1 || $hour < 0 || $hour > 23} {
    error "playTime: Non digit hour or value out of range: $hour"
  }
  if {[scan $minute "%d" minute] != 1 || $minute < 0 || $minute > 59} {
    error "playTime: Non digit minute or value out of range: $hour"
  }

  if {$time_format == 24} {
    if {$hour == 0} {
      set hour "00"
    } elseif {[string length $hour] == 1} {
      set hour "o$hour";
    }
    playTwoDigitNumber $hour;

    if {$minute != 0} {
      if {[string length $minute] == 1} {
        set minute "o$minute";
      }
      playTwoDigitNumber $minute;
    }
    playMsg "Default" "hours";
    playSilence 100;
  } else {
    # Anything not 24 will fail back to 12 hour default
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
    }
  
    playMsg "Default" [expr $hour];
    if {$minute != 0} {
      if {[string length $minute] == 1} {
        set minute "o$minute";
      }
      playTwoDigitNumber $minute;
    }
    playSilence 100;
    playMsg "Core" $ampm;
  }
}


#
# Say the given frequency as intelligently as popssible
#
#   fq -- The frequency in Hz
#
proc playFrequency {fq} {
  if {$fq < 1000} {
    set unit "Hz"
  } elseif {$fq < 1000000} {
    set fq [expr {$fq / 1000.0}]
    set unit "kHz"
  } elseif {$fq < 1000000000} {
    set fq [expr {$fq / 1000000.0}]
    set unit "MHz"
  } else {
    set fq [expr {$fq / 1000000000.0}]
    set unit "GHz"
  }
  playNumber [string trimright [format "%.3f" $fq] ".0"]
  playMsg "Core" $unit
}


#
# This file has not been truncated
#
