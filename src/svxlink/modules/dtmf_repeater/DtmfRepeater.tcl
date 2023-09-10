###############################################################################
#
# DtmfRepeater module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleDtmfRepeater] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval DtmfRepeater {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module

# end of namespace
}


#
# This file has not been truncated
#
