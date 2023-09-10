###############################################################################
#
# PropagationMonitor module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTcl] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval PropagationMonitor {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


#
# Called when an illegal command has been entered
#
#   cmd - The received command
#
proc unknown_command {cmd} {
  playNumber $cmd
  playMsg "unknown_command"
}


#
# Play an alert sound to get the users attention
#
proc playAlertSound {} {
  for {set i 0} {$i < 3} {set i [expr $i + 1]} {
    playTone 440 500 100
    playTone 880 500 100
    playTone 440 500 100
    playTone 880 500 100
    playSilence 600
  }
  playSilence 1000
}


#
# Say the specified band name (e.g. two meters, seventy centimeters etc)
#
#   band - The band (e.g. 2m, 70cm etc)
#
proc sayBand {band} {
  if [regexp {^(\d+)(c?m)$} $band -> number unit] {
    if {[string length $number] == 2} {
      playTwoDigitNumber $number
    } else {
      playNumber $number
    }
    playMsg unit_$unit
  }
}


#
# Say the specified locator
#
#   loc - The locator (e.g. JP79 or JP79XI)
#
proc sayLocator {loc} {
  if [regexp {^(\w\w)(?:(\d\d)(\w\w)?)?$} $loc -> part1 part2 part3] {
    spellWord $part1
    playNumber $part2
    #spellWord $part3
  }
}


#
# Handle aurora alert from Good DX.
#
#   hour - The hour of the alert
#   min  - The minute of the alert
#
proc dxrobot_aurora_alert {hour min} {
  playAlertSound
  playSilence 500
  
  #playTime $hour $min
  #playSilence 200
  
  playMsg "aurora_opening"
  sayBand "2m"
  playSilence 500
  
  playMsg "aurora_opening"
  sayBand "2m"
  playSilence 200
}


#
# Handle E-skip alert from Good DX.
#
#   hour - The hour of the alert
#   min  - The minute of the alert
#   band - The band it occured on. E.g 2m, 4m, 6m.
#   from - From locator
#   to   - To locator
#
proc dxrobot_eskip_alert {hour min band {from ""} {to ""}} {
  playAlertSound
  playSilence 500
  
  #playTime $hour $min
  #playSilence 200
  
  playMsg "sporadic_e_opening"
  sayBand $band
  playSilence 500
  playMsg "sporadic_e_opening"
  sayBand $band
  
  if {"$from" ne "" && "$to" ne ""} {
    playSilence 500
    playMsg "band_open_from"
    spellWord $from
    playSilence 100
    playMsg "to"
    playSilence 100
    spellWord $to
  }
  playSilence 200
}


#
# Handle sporadic E alert from VHFDX.
#
#   band    - The band the alert is for (e.g. 2m, 70cm etc)
#   muf     - Maximum usable frequency
#   locator - The locator the alert applies to
proc vhfdx_sporadic_e_opening {band muf locator} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg sporadic_e_opening
    sayBand $band
    playSilence 500
    playMsg MUF
    playSilence 100
    playNumber $muf
    playMsg unit_MHz
    playSilence 200
    playMsg above
    playSilence 200
    sayLocator $locator
    playSilence 1000
  }
}


#
# Handle possible sporadic E alert from VHFDX.
#
#   band      - The band the alert is for (e.g. 2m, 70cm etc)
#   from_loc  - From locator
#   to_loc    - To locator
#   direction - Beam direction
#
proc vhfdx_possible_sporadic_e_opening {band from_loc to_loc direction} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg possible
    playMsg sporadic_e_opening
    sayBand $band
    playSilence 100
    playMsg between
    playSilence 200
    sayLocator $from_loc
    playSilence 200
    playMsg and
    playSilence 200
    sayLocator $to_loc
    playSilence 1000
  }
}


#
# Handle multi-hop sporadic-E opening alert from VHFDX.
#
#   band - The band the alert is for (e.g. 2m, 70cm etc)
#
proc vhfdx_multi_hop_sporadic_e_opening {band} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg multi_hop
    playMsg sporadic_e_opening
    sayBand $band
    playSilence 1000
  }
}


#
# Handle tropo opening alert from VHFDX.
#
#   band  - The band the alert is for (e.g. 2m, 70cm etc)
#   range - Range, in kilometers, from... what?
#   call1 - Call of first station in reported QSO
#   loc1  - Locator of first station
#   call2 - Call of second station in reported QSO
#   loc2  - Locator of second station
#
proc vhfdx_tropo_opening {band range call1 loc1 call2 loc2} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg tropo_opening
    sayBand $band
    playSilence 200
    playMsg between
    sayLocator $loc1
    playSilence 200
    playMsg and
    playSilence 200
    sayLocator $loc2
    playSilence 1000
  }
}


#
# Handle aurora opening alert from VHFDX.
#
#   band - The band the alert is for (e.g. 2m, 70cm etc)
#   lat  - Lowest latitude where aurora is active
#   call - The call of the reporting station
#   loc  - The locator of the reporting station
#
proc vhfdx_aurora_opening {band lat call loc} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg aurora_opening
    sayBand $band
    playSilence 200
    playMsg down_to_lat
    playNumber $lat
    playMsg unit_deg
    playSilence 1000
  }
}


#
# Handle FAI activity
#
# band - The band the alert is for (e.g. 2m, 70cm etc)
#
proc vhfdx_fai_active {band} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg "fai_active_on"
    playSilence 200
    sayBand $band
    playSilence 1000
  }
}

#
# Handle TEP opening
#
# band - The band the alert is for (e.g. 2m, 70cm etc)
#
proc vhfdx_tep_opening {band} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg "tep"
    playMsg "opening_on"
    playSilence 200
    sayBand $band
    playSilence 1000
  }
}

#
# Handle F2 opening
#
# band - The band the alert is for (e.g. 2m, 70cm etc)
#
proc vhfdx_f2_opening {band} {
  playAlertSound
  for {set i 0} {$i < 2} {set i [expr $i + 1]} {
    playMsg "f2"
    playMsg "opening_on"
    playSilence 200
    sayBand $band
    playSilence 1000
  }
}


# end of namespace
}


#
# This file has not been truncated
#
