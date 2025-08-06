###############################################################################
#
# MetarInfo module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleMetarInfo] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval MetarInfo {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


# METAR as raw txt to make them available in a file
proc metar {input} {
  set fp [open "/tmp/metar" w];
  puts $fp $input;
  close $fp
}


# no airport defined
proc no_airport_defined {} {
   playMsg "no";
   playMsg "airport";
   playMsg "defined";
   playSilence 200;
}


# no airport defined
proc no_such_airport {} {
   playMsg "no";
   playMsg "such";
   playMsg "airport";
   playSilence 200;
}


# METAR not valid
proc metar_not_valid {} {
  playMsg "metarinformation";
  playMsg "not";
  playMsg "valid";
   playSilence 200;
}


# MET-report TIME
proc metreport_time item {
   playMsg "metreport_time";
   spellNumber $item;
   playSilence 200;
}


# visibility
proc visibility args {
  playMsg "visibility";
  foreach item $args {
    if [regexp {(\d+)} $item] {
         sayNumber $item;
    } else {
      playMsg $item;
    }
    playSilence 100;
  }
  playSilence 200;
}


# temperature
proc temperature {temp} {
  playMsg "temperature";
  playSilence 100;
  if {$temp == "not"} {
    playMsg "not";
    playMsg "reported";
  } else {
    if { int($temp) < 0} {
       playMsg "minus";
       set temp [string trimleft $temp "-"];
    }
    spellNumber $temp;
    if {int($temp) == 1} {
      playMsg "unit_degree";
    } else {
      playMsg "unit_degrees";
    }
    playSilence 100;
  }
  playSilence 200;
}


# dewpoint
proc dewpoint {dewpt} {
  playMsg "dewpoint";
  playSilence 100;
  if {$dewpt == "not"} {
    playMsg "not";
    playMsg "reported";
  } else {
    if { int($dewpt) < 0} {
       playMsg "minus";
       set dewpt [string trimleft $dewpt "-"];
    }
    spellNumber $dewpt;
    if {int($dewpt) == 1} {
      playMsg "unit_degree";
    } else {
      playMsg "unit_degrees";
    }
    playSilence 100;
  }
  playSilence 200;
}


# sea level pressure
proc slp {slp} {
  playMsg "slp";
  spellNumber $slp;
  playMsg "unit_hPa";
  playSilence 200;
}


# flightlevel
proc flightlevel {level} {
  playMsg "flightlevel";
  spellNumber $level;
  playSilence 200;
}


# No specific reports taken
proc nospeci {} {
  playMsg "no";
  playMsg "specific_reports_taken";
  playSilence 100;
}


# peakwind
proc peakwind {val} {
  playMsg "pk_wnd";
  playSilence 100;
  playNumber $val;
  playSilence 200;
}


# wind
proc wind {deg {vel 0 } {unit 0} {gusts 0} {gvel 0}} {

  playMsg "wind";

  if {$deg == "calm"} {
    playMsg "calm";
  } elseif {$deg == "variable"} {
    playMsg "variable";
    playSilence 200;
    spellNumber $vel;
    playMsg $unit;
  } else {
    sayNumber $deg;
    playMsg "unit_degree";
    playSilence 100;
    playMsg "at";
    playSilence 100;
    spellNumber $vel;
    playMsg $unit;
    playSilence 100;

    if {$gusts > 0} {
      playMsg "gusts_up";
      spellNumber $gusts;
      playMsg $gvel;
    }
  }
  playSilence 200;
}


# weather actually
proc actualWX args {
  foreach item $args {
    if [regexp {(\d+)} $item] {
      spellNumber $item;
    } else {
      playMsg $item;
    }
  }
  playSilence 200;
}


# wind varies $from $to
proc windvaries {from to} {
   playMsg "wind";
   playSilence 50;
   playMsg "varies_from";
   playSilence 100;

   sayNumber $from;
   playSilence 100;

   playMsg "to";
   playSilence 100;
   sayNumber $to;

   playMsg "unit_degrees";
   playSilence 200;
}


# Peak WIND
proc peakwind {deg kts hh mm} {
   playMsg "pk_wnd";
   playMsg "from";
   playSilence 100;
   playNumber $deg;
   playMsg "unit_degrees";

   playSilence 100;
   playNumber $kts;
   playMsg "unit_kts";
   playSilence 100;
   playMsg "at";
   if {$hh != "XX"} {
      playNumber $hh;
   }
   playNumber $mm;
   playMsg "utc";
   playSilence 200;
}


# ceiling varies $from $to
proc ceilingvaries {from to} {
   playMsg "ca";
   playSilence 50;
   playMsg "varies_from";
   playSilence 100;
   set from [expr {int($from) * 100}];
   sayNumber $from;
   playSilence 100;

   playMsg "to";
   playSilence 100;
   set to [expr {int($to)*100}];
   sayNumber $to;

   playMsg "unit_feet";
   playSilence 200;
}

# runway visual range
proc rvr args {
   playMsg "rwy";
   foreach item $args {
     if [regexp {(\d+)} $item] {
       sayNumber $item;
     } else {
       playMsg $item;
     }
     playSilence 100;
   }
   playSilence 200;
}


# airport is closed due to snow
proc snowclosed {} {
   playMag "aiport";
   playMag "closed";
   playMsg "due_to"
   playMsg "sn";
   playSilence 200;
}


# RWY is clear
proc all_rwy_clear {} {
  playMsg "all";
  playMsg "runways";
  playMsg "clr";
  playSilence 200;
}


# Runway designator
proc runway args {
  foreach item $args {
    if [regexp {(\d+)} $item] {
      spellNumber $item;
    } else {
      playMsg $item;
    }
    playSilence 100;
  }
  playSilence 200;
}


# time
proc utime {utime} {
   playNumber $utime;
   playSilence 100;
   playMsg "utc";
   playSilence 200;
}


# vv100 -> "vertical view (ceiling) 1000 feet"
proc ceiling {param} {
   playMsg "ca";
   playSilence 100;
   sayNumber $param;
   playSilence 100;
   playMsg "unit_feet";
   playSilence 200;
}


# QNH
proc qnh {value} {
  playMsg "qnh";
  if {$value == 1000} {
     playNumber 1;
     playMsg "thousand";
  } else {
     spellNumber $value;
  }
  playMsg "unit_hPa";
  playSilence 200;
}


# altimeter
proc altimeter {value} {
  playMsg "altimeter";
  playSilence 100;
  spellNumber $value;
  playMsg "unit_inches";
  playSilence 200;
}


# trend
proc trend args {
  playMsg "trend";
  foreach item $args {
    playMsg $item;
    playSilence 100;
  }
  playSilence 200;
}


# clouds with arguments
proc clouds {obs height {cbs ""}} {

  playMsg $obs;
  playSilence 100;
  sayNumber $height;
  playSilence 100;
  playMsg "unit_feet";

  if {[string length $cbs] > 0} {
    playMsg $cbs;
  }
  playSilence 200;
}


# temporary weather obscuration
proc tempo_obscuration {from until} {
  playMsg "tempo";
  playSilence 100;
  playMsg "obsc";
  playSilence 200;
  playMsg "from";
  playNumber $from;
  playSilence 200;
  playMsg "to";
  playSilence 100;
  playNumber $until;
  playSilence 200;
}


# max day temperature
proc max_daytemp {deg time} {
  playMsg "predicted";
  playSilence 50;
  playMsg "maximal";
  playSilence 50;
  playMsg "daytime_temperature";
  playSilence 150;
  playNumber $deg;
  playMsg "unit_degrees";
  playSilence 150;
  playMsg "at";
  playSilence 50;
  playNumber $time;
  playSilence 200;
}


# min day temperature
proc min_daytemp {deg time} {
  playMsg "predicted";
  playSilence 50;
  playMsg "minimal";
  playSilence 50;
  playMsg "daytime_temperature";
  playSilence 150;
  spellNumber $deg;
  playMsg "unit_degrees";
  playSilence 150;
  playMsg "at";
  playSilence 50;
  playNumber $time;
  playSilence 200;
}


# Maximum temperature in RMK section
proc rmk_maxtemp {val} {
  playMsg "maximal";
  playMsg "temperature";
  playMsg "last";
  playNumber 6;
  playMsg "hours";
  if {$val < 0} {
    playMsg "minus";
  }
  spellNumber $val;
  playMsg "unit_degrees";
  playSilence 200;
}


# Minimum temperature in RMK section
proc rmk_mintemp {val} {
  playMsg "minimal";
  playMsg "temperature";
  playMsg "last";
  playNumber 6;
  playMsg "hours";
  if {$val < 0} {
    playMsg "minus";
  }
  spellNumber $val;
  playMsg "unit_degrees";
  playSilence 200;
}


# the begin of RMK section
proc remarks {} {
  playSilence 200;
  playMsg "remarks";
  playSilence 200;
}


# RMK section pressure trend next 3 h
proc rmk_pressure {val args} {
  playMsg "pressure";
  playMsg "tendency";
  playMsg "next";
  playNumber 3;
  playMsg "hours";
  playSilence 150;
  playNumber $val;
  playSilence 150;
  playMsg "unit_mbs";
  playSilence 250;

  foreach item $args {
     if [regexp {(\d+)} $item] {
       sayNumber $item;
     } else {
       playMsg $item;
     }
     playSilence 100;
  }
  playSilence 200;
}


# precipitation last hours in RMK section
proc rmk_precipitation {hour val} {
  playMsg "precipitation";
  playMsg "last";

  if {$hour == "1"} {
     playMsg "hour";
  } else {
     playNumber $hour;
     playMsg "hours";
  }

  playSilence 150;
  playNumber $val;
  playMsg "unit_inches";
  playSilence 200;
}

# precipitations in RMK section
proc rmk_precip {args} {
  foreach item $args {
     if [regexp {(\d+)} $item] {
       sayNumber $item;
     } else {
       playMsg $item;
     }
     playSilence 100;
  }
  playSilence 200;
}


# daytime minimal/maximal temperature
proc rmk_minmaxtemp {max min} {
  playMsg "daytime";
  playMsg "temperature";
  playMsg "maximum";
  if { $max < 0} {
     playMsg "minus";
     set max [string trimleft $max "-"];
  }
  spellNumber $min;
  playMsg "unit_degrees";

  playMsg "minimum";
  if { $min < 0} {
     playMsg "minus";
     set min [string trimleft $min "-"];
  }
  spellNumber $max;
  playMsg "unit_degrees";
  playSilence 200;
}


# recent temperature and dewpoint in RMK section
proc rmk_tempdew {temp dewpt} {
  playMsg "re";
  playMsg "temperature";
  if { $temp < 0} {
     playMsg "minus";
     set temp [string trimleft $temp "-"];
  }

  spellNumber $temp;
  playMsg "unit_degrees";
  playSilence 200;
  playMsg "dewpoint";
  if { $dewpt < 0} {
     playMsg "minus";
     set dewpt [string trimleft $dewpt "-"];
  }
  spellNumber $dewpt;
  playMsg "unit_degrees";
  playSilence 200;
}


# wind shift
proc windshift {val} {
  playMsg "wshft";
  playSilence 100;
  playMsg "at";
  playSilence 100;
  playNumber $val;
  playSilence 200;
}

# QFE value
proc qfe {val} {
  playMsg "qfe";
  spellNumber $val;
  playMsg "unit_hPa";
  playSilence 200;
}

# runwaystate
proc runwaystate args {
  foreach item $args {
    if [regexp {(\d+)} $item] {
      sayNumber $item;
    } else {
      playMsg $item;
    }
    playSilence 200;
  }
  playSilence 200;
}


# output numbers
proc sayNumber { number } {
  variable ts;
  variable hd;

  if {$number > 99 && $number < 10000} {
    if [ expr {$number % 100} ] {
      spellNumber $number;
    } else {
      set ts [expr {int($number / 1000)}];   # 1...9 thousand
      set hd [expr {(($number - $ts * 1000)/100) * 100}];  # 1...9 houndred

      # say 1...9 thousand
      if { $ts > 0 } {
        playMsg $ts;
        playMsg "thousand";
      }
      if { $hd > 0 } {
        playMsg $hd;
      }
    }
  } else {
    spellNumber $number;
  }
}


# output
proc say args {
  variable tsay;

  playSilence 100;
  foreach item $args {
    if [regexp {^(\d+)} $item] {
      sayNumber $item;
    } else {
      if {$item == "."} {
        playMsg "decimal";
      } else {
        playMsg $item;
      }
    }
    playSilence 100;
  }
  playSilence 200;
}


# part 1 of help #01
proc icao_available {} {
   playMsg "icao_available";
   playSilence 200;
}


# announce airport at the beginning of the MEATAR
proc announce_airport {icao} {
  global langdir;
  if [file exists "$langdir/MetarInfo/$icao.wav"] {
    playMsg $icao;
  } else {
    spellWord $icao;
  }
  playSilence 100;
  playMsg "airport";
}


# say preconfigured airports
proc airports args {
  global langdir;
#  global lang;
  variable tval;

  foreach item $args {

     # is a number??

     if {[regexp {(\d+)} $item tval]} {
       sayNumber $tval;
     } else {
       if [file exists "$langdir/MetarInfo/$item.wav"] {
         playFile "$langdir/MetarInfo/$item.wav";
       } else {
         spellWord $item;
       }
     }
     playSilence 100;
  }
  playSilence 200;
}


# say clouds with covering
proc cloudtypes {} {
variable a 0;
  variable l [llength $args];

  while {$a < $l} {
    set msg [lindex $args $a];
    playMsg "cld_$msg";
    playMsg "covering";
    incr a;
    playNumber [lindex $args $a];
    playMsg "eighth";
    incr a;
    playSilence 100;
  }
}


#
# Spell the specified number
#
proc playNr {number} {
  for {set i 0} {$i < [string length $number]} {set i [expr $i + 1]} {
    set ch [string index $number $i];
    if {$ch == "."} {
      playMsg "decimal";
    } else {
      playMsg "$ch";
    }
  }
}


# end of namespace
}

#
# This file has not been truncated
#
