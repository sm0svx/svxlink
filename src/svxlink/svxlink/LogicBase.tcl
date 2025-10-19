###############################################################################
#
# LogicBase basic functionality for a logic core
#
###############################################################################

# Source locale handling code
sourceTclWithOverrides "locale.tcl"

# Wrap all variables and functions in a namespace
namespace eval Logic {

# Enable finding commands in the parent namespace from this namespace
namespace path [namespace parent]

# Add the logic_name variable to the Logic namespace to support legacy TCL code
variable logic_name ${::logic_name}


#
# An "overloaded" playMsg that eliminates the need to write "Core" as the first
# argument everywhere.
# For legacy code support, if more than one argument is given to the function
# it will call the original playMsg using all given arguments
#
proc playMsg {args} {
  if {[llength $args] == 1} {
    return [::playMsg Core $args]
  } else {
    return [::playMsg {*}$args]
  }
}


#
# A convenience function for printing out information prefixed by the
# module name
#
#proc printInfo {msg} {
#  puts "${::logic_name}: ${msg}"
#}



# End of namespace Logic
}


#
# This file has not been truncated
#
