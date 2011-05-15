###############################################################################
#
# Generates selcall tone-sequences according
# the modes: ZVEI1,ZVEI2,ZVEI3,PZVEI,PDZVEI,DZVEI,EEA,EIA,CCITT,
# CCIR1,CCIR2,EURO,VDEW, MODAT, NATEL and QC2
# ATTENTION: I have found different tone definitions
# in the internet, no guarantee!!
#
# Usage: Use the functions SelCall::setMode, SelCall::setAmplitude and
#        SelCall::setFirstToneLength to set the selcall encoder up.
#        Then use the SelCall::play function to send a sequence.
#
# Author: Adi, DL1HRC
# Rel:    1.0  (2010-04-11) - Adi: checking tones and lengths
# Rel:    1.01 (2011-04-26) - Adi: added Motorola's Quick Call (QC2)
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

# the silence after a selcall to separate the sequences
variable silence;

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

#
# Stuff for Motorola's Quick Call II modes
# matrix for a 3 digits SelCall
array set qc2_cap_code_matrix {
  "1" 11
  "2" 22
  "3" 12
  "4" 44
  "5" 55
  "6" 21
  "7" 45
  "8" 54
  "9" 24
  "0" 42
  "A" 33
}

array set qc2_tone_matrix {
  "11" 349.0
  "12" 368.5
  "13" 389.0
  "14" 410.8
  "15" 433.7
  "16" 457.9
  "17" 483.5
  "18" 510.5
  "19" 539.0
  "10" 330.5

  "21" 600.9
  "22" 634.5
  "23" 669.9
  "24" 707.3
  "25" 746.8
  "26" 788.5
  "27" 832.5
  "28" 879.0
  "29" 928.1
  "20" 569.1

  "31" 288.5
  "32" 296.5
  "33" 304.7
  "34" 313.0
  "35" 953.7
  "36" 979.9
  "37" 1006.9
  "38" 1034.7
  "39" 1063.2
  "30" 1092.4

  "41" 339.6
  "42" 358.6
  "43" 378.6
  "44" 399.8
  "45" 422.1
  "46" 445.7
  "47" 470.5
  "48" 496.8
  "49" 524.6
  "40" 321.7

  "51" 584.8
  "52" 617.4
  "53" 651.9
  "54" 688.3
  "55" 726.8
  "56" 767.4
  "57" 810.2
  "58" 855.5
  "59" 903.2
  "50" 553.9

  "61" 1153.4
  "62" 1185.2
  "63" 1217.8
  "64" 1251.4
  "65" 1285.8
  "66" 1321.2
  "67" 1357.6
  "68" 1395.0
  "69" 1433.4
  "60" 1122.5

  "A1" 1472.9
  "A2" 1513.5
  "A3" 1555.2
  "A4" 1598.0
  "A5" 1642.0
  "A6" 1687.2
  "A7" 1733.7
  "A8" 1781.5
  "A9" 1830.5
  "A0" 1881.0

  "B1" 1930.2
  "B2" 1989.0
  "B3" 2043.8
  "B4" 2094.5
  "B5" 2155.6
  "B6" 2212.2
  "B7" 2271.7
  "B8" 2334.6
  "B9" 2401.0
  "B0" 2468.2
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
  "QC2"   1000
}

#
# minimal gap between two SelCalls
#
array set mode_silence_length {
  "EIA"    150
  "EEA"    150
  "MODAT"  250
  "ZVEI1"  250
  "ZVEI2"  250
  "ZVEI3"  250
  "DZVEI"  250
  "PDZVEI" 250
  "NATEL"  250
  "AUTOA"  250
  "CCIR1"  250
  "CCIR2"  250
  "PCCIR"  250
  "PZVEI"  250
  "CCITT"  250
  "VDEW"   250
  "EURO"   250
  "QC2"   1000
}

#
# Set the amplitude in per mille (0-1000) of full amplitude.
#
proc setAmplitude {new_amplitude} {
  variable amplitude $new_amplitude;
}

#
# Set the silence at the end of a selcall, minimum gap
# between SelCalls in sequence.
#
proc setSilence {silence_at_end} {
  variable silence $silence_at_end;
}

#
# Set the selcall mode to use
#   sel_mode  - The selcall mode (ZVEI1, ZVEI2, ZVEI3, PZVEI, DZVEI, PDZVEI, EEA,
#     	      	      	      VDEW, CCITT, NATEL, EIA, EURO, MODAT, CCIR1,
#                             PCCIR, CCIR2, AUTOA, QC2)
#
proc setMode {sel_mode} {
  variable mode_tone_length;
  variable mode_silence_length;
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
  variable QC2;

  array set tones [array get $sel_mode];  # copy the arrays depending on
      	      	      	      	      # the requested mode
  variable tone_length $mode_tone_length($sel_mode);
  setFirstToneLength $tone_length;
  setSilence $mode_silence_length($sel_mode);
  variable mode $sel_mode;
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
  variable mode;
  variable silence;

  set last "";
  set tlen $first_tone_length;

  #
  # Motorola QC2 makes it all really long-winded ;-(
  #
  if {$mode == "QC2" } {
    variable toneA;
    variable toneB;
    variable table1;

    #
    # check if a 3 digit cap code
    #
    if {[string length $selcallnr] == 3 } {

      set table1 [string range $selcallnr 0 0];   # is 6

      variable qc2_tone_matrix;
      variable qc2_cap_code_matrix;
      variable tonematrix $qc2_cap_code_matrix($table1);

      # e.g. 635
      set tone1 [string range $selcallnr 1 1];
      set tone2 [string range $selcallnr 2 2];


      # check for a group cap call?
      if { $tone1 != $tone2 } {
        # no group call
        append toneA [string range $tonematrix 0 0] $tone1;
        append toneB [string range $tonematrix 1 1] $tone2;

        # play tone 3 from group 2 and  -> 669.9 Hz
        #      tone 5 from group 1      -> 433.7 Hz
        playTone $qc2_tone_matrix($toneA) $amplitude $tlen; # 1 sec
        playTone $qc2_tone_matrix($toneB) $amplitude 3000;  # 3 sec

      } else {

        # yes, we have a group call
        append toneA $table1 $tone1;
        # group call cap code
        playTone $qc2_tone_matrix($toneA) $amplitude 8000;
      }
    }

    # clear the tones
    set toneA "";
    set toneB "";

  } else {
    foreach tone [split $selcallnr ""] {
      # repeat-tone: "34433" -> "34E3E"
      if {$tone == "*"} {
        set tone "A";
      }
      if {$tone == $last} {
        set tone "E";
      }
      playTone $tones($tone) $amplitude $tlen;
      # remember last tone
      set last $tone;
      set tlen $tone_length;
    }
  }
  playSilence $silence;
}


# Set defaults
setAmplitude 300;
setMode "CCIR1";
#setFirstToneLength 1000;

}

