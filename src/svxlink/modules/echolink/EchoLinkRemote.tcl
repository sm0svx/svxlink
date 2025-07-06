###############################################################################
#
# EchoLink module event handlers for remote announcements
#
###############################################################################

# Set up some global variables
set basedir [file dirname [file dirname [info script]]];
set lang [getConfigValue ${::logic_name} DEFAULT_LANG "en_US"]
set lang [getConfigValue ${::module_name} REMOTE_LANG ${::lang}]
set langdir "${::basedir}/sounds/${::lang}"

# Load a base of global functions
source "${::basedir}/events.d/globals.tcl"


#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleEchoLink] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval ${::logic_name}::${::module_name} {

#
# An "overloaded" playMsg that eliminates the need to write the module name
# as the first argument.
# For legacy code support, if more than one argument is given to the function
# it will call the original playMsg using all given arguments
#
proc playMsg {args} {
  if {[llength $args] == 1} {
    ::playMsg ${::module_type} $args
  } else {
    ::playMsg {*}$args
  }
}


#
# A convenience function for printing out information prefixed by the
# module name
#
#   msg - The message to print
#
proc printInfo {msg} {
  puts "${::module_name}: $msg";
}


#
# Executed when an incoming connection is accepted
#
#   call - The callsign of the remote node
#
proc remote_greeting {call} {
  playSilence 1000
  playMsg "greeting"
}


#
# Executed when an incoming connection is rejected
#
#   perm - Set to non-zero if the rejection is permanent
#
proc reject_remote_connection {perm} {
  playSilence 1000
  if {$perm} {
    playMsg "reject_connection"
  } else {
    playMsg "reject_connection"
    playMsg "please_try_again_later"
  }
  playSilence 1000
}


#
# Executed when the inactivity timer times out
#
proc remote_timeout {} {
  playMsg "timeout"
  playSilence 1000
}


#
# Executed when the squelch state changes
#
#   is_open - Set to non-zero if the squelch is open
#
proc squelch_open {is_open} {
  # The listen_only_active variable is set by the C++ code
  variable listen_only_active

  set remote_rgr_sound [getConfigValue ${::module_name} "REMOTE_RGR_SOUND" 0]
  if {$remote_rgr_sound && !$is_open && !$listen_only_active} {
    playSilence 200
    playTone 1000 100 100
  }
}


# end of namespace
}

#
# This file has not been truncated
#
