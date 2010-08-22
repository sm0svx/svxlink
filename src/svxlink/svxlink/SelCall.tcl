###############################################################################
#
# Generates selcall tone-sequences according
# the modes: ZVEI1,ZVEI2,ZVEI3,PZVEI,PDZVEI,DZVEI,EEA,EIA,CCITT,
# CCIR1,CCIR2,EURO,VDEW, MODAT and NATEL
# ATTENTION: I have found different tone definitions
# in the internet, no guarantee!!
#
# Usage: Use the functions SelCall::setMode, SelCall::setAmplitude and
#        SelCall::setFirstToneLength to set the selcall encoder up.
#        Then use the SelCall::play function to send a sequence.
#
# Author: Adi, DL1HRC
# Rel:    1.0 (2010-04-11) - Adi: checking tones and lengths
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
  "B" 810
  "C" 740
  "D" 680
  "E" 970
  "F" 2600
}

# Tone definitions for ZVEI3
array set ZVEI3 {
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
  "A" 885
  "E" 2400
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
  "A" 825
  "B" 740
  "C" 2600
  "D" 885
  "E" 2400
  "F" 680
}

# Tone definitions for PDZVEI
array set PDZVEI {
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
  "A" 825
  "B" 886
  "C" 2600
  "D" 856
  "E" 2400
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
}

# Tone definitions for CCIR1
array set CCIR1 {
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

# Tone definitions for CCIR2
array set CCIR2 {
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

# Tone definitions for PCCIR
array set PCCIR {
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
  "A" 1050
  "B" 930
  "C" 2400
  "D" 991
  "E" 2110
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
  "8" 1336
  "9" 1477
  "A" 1995
  "B" 571
  "C" 2205
  "D" 2437
  "E" 1805
  "F" 2694
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
}

# Tone definitions for EURO
array set EURO {
  "0" 979.8
  "1" 903.1
  "2" 832.5
  "3" 767.4
  "4" 707.4
  "5" 652.0
  "6" 601.0
  "7" 554.0
  "8" 510.7
  "9" 470.8
  "A" 433.9
  "B" 400.0
  "C" 368.7
  "D" 1153.1
  "E" 1062.9
  "F" 339.9
}

# Tone definitions for MODAT
array set MODAT {
  "0" 637.5
  "1" 787.5
  "2" 937.5
  "3" 1087.5
  "4" 1237.5
  "5" 1387.5
  "6" 1537.5
  "7" 1687.5
  "8" 1837.5
  "9" 1987.5
  "E" 487.5
}

# Tone definitions for AUTO-A
array set AUTOA {
  "0" 1962
  "1" 764
  "2" 848
  "3" 942
  "4" 1047
  "5" 1163
  "6" 1292
  "7" 1436
  "8" 1595
  "9" 1770
  "B" 2430
  "E" 2188
}


# Tone length definitions for the supported selcall modes
array set mode_tone_length {
  "EIA"   33
  "EEA"   40
  "MODAT"  40
  "ZVEI1" 70
  "ZVEI2" 70
  "ZVEI3" 70
  "DZVEI" 70
  "PDZVEI" 70
  "NATEL" 70
  "AUTOA"  70
  "CCIR1" 100
  "CCIR2"  70
  "PCCIR" 100
  "PZVEI" 100
  "CCITT" 100
  "VDEW"  100
  "EURO"  100
}


#
# Set the amplitude in per mille (0-1000) of full amplitude.
#
proc setAmplitude {new_amplitude} {
  variable amplitude $new_amplitude;
}


#
# Set the selcall mode to use
#   mode  - The selcall mode (ZVEI1, ZVEI2, ZVEI3, PZVEI, DZVEI, PDZVEI, EEA,
#     	      	      	      VDEW, CCITT, NATEL, EIA, EURO, MODAT, CCIR1,
#                             PCCIR, CCIR2, AUTOA)
#
proc setMode {mode} {
  variable mode_tone_length;
  variable tones;
  variable ZVEI1;
  variable ZVEI2;
  variable ZVEI3;
  variable PZVEI;
  variable DZVEI;
  variable PDZVEI;
  variable EEA;
  variable CCIR1;
  variable CCIR2;
  variable PCCIR;
  variable MODAT;
  variable VDEW;
  variable AUTOA;
  variable CCITT;
  variable NATEL;
  variable EIA;
  variable EURO;
  
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
setMode "CCIR1";
#setFirstToneLength 1000;

}

