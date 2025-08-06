###############################################################################
#
# Parrot module event handlers
#
###############################################################################

#
# This is the namespace in which all functions and variables below will exist.
# The name must match the configuration variable "NAME" in the
# [ModuleParrot] section in the configuration file. The name may be changed
# but it must be changed in both places.
#
namespace eval Parrot {

# Load Module core handlers
sourceTclWithOverrides "Module.tcl"
mixin Module


#
# Executed when the user has entered some digits that he want the node to
# read back to him.
#
proc spell_digits {digits} {
  spellWord $digits;
  playSilence 500;
}


#
# Executed when all recorded audio has been played back
#
proc all_played {} {
  #playSilence 500
  #playTone 440 500 100
  #playSilence 100
}


# end of namespace
}

#
# This file has not been truncated
#
