###############################################################################
#
# Help module event handlers
#
###############################################################################

namespace eval Help {

#
# Executed when this module is being activated
#
proc activating_module {} {
  Module::activating_module "Help";
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  Module::deactivating_module "Help";
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  Module::timeout "Help";
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  Module::play_help "Help";
}


#
# Executed to prompt the user to select a module to get help about
#
proc choose_module {module_list} {
  playMsg "Help" "choose_module";
  foreach {module_id module_name} "$module_list" {
    playNumber $module_id;
    playSilence 50;
    playMsg $module_name "name";
    playSilence 200;
  }
}


#
# Executed when the user selects a non-existing module.
#
proc no_such_module {module_id} {
  playNumber $module_id;
  playMsg "Help" "no_such_module";
}



# end of namespace
}

#
# This file has not been truncated
#
