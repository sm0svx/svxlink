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

#
# Check if this module is loaded in the current logic core
#
if {![info exists CFG_ID]} {
  return;
}

#
# Extract the module name from the current namespace
#
set module_name [namespace tail [namespace current]];


#
# An "overloaded" playMsg that eliminates the need to write the module name
# as the first argument.
#
proc playMsg {msg} {
  variable module_name;
  ::playMsg $module_name $msg;
}


#
# A convenience function for printing out information prefixed by the
# module name
#
proc printInfo {msg} {
  variable module_name;
  puts "$module_name: $msg";
}


#
# Executed when this module is being activated
#
proc activating_module {} {
  variable module_name;
  Module::activating_module $module_name;
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  variable module_name;
  Module::deactivating_module $module_name;
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  variable module_name;
  Module::timeout $module_name;
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  variable module_name;
  Module::play_help $module_name;
}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  printInfo "status_report called...";
}


proc say { word } {
  playSilence 100;
  playMsg $word;
}


proc clouds { clouds height } {
  variable hh;
  variable lh;

  playMsg $clouds;

  set hh [expr {int($height / 1000)}];
  set lh [expr {$height - $hh * 1000}];
  if {$hh > 0} {
    playMsg $hh;
    playMsg "thousand";
  }
  if {$lh > 0} {
    playMsg $lh;
  }
  playMsg "feet";
}


proc airport { icao } {
  global basedir;
  playSilence 200;
  
  if [file exists "$basedir/MetarInfo/$icao.raw"] {
    playFile "$basedir/MetarInfo/$icao.raw";
  } else {
    spellWord $icao;
  }
  playMsg "Airport";
}


proc more_than_10 { unit } {
  playSilence 200;
  playMsg "visibility";
  playMsg "more_than";
  playNr 10;
  playMsg $unit;
}


proc visibility { view unit } {
  variable ther;
  variable diff;
  variable lch;
  set lch [string index view 0];

  playSilence 200;
  playMsg "visibility";

  if {$lch == "M"} {
    playMsg "less_than";
  }
  if {$lch == "P"} {
    playMsg "more_than";
  }

  if [expr $view < 50 ] {
    playNr  $view;
  } elseif [expr $view < 5000] {
    set ther [expr {int($view/1000)} ];
    set diff [expr {int($view-$ther*1000)} ];
    playNr $ther;
    playMsg "thousand";

    # the 100's
    if [expr $diff > 0 ] {
      playMsg $diff;
    }
  } else {
    playMsg $view;
  }
  playMsg $unit;
}


proc metreport_time { utc } {
  playSilence 200;
  playMsg "metreport_time";
  playNr $utc;
}


proc temperature { temp } {
  playSilence 200;
  playMsg "temperature";
  if { $temp < 0 } {
    playMsg "minus";
  }
  playNr $temp;
  playMsg "degrees";
}


proc dewpoint { dp } {
  playSilence 200;
  playMsg "dewpoint";
  if { $dp < 0 } {
    playMsg "minus";
  }
  playNr $dp;
  playMsg "degrees";
}


proc qnh { qnh unit } {
  playSilence 200;
  playMsg "qnh";
  playNr $qnh;
  playMsg $unit;
}


proc gusts_up { gusts } {
  playSilence 200;
  playMsg "gusts_up";
  playSilence 150;
  playNr $gusts;
  playMsg "knots";
}


proc wind_variable { kts } {
  playSilence 200;
  playMsg "wind_variable";
  playNr $kts;
  playMsg "knots";
}


proc wind_calm {} {
  playSilence 200;
  playMsg "wind";
  playMsg "calm";
}


proc wind_mps { deg mps } {
  playSilence 200;
  playMsg "wind";
  if {$deg == "100"} {
    playMsg "100";
  } elseif {$deg == "200"} {
    playMsg "200";
  } elseif {$deg == "300"} {
    playMsg "300";
  } else {
    playNr $deg;
  }
  playMsg "degrees";
  playSilence 100;
  playNr $mps;
  playMsg "mps";
}


proc wind_varies_from { from to } {
  playSilence 200;
  playMsg "wind_varies_from";
  playNr  $from;
  playSilence 100;
  playMsg "to";
  playSilence 100;
  playNr  $to;
  playMsg "degrees";
}


proc rvr_varies_from { id val } {
  playSilence 200;
  playMsg "rvr_varies_from";
  if {$val == "m"} {
    playMsg "less_than";
  }
  if {$val == "p"} {
    playMsg "more_than";
  }
  playNr  $val;
  playMsg "meters";
}


proc vertical_view {vv} {
  variable vvh;
  variable diff;
  
  playSilence 200;
  playMsg "vv";
  
  set vvh [expr {int($vv/1000)} ];
  set diff [expr {int($vv-$vvh*1000)} ];

  if {$vvh > 0} {
    playNr $vvh;
    playMsg "thousand";
  }
  # the 100's
  if [expr $diff > 0 ] {
      playMsg $diff;
  } else {
    playMsg $vv;
  }
  playMsg "feet";
}


proc wind_degrees { wind kts } {
  playSilence 200;
  playMsg "wind";

  if {$wind == "100"} {
    playMsg "100";
  } elseif {$wind == "200"} {
    playMsg "200";
  } elseif {$wind == "300"} {
    playMsg "300";
  } else {
    playNr $wind;
  }

  playMsg "degrees";
  playSilence 100;
  playNr $kts;
  playMsg "knots";
  playSilence 50;
}


proc runway {rwy} {
  variable lrc;
  playSilence 200;

  playMsg "runway";
  playNr $rwy;
  
  set lrc [string index rwy end];
  if {$lrc == "l"} {
    playMsg "left";
  }
  if {$lrc == "c"} {
    playMsg "center";
  }
  if {$lrc == "r"} {
    playMsg "right";
  }
}


# executed when no valid METAR
proc metar_not_valid {} {
  playTone 500 500 500;
  playSilence 200;
  playMsg "metar_not_valid";
}


proc report_cfg {nr icao} {
  global basedir;
  playSilence 100;

  playNr $nr;  
  playSilence 100;
  if [file exists "$basedir/MetarInfo/$icao.raw"] {
    playFile "$basedir/MetarInfo/$icao.raw";
  } else {
    spellWord $icao;
  }
  playSilence 100;
}

proc no_airport_defined {} {
  playSilence 200;
  playMsg "no_airport_defined";
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
