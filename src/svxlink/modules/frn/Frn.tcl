###############################################################################
#
# Frn module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleFrn] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Frn {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


#
# Executed when an entered command failed or have bad syntax.
#
proc command_failed {cmd} {
  spellWord $cmd;
  playMsg "operation_failed";
}


#
# Executed when an unrecognized command has been received.
#
proc unknown_command {cmd} {
  spellWord $cmd;
  playMsg "unknown_command";
}


#
# Executed when command to count nodes on the channel is called
#
proc count_clients {count_clients} {
  playNumber $count_clients;
  playSilence 50;
  playMsg "connected_clients";
  playSilence 250;
}


#
# Executed when the rf disable feature is activated or deactivated
#   status    - The current status of the feature (0=deactivated, 1=activated)
#   activate  - The requested new status of the feature
#               (0=deactivate, 1=activate)
#
proc rf_disable {status activate} {
  variable module_name;
  puts "$module_name: [expr {$activate ? "Activating" : "Deactivating"}]\
        listen only mode.";
  playMsg [expr {$activate ? "activating" : "deactivating"}];
  playMsg "rf_disable";
}

# end of namespace
}


#
# This file has not been truncated
#
