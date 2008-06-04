##############################################################################
# Generates selcall tone-sequences according
# the modes: ZVEI1,ZVEI2,ZVEI3,EEA,EIA,CCITT,
# PZVEI,DZVEI,CCIR,VDEW and NATEL
# ATTENTION: I have found different tone definitions
# in the internet, no guarantee!!
#
# Author: Adi, DL1HRC
##############################################################################

namespace eval SelCall {

proc play {selcallnr mode} {
  global tones;
  global tone;
  variable last;

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
  array set Tonelength {
    "EIA"   33
    "EEA"   40
    "ZVEI1" 70
    "ZVEI2" 70
    "ZVEI3" 70
    "DZVEI" 70
    "NATEL" 70
    "CCIR"  100
    "MODAT" 100
    "PZVEI" 100
    "CCITT" 100
    "VDEW"  100
  }

  set last "";
  array set tones [array get $mode];   # copy the arrays depending on
                                       # the requested mode

  foreach tone [split $selcallnr ""] {
    if {$last == $tone} {    # repeat-tone: "34433" -> "34E3E"
      $tone="E";
    }
    playTone $tones($tone) 300 $Tonelength($mode);
    set last $tone;          # remember last tone
  }
}

}

