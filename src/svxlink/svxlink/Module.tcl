###############################################################################
#
# Generic module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
#
namespace eval Module {


#
# Executed when a module is being activated
#
proc activating_module {module_name} {
  playMsg "Default" "activating";
  playSilence 100;
  playMsg $module_name "name";
  playSilence 200;
}


#
# Executed when a module is being deactivated.
#
proc deactivating_module {module_name} {
  playMsg "Default" "deactivating";
  playSilence 100;
  playMsg $module_name "name";
  playSilence 200;
}


#
# Executed when the inactivity timeout for a module has expired.
#
proc timeout {module_name} {
  playMsg "Default" "timeout";
  playSilence 100;
}


#
# Executed when playing of the help message for a module has been requested.
#
proc play_help {module_name} {
  playMsg $module_name "help"
  playSubcommands $module_name help_subcmd "sub_commands_are"
}


# End of namespace
}

#
# This file has not been truncated
#
