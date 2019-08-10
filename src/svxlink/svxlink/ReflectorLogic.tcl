###############################################################################
#
# ReflectorLogic event handlers
#
###############################################################################

#
# This is the namespace in which all functions below will exist. The name
# must match the corresponding section "[ReflectorLogic]" in the configuration
# file. The name may be changed but it must be changed in both places.
#
namespace eval ReflectorLogic {

#
# Checking to see if this is the correct logic core
#
if {$logic_name != [namespace tail [namespace current]]} {
  return;
}


#
# Executed when the selected TG should be reported
#
proc report_tg {tg} {
  playMsg "Core" "talk_group"
  spellNumber $tg
}


#
# Executed when an entered DTMF command failed
#
proc command_failed {cmd} {
  Logic::command_failed $cmd;
}


# end of namespace
}


#
# This file has not been truncated
#
