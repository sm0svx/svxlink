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

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


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
