/**
@file	 Modulation.cpp
@brief   Transceiver modulation representation
@author  Tobias Blomberg / SM0SVX
@date	 2018-01-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Modulation.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public functions
 *
 ****************************************************************************/

Modulation::Type Modulation::fromString(const std::string& modstr)
{
  static std::map<std::string,Type> modmap;
  if (modmap.empty())
  {
    modmap["FM"] = MOD_FM;
    modmap["NBFM"] = MOD_NBFM;
    modmap["WBFM"] = MOD_WBFM;
    modmap["AM"] = MOD_AM;
    modmap["NBAM"] = MOD_NBAM;
    modmap["USB"] = MOD_USB;
    modmap["LSB"] = MOD_LSB;
    modmap["CW"] = MOD_CW;
    modmap["WBCW"] = MOD_WBCW;
  }
  std::map<std::string,Type>::const_iterator it;
  it = modmap.find(modstr);
  if (it == modmap.end())
  {
    return MOD_UNKNOWN;
  }
  return (*it).second;
}


const char *Modulation::toString(Modulation::Type mod)
{
  switch (mod)
  {
    case MOD_FM:
      return "FM";
    case MOD_NBFM:
      return "NBFM";
    case MOD_WBFM:
      return "WBFM";
    case MOD_AM:
      return "AM";
    case MOD_NBAM:
      return "NBAM";
    case MOD_USB:
      return "USB";
    case MOD_LSB:
      return "LSB";
    case MOD_CW:
      return "CW";
    case MOD_WBCW:
      return "WBCW";
    case MOD_UNKNOWN:
      break;
  }
  return "UNKNOWN";
}


/*
 * This file has not been truncated
 */

