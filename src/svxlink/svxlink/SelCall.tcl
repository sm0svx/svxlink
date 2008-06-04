###############################################################################
#
# Generates selcall tone-sequences according
# the modes: ZVEI1,ZVEI2,ZVEI3,EEA,EIA,CCITT,
# PZVEI,DZVEI,CCIR,VDEW and NATEL
# ATTENTION: I have found different tone definitions
# in the internet, no guarantee!!
#
# Usage: Use the functions SelCall::setMode, SelCall::setAmplitude and
#        SelCall::setFirstToneLength to set the selcall encoder up.
#        Then use the SelCall::play function to send a sequence.
#
# Author: Adi, DL1HRC
#
###############################################################################

namespace eval SelCall {

# The amplitude of the selcall audio. Set by function setAmplitude.
variable amplitude;

# Tone array for the selected mode. Set by function setMode.
variable tones;

# The length of the tones. Set by function setMode.
variable tone_length;

# The length of the first tone in a sequence. Set by function setMode.
variable first_tone_length;

# Tone definitions for ZVEI1
array set ZVEI1 {
  "0" 2400
  "1" 1060
  "2" 1160
  "3" 1270
  "4" 1400
  "5" 1530
  "6" 1670
  "7" 1830
  "8" 2000
  "9" 2200
  "A" 2800
  "B" 810
  "C" 970
  "D" 885
  "E" 2600
  "F" 680
  "N" 2800
  "W" 2600
  "H" 885
  "U" 1750
  "L" 1860
  "J" 2135
  "P" 2280
}

# Tone definitions for ZVEI2
array set ZVEI2 {
  "0" 2400
  "1" 1060
  "2" 1160
  "3" 1270
  "4" 1400
  "5" 1530
  "6" 1670
  "7" 1830
  "8" 2000
  "9" 2200
  "A" 885
  "B" 825
  "C" 740
  "D" 680
  "E" 970
  "F" 2600
  "H" 2800
  "U" 1750
  "L" 1860
  "J" 2135
  "N" 885
  "P" 2280
  "W" 970
}

# Tone definitions for ZVEI3
array set ZVEI3 {
  "0" 2400
  "1" 1060
  "2" 1160
  "3" 1270
  "4" 1400
  "5" 1530
  "6" 1670
  "7" 1830
  "8" 2000
  "9" 2200
  "A" 885
  "B" 810
  "C" 2800
  "D" 680
  "E" 970
  "F" 2600
}

# Tone definitions for PZVEI
array set PZVEI {
  "0" 2400
  "1" 1060
  "2" 1160
  "3" 1270
  "4" 1400
  "5" 1530
  "6" 1670
  "7" 1830
  "8" 2000
  "9" 2200
  "A" 970
  "B" 810
  "C" 2800
  "D" 885
  "E" 2600
  "F" 680
}

# Tone definitions for DZVEI
array set DZVEI {
  "0" 2200
  "1" 970
  "2" 1060
  "3" 1160
  "4" 1270
  "5" 1400
  "6" 1530
  "7" 1670
  "8" 1830
  "9" 2000
  "A" 970
  "B" 740
  "C" 2600
  "D" 885
  "E" 2400
  "F" 680
}

# Tone definitions for EEA
array set EEA {
  "0" 1981
  "1" 1124
  "2" 1197
  "3" 1275
  "4" 1358
  "5" 1446
  "6" 1540
  "7" 1640
  "8" 1747
  "9" 1860
  "A" 1055
  "B" 930
  "C" 2400
  "D" 991
  "E" 2110
  "F" 2247
  "E" 2110
  "W" 2119
  "H" 2247
  "U" 1750
  "L" 1860
  "J" 2135
  "N" 1055
  "P" 2280
}

# Tone definitions for CCIR
array set CCIR {
  "0" 1981
  "1" 1124
  "2" 1197
  "3" 1275
  "4" 1358
  "5" 1446
  "6" 1540
  "7" 1640
  "8" 1747
  "9" 1860
  "A" 2400
  "B" 930
  "C" 2247
  "D" 991
  "E" 2110
  "F" 1055
}

# Tone definitions for VDEW
array set VDEW {
  "0" 2280
  "1" 370
  "2" 450
  "3" 550
  "4" 675
  "5" 825
  "6" 1010
  "7" 1240
  "8" 1520
  "9" 1860
  "A" 2000
  "B" 2100
  "C" 2200
  "D" 2300
  "E" 2400
}

# Tone definitions for CCITT
array set CCITT {
  "0" 400
  "1" 697
  "2" 770
  "3" 852
  "4" 941
  "5" 1209
  "6" 1335
  "7" 1477
  "8" 1633
  "9" 1800
  "A" 1900
  "B" 2000
  "C" 2100
  "D" 2200
  "E" 2300
}

# Tone definitions for NATEL
array set NATEL {
  "0" 1633
  "1" 631
  "2" 697
  "3" 770
  "4" 852
  "5" 941
  "6" 1040
  "7" 1209
  "8" 1363
  "9" 1477
  "A" 1633
  "B" 600
  "C" 1995
  "D" 2205
  "E" 1805
}

# Tone definitions for EIA
array set EIA {
  "0" 600
  "1" 741
  "2" 882
  "3" 1023
  "4" 1164
  "5" 1305
  "6" 1446
  "7" 1587
  "8" 1728
  "9" 1869
  "A" 2151
  "B" 2433
  "C" 2010
  "D" 2292
  "E" 459
  "F" 1091
  "N" 2151
  "W" 459
  "H" 2433
  "U" 1750
  "L" 1860
  "J" 2135
  "P" 2280
}

# Tone length definitions for the supported selcall modes
array set mode_tone_length {
  "EIA"   33
  "EEA"   40
  "ZVEI1" 70
  "ZVEI2" 70
  "ZVEI3" 70
  "DZVEI" 70
  "NATEL" 70
  "CCIR"  100
  "MODAT" 100
  "PZVEI" 100
  "CCITT" 100
  "VDEW"  100
}


#
# Set the amplitude in per mille (0-1000) of full amplitude.
#
proc setAmplitude {new_amplitude} {
  variable amplitude $new_amplitude;
}


#
# Set the selcall mode to use
#   mode  - The selcall mode (ZVEI1, ZVEI2, ZVEI3, PZVEI, DZVEI, EEA,
#     	      	      	      CCIR, VDEW, CCITT, NATEL, EIA)
#
proc setMode {mode} {
  variable mode_tone_length;
  variable tones;
  variable CCIR;
  variable ZVEI1;
  variable ZVEI2;
  variable ZVEI3;
  variable PZVEI;
  variable DZVEI;
  variable EEA;
  variable CCIR;
  variable VDEW;
  variable CCITT;
  variable NATEL;
  variable EIA;
  
  array set tones [array get $mode];  # copy the arrays depending on
      	      	      	      	      # the requested mode
  variable tone_length $mode_tone_length($mode);
  setFirstToneLength $tone_length;
}


#
# Set the length of the first tone in a selcall sequence
#   length - The length, in milliseconds, of the first tone
#
proc setFirstToneLength {length} {
  variable first_tone_length $length;
}


#
# Play a selcall sequence
#   selcallnr - The selcall number to transmit
#
proc play {selcallnr} {
  variable amplitude;
  variable tones;
  variable tone_length;
  variable first_tone_length;

  set last "";
  set tlen $first_tone_length;
  
  foreach tone [split $selcallnr ""] {
    if {$tone == $last} { # repeat-tone: "34433" -> "34E3E"
      set tone "E";
    }
    playTone $tones($tone) $amplitude $tlen;
    set last $tone; # remember last tone
    set tlen $tone_length;
  }
}


# Set defaults
setAmplitude 300;
setMode "CCIR";
#setFirstToneLength 1000;

}

