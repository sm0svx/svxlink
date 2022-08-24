/**
@file	 PttSerialPin.cpp
@brief   A PTT hardware controller using a pin in a serial port
@author  Tobias Blomberg / SM0SVX
@date	 2014-01-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#include <iostream>
#include <cstring>


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

#include "PttSerialPin.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



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
 * Public member functions
 *
 ****************************************************************************/

PttSerialPin::PttSerialPin(void)
  : serial(0), ptt_pin1(Serial::PIN_NONE), ptt_pin1_rev(false),
    ptt_pin2(Serial::PIN_NONE), ptt_pin2_rev(false)
{
} /* PttSerialPin::PttSerialPin */


PttSerialPin::~PttSerialPin(void)
{
  delete serial;
} /* PttSerialPin::~PttSerialPin */


bool PttSerialPin::initialize(Async::Config &cfg, const std::string name)
{
  string ptt_port;
  if (!cfg.getValue(name, "PTT_PORT", ptt_port))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PORT not set\n";
    return false;
  }
  serial = new Serial(ptt_port.c_str());
  if (!serial->open())
  {
    perror("open serial port");
    return false;
  }
  if (!setPins(cfg, name))
  {
    return false;
  }

  string ptt_pin_str;
  if (!cfg.getValue(name, "PTT_PIN", ptt_pin_str))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PIN not set\n";
    return false;
  }
  const char *ptr = ptt_pin_str.c_str();
  int cnt;
  cnt = parsePttPin(name, ptr, ptt_pin1, ptt_pin1_rev);
  if (cnt == 0)
  {
    return false;
  }
  ptr += cnt;
  if (*ptr != 0)
  {
    if (parsePttPin(name, ptr, ptt_pin2, ptt_pin2_rev) == 0)
    {
      return false;
    }
  }

  return true;
} /* PttSerialPin::initialize */


bool PttSerialPin::setTxOn(bool tx_on)
{
  //cerr << "### PttSerialPin::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

  if (!serial->setPin(ptt_pin1, tx_on ^ ptt_pin1_rev))
  {
    return false;
  }

  if (!serial->setPin(ptt_pin2, tx_on ^ ptt_pin2_rev))
  {
    return false;
  }

  return true;
} /* PttSerialPin::setTxOn */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

int PttSerialPin::parsePttPin(const string &name, const char *str,
                              Serial::Pin &pin, bool &rev)
{
  int cnt = 0;
  if (*str == '!')
  {
    rev = true;
    str++;
    cnt++;
  }
  if (strncmp(str, "RTS", 3) == 0)
  {
    pin = Serial::PIN_RTS;
    str += 3;
    cnt += 3;
  }
  else if (strncmp(str, "DTR", 3) == 0)
  {
    pin = Serial::PIN_DTR;
    str += 3;
    cnt += 3;
  }
  else
  {
    cerr << "*** ERROR: Accepted values for config variable "
      	 << name << "/PTT_PIN are \"[!]RTS\" and/or \"[!]DTR\".\n";
    return 0;
  }

  return cnt;

} /* PttSerialPin::parsePttPin */


bool PttSerialPin::setPins(const Async::Config &cfg, const std::string &name)
{
  std::string pins;
  if (!cfg.getValue(name, "SERIAL_SET_PINS", pins))
  {
    return true;
  }
  std::string::iterator it(pins.begin());
  while (it != pins.end())
  {
    bool do_set = true;
    if (*it == '!')
    {
      do_set = false;
      ++it;
    }
    std::string pin_name(it, it+3);
    it += 3;
    if (pin_name == "RTS")
    {
      serial->setPin(Async::Serial::PIN_RTS, do_set);
    }
    else if (pin_name == "DTR")
    {
      serial->setPin(Async::Serial::PIN_DTR, do_set);
    }
    else
    {
      std::cerr << "*** ERROR: Illegal pin name \"" << pin_name << "\" for the "
                << name << "/SERIAL_SET_PINS configuration variable. "
                << "Accepted values are \"[!]RTS\" and/or \"[!]DTR\".\n";
      return false;
    }
  }
  return true;
} /* PttSerialPin::setPins */




/*
 * This file has not been truncated
 */

