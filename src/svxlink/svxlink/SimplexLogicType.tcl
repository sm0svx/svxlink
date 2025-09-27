###############################################################################
#
# SimplexLogic event handlers
#
# This file is sourced by all simplex logics and contains the TCL code that are
# in common to all simplex logics.
#
###############################################################################

namespace eval ${::logic_name} {

# Mix in ("inherit") generic logic TCL code
sourceTclWithOverrides "Logic.tcl"
mixin Logic

# end of namespace
}

#
# This file has not been truncated
#
