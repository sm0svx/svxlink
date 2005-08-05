###############################################################################
#
# Parrot module event handlers
#
###############################################################################

namespace eval Parrot {

#
# Executed when this module is being activated
#
proc activating_module {} {
  Module::activating_module "Parrot";
}


#
# Executed when this module is being deactivated.
#
proc deactivating_module {} {
  Module::deactivating_module "Parrot";
}


#
# Executed when the inactivity timeout for this module has expired.
#
proc timeout {} {
  Module::timeout "Parrot";
}


#
# Executed when playing of the help message for this module has been requested.
#
proc play_help {} {
  Module::play_help "Parrot";
}


#
# Executed when the user has entered some digits that he want the node to
# read back to him.
#
proc spell_digits {digits} {
  spellWord $digits;
  playSilence 500;
}


# end of namespace
}

#
# This file has not been truncated
#
