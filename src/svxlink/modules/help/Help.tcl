###############################################################################
#
# Help module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleHelp] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Help {

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
# Executed to prompt the user to select a module to get help about
#
proc choose_module {module_list} {
  playMsg "choose_module";
  foreach {module_id module_name} "$module_list" {
    playNumber $module_id;
    playSilence 50;
    ::playMsg $module_name "name";
    playSilence 200;
  }
}


#
# Executed when the user selects a non-existing module.
#
proc no_such_module {module_id} {
  playNumber $module_id;
  playMsg "no_such_module";
}



# end of namespace
}

#
# This file has not been truncated
#
