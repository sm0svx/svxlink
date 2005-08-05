###############################################################################
#
# Generic module event handlers
#
###############################################################################

namespace eval Module {

#
# Executed when a module is being activated
#
proc activating_module {module_name} {
  playMsg "Default" "activating_module";
  playSilence 100;
  playMsg $module_name "name";
  playSilence 200;
}


#
# Executed when a module is being deactivated.
#
proc deactivating_module {module_name} {
  playMsg "Default" "deactivating_module";
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
  playMsg $module_name "help";
}


# End of namespace
}

#
# This file has not been truncated
#
