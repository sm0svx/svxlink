###############################################################################
#
# Template module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleTemplate] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Template {


#
# Executed when this module is being activated
#
proc activating_module {} {
  Module::activating_module "Template";
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  Module::deactivating_module "Template";
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  Module::timeout "Template";
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  Module::play_help "Template";
}


#
# Executed when the state of this module should be reported on the radio
# channel. Typically this is done when a manual identification has been
# triggered by the user by sending a "*".
# This function will only be called if this module is active.
#
proc status_report {} {
  puts "ModuleTemplate: status_report called...";
}


# end of namespace
}


#
# This file has not been truncated
#
