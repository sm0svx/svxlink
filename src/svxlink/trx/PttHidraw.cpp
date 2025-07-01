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
#include <unistd.h>
#include <fcntl.h>
#include <linux/hidraw.h>
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
  : active_low(false), fd(-1), pin(0)
{
} /* PttHidraw::PttHidraw */


PttHidraw::~PttHidraw(void)
{
  if (fd >= 0)
  {
    close(fd);
    fd = -1;
  }
} /* PttHidraw::~PttHidraw */


bool PttHidraw::initialize(Async::Config &cfg, const std::string name)
{

  map<string, char> pin_mask;
  pin_mask["GPIO1"] = 0x01;
  pin_mask["GPIO2"] = 0x02;
  pin_mask["GPIO3"] = 0x04;
  pin_mask["GPIO4"] = 0x08;

  string hidraw_pin;
  if (!cfg.getValue(name, "HID_PTT_PIN", hidraw_pin) || hidraw_pin.empty())
  {
    cerr << "*** ERROR: Config variable " << name << "/HID_PTT_PIN not set\n";
    return false;
  }

  string hidraw_dev;
  if (!cfg.getValue(name, "HID_DEVICE", hidraw_dev) || hidraw_dev.empty())
  {
    cerr << "*** ERROR: Config variable " << name << "/HID_DEVICE not set\n";
    return false;
  }

  if ((fd = open(hidraw_dev.c_str(), O_WRONLY, 0)) < 0)
  {
    cerr << "*** ERROR: Can't open port " << hidraw_dev << endl;
    return false;
  }

  struct hidraw_devinfo hiddevinfo;
  if ((ioctl(fd, HIDIOCGRAWINFO, &hiddevinfo) != -1) &&
      (hiddevinfo.vendor == 0x0d8c))
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
    else if (hiddevinfo.product == 0x0012)
    {
      cout << "CM108B";
    }
    else if (hiddevinfo.product == 0x000e)
    {
      cout << "CM109";
    }
    else if (hiddevinfo.product == 0x013a)
    {
      cout << "CM119";
    }
    else if (hiddevinfo.product == 0x0013)
    {
      cout << "CM119A";
    }
    else
    {
      cout << "unknown";
    }
    cout << endl;
  }
  else
  {
    cerr << "*** ERROR: unknown/unsupported sound chip detected...\n";
    return false;
  }

  if (hidraw_pin[0] == '!')
  {
    active_low = true;
    hidraw_pin.erase(0, 1);
  }

  map<string, char>::iterator it = pin_mask.find(hidraw_pin);
  if (it == pin_mask.end())
  {
    cerr << "*** ERROR: Wrong value for " << name << "/HID_PIN=" << hidraw_pin
         << ", valid are GPIO1, GPIO2, GPIO3, GPIO4" << endl;
    return false;
  }
  pin = (*it).second;

  return true;
} /* PttHidraw::initialize */


bool PttHidraw::setTxOn(bool tx_on)
{
  //cerr << "### PttHidraw::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

  char a[5] = {'\000', '\000',
              (tx_on ^ active_low ? pin : '\000'), pin, '\000'};

  if (write(fd, a, sizeof(a)) == -1)
  {
    return false;
  }

  return true;
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

