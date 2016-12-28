###############################################################################
#
# This code is used to send morse code messages.
#
# This file is sourced by the main event handler file so no manual inclusion
# is necessary. Use functions CW::setWpm or CW::setCpm to set the CW speed.
# Use functions CW::setPitch to set the pitch and CW::setAmplitude to set the
# amplitude. Then use CW::play to play some text. See documentation for each
# function below.
#
###############################################################################

namespace eval CW {

# Timing variables
variable short_len;
variable long_len;
variable char_spacing;
variable letter_spacing;
variable word_spacing;

# The pitch of the CW audio
variable fq;

# The amplitude of the CW audio
variable amplitude;

# Morse code table
array set letters {
  "A" ".-"
  "B" "-..."
  "C" "-.-."
  "D" "-.."
  "E" "."
  "F" "..-."
  "G" "--."
  "H" "...."
  "I" ".."
  "J" ".---"
  "K" "-.-"
  "L" ".-.."
  "M" "--"
  "N" "-."  
  "O" "---"
  "P" ".--."
  "Q" "--.-"
  "R" ".-."
  "S" "..."
  "T" "-"
  "U" "..-"
  "V" "...-"
  "W" ".--"
  "X" "-..-"
  "Y" "-.--"
  "Z" "--.."
  
  "0" "-----"
  "1" ".----"
  "2" "..---"
  "3" "...--"
  "4" "....-"
  "5" "....."
  "6" "-...."
  "7" "--..."
  "8" "---.."
  "9" "----."
  
  "." ".-.-.-"
  "," "--..--"
  "?" "..--.."
  "/" "-..-."
  "=" "-...-"

  " " " "
}


# Private function
proc calculateTimings {} {
  variable short_len;
  variable long_len [expr $short_len * 3];
  variable char_spacing $short_len;
  variable letter_spacing [expr $short_len * 3];
  variable word_spacing [expr $short_len * 7];
}


#
# Set the CW speed in words per minute
#
proc setWpm {wpm} {
  variable short_len [expr 60000 / (50 * $wpm)];
  calculateTimings;
}


#
# Set the CW speed in characters per minute
#
proc setCpm {cpm} {
  variable short_len [expr 60000 / (10 * $cpm)];
  calculateTimings;
}


#
# Set the pitch (frequency), in Hz, of the CW audio.
#
proc setPitch {new_fq} {
  variable fq $new_fq;
}


#
# Set the amplitude in per mille (0-1000) of full amplitude.
#
proc setAmplitude {new_amplitude} {
  variable amplitude $new_amplitude;
}


#
# Play the given CW text
#
proc play {txt} {
  variable short_len;
  variable long_len;
  variable char_spacing;
  variable letter_spacing;
  variable word_spacing;
  variable fq;
  variable amplitude;
  variable letters;
  
  set txt [string toupper $txt];
  set first_letter 1;
  foreach letter [split $txt ""] {
    if {!$first_letter} {
      playSilence $letter_spacing;
    }
    set first_letter 0;
    set first_char 1;
    foreach char [split $letters($letter) ""] {
      if {!$first_char} {
      	playSilence $char_spacing;
      }
      set first_char 0;
      if {$char == "."} {
      	playTone $fq $amplitude $short_len;
      } elseif {$char == "-"} {
      	playTone $fq $amplitude $long_len;  
      } elseif {$char == " "} {
      	playSilence $word_spacing;
      }
    }
  }
}


# Set defaults
setPitch 800;
setAmplitude 500;
setCpm 100;

}
