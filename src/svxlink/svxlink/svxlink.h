#ifndef SVXLINK_INCLUDED
#define SVXLINK_INCLUDED

#include <vector>

#include "SimplexLogic.h"
#include "RepeaterLogic.h"
#include "AnalogPhoneLogic.h"

std::vector< SimplexLogic * > get_simplex_logics();
std::vector< RepeaterLogic * > get_repeater_logics();
std::vector< AnalogPhoneLogic * > get_analogphone_logics();

#endif
