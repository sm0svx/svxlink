/**
@file	 PttGpio.cpp
@brief   A PTT hardware controller using a pin in a GPIO port
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
#include <fstream>
#include <cstring>
#include <sstream>


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

#include "PttGpio.h"



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

PttGpio::PttGpio(void)
  : gpio_path("/sys/class/gpio"), active_low(false)
{
} /* PttGpio::PttGpio */


PttGpio::~PttGpio(void)
{
} /* PttGpio::~PttGpio */


bool PttGpio::initialize(Async::Config &cfg, const std::string name)
{
  cfg.getValue(name, "GPIO_PATH", gpio_path);

  if (!cfg.getValue(name, "PTT_PIN", gpio_pin) || gpio_pin.empty())
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PIN not set\n";
    return false;
  }

  if (gpio_pin[0] == '!')
  {
    active_low = true;
    gpio_pin.erase(0, 1);
  }

  stringstream ss;
  ss << gpio_path << "/" << gpio_pin << "/value";
  ofstream gpioval(ss.str().c_str());
  if (gpioval.fail())
  {
    cerr << "*** ERROR: Could not open GPIO " << ss.str()
         << " for writing in transmitter " << name << ".\n";
    return false;
  }
  gpioval.close();

  return true;
} /* PttGpio::initialize */


bool PttGpio::setTxOn(bool tx_on)
{
  //cerr << "### PttGpio::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

  stringstream ss;
  ss << gpio_path << "/" << gpio_pin << "/value";
  ofstream gpioval(ss.str().c_str());
  if (gpioval.fail())
  {
    return false;
  }
  gpioval << (tx_on ^ active_low ? 1 : 0);
  gpioval.close();

  return true;
} /* PttGpio::setTxOn */



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



/*
 * This file has not been truncated
 */

