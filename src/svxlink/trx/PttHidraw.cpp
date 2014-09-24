/**
@file	 PttHidraw.cpp
@brief   A PTT hardware controller using the Hidraw Board from DMK
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2014-09-17

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
#include <fcntl.h>
#include <sys/ioctl.h>


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

#include "PttHidraw.h"



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

PttHidraw::PttHidraw(void)
  : active_low(false)
{
} /* PttHidraw::PttHidraw */


PttHidraw::~PttHidraw(void)
{
  close(fd);
  fd = -1;
} /* PttHidraw::~PttHidraw */


bool PttHidraw::initialize(Async::Config &cfg, const std::string name)
{
  if (!cfg.getValue(name, "HID_PIN", Hidraw_pin) || Hidraw_pin.empty())
  {
    cerr << "*** ERROR: Config variable " << name << "/HID_PIN not set\n";
    return false;
  }

  string Hidraw_dev;
  if (!cfg.getValue(name, "HID_DEVICE", Hidraw_dev) || Hidraw_dev.empty())
  {
    cerr << "*** ERROR: Config variable " << name << "/HID_DEVICE not set\n";
    return false;
  }

  if ((fd = open(Hidraw_dev.c_str(), O_WRONLY, 0)) < 0)
  {
    cerr << "*** ERROR: Can't open port " << Hidraw_dev << endl;
    return false;
  }

  if (!ioctl(fd, HIDIOCGRAWINFO, &hiddevinfo) && hiddevinfo.vendor == 0x0d8c)
  {
    cout << "--- Hidraw sound chip is ";
    if (hiddevinfo.product == 0x000c)
    {
      cout << "CM108";
    }
    else if (hiddevinfo.product == 0x013c)
    {
      cout << "CM108A";
    }
    else if (hiddevinfo.product == 0x013a)
    {
      cout << "CM119";
    }
    else
    {
      cout << "unknown";
    }
    cout << endl;
  }
  else
  {
    cout << "*** ERROR: unknown/unsupported sound chip detected...\n";
    return false;
  }

  if (Hidraw_pin[0] == '!')
  {
    active_low = true;
    Hidraw_pin.erase(0, 1);
  }

  return true;
} /* PttHidraw::initialize */


bool PttHidraw::setTxOn(bool tx_on)
{
  //cerr << "### PttHidraw::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

  char a[5] = {'\000', '\000',
              (tx_on ^ active_low ? '\004' : '\000'), '\004', '\000'};

  if (write(fd, a, 5) != -1)
  {
    return true;
  }
  else
  {
    return false;
  }
} /* PttHidraw::setTxOn */



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

