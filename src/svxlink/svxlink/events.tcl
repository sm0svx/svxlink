###############################################################################
#
# This is the main file for the SvxLink TCL script event handling subsystem.
# It loads the event handling scripts and provides some basic functions for
# playing sounds to. The event handling functions are read from the following
# subdirectories:
#
#   events.d        - Main event script directory
#   events.d/local  - Local modifications to event handling scripts
#
# The same structure is also available, if needed, in the sound clip
# directories for each language.
#
###############################################################################

puts "${::logic_name}: Loading [file normalize [info script]]"


#
# Enable features to support legacy code
#
proc enableLegacySupport {} {
  namespace eval Logic {
    # Access to variables in the ::Logic namespace
    set ns ::${::logic_name}::Logic
    foreach var [namespace eval ${ns} info vars ${ns}::*] {
      set varname [namespace tail ${var}]
      namespace upvar ${ns} ${varname} ${varname}
    }
  }
}


###############################################################################
#
# Main program
#
###############################################################################

# Set up some global variables
set basedir [file dirname [info script]];
set lang [getConfigValue $logic_name DEFAULT_LANG "en_US"]
set langdir "$basedir/sounds/$lang"

# Source globals
source "${::basedir}/events.d/globals.tcl"

# Source TCL code for the logic core
sourceTclWithOverrides "${::logic_type}LogicType.tcl"
sourceTclOverrides "${::logic_name}.tcl"

# Enable support for legacy TCL code
enableLegacySupport

printInfo "Event handler script successfully loaded.";

#printNamespaceTree

#
# This file has not been truncated
#
