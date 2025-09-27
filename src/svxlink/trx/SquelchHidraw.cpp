/**
@file	 SquelchHidraw.cpp
@brief   A squelch detector that read squelch state from a linux/hidraw
         device
@author  Adi Bier / DL1HRC
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

#include <cstring>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SquelchHidraw.h"



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

SquelchHidraw::SquelchHidraw(void)
  : fd(-1), watch(0), active_low(false), pin(0)
{
} /* SquelchHidraw::SquelchHidraw */


SquelchHidraw::~SquelchHidraw(void)
{
  delete watch;
  if (fd >= 0)
  {
    close(fd);
    fd = -1;
  }
} /* SquelchHidraw::~SquelchHidraw */


/**
Initializing the sound card as linux/hidraw device
For further information:
  http://dmkeng.com
  http://www.halicky.sk/om3cph/sb/CM108_DataSheet_v1.6.pdf
  http://www.ti.com/lit/ml/sllu093/sllu093.pdf
  http://www.ti.com/tool/usb-to-gpio
*/
bool SquelchHidraw::initialize(Async::Config& cfg, const std::string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }

  string devicename;
  if (!cfg.getValue(rx_name, "HID_DEVICE", devicename))
  {
    cerr << "*** ERROR: Config variable " << devicename <<
            "/HID_DEVICE not set" << endl;
    return false;
  }

  string sql_pin;
  if (!cfg.getValue(rx_name, "HID_SQL_PIN", sql_pin) || sql_pin.empty())
  {
    cerr << "*** ERROR: Config variable " << rx_name
         << "/HID_SQL_PIN not set or invalid\n";
    return false;
  }

  if ((sql_pin.size() > 1) && (sql_pin[0] == '!'))
  {
    active_low = true;
    sql_pin.erase(0, 1);
  }

  map<string, char> pin_mask;
  pin_mask["VOL_UP"] = 0x01;
  pin_mask["VOL_DN"] = 0x02;
  pin_mask["MUTE_PLAY"] = 0x04;
  pin_mask["MUTE_REC"] = 0x08;

  map<string, char>::iterator it = pin_mask.find(sql_pin);
  if (it == pin_mask.end())
  {
    cerr << "*** ERROR: Invalid value for " << rx_name << "/HID_SQL_PIN="
         << sql_pin << ", must be VOL_UP, VOL_DN, MUTE_PLAY, MUTE_REC" << endl;
    return false;
  }
  pin = (*it).second;

  if ((fd = open(devicename.c_str(), O_RDWR, 0)) < 0)
  {
    cout << "*** ERROR: Could not open event device " << devicename
         << " specified in " << rx_name << "/HID_DEVICE: "
         << strerror(errno) << endl;
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
    cout << "*** ERROR: unknown/unsupported sound chip detected...\n";
    return false;
  }

  watch = new Async::FdWatch(fd, Async::FdWatch::FD_WATCH_RD);
  assert(watch != 0);
  watch->activity.connect(mem_fun(*this, &SquelchHidraw::hidrawActivity));

  return true;
}



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

/**
 * @brief  Called when state of Hidraw port has been changed
 *
 */
void SquelchHidraw::hidrawActivity(FdWatch *watch)
{
  char buf[5];
  int rd = read(fd, buf, sizeof(buf));
  if (rd < 0)
  {
    cerr << "*** ERROR: reading HID_DEVICE\n";
    return;
  }

  bool pin_high = buf[0] & pin;
  setSignalDetected(pin_high != active_low);
} /* SquelchHidraw::hidrawActivity */



/*
 * This file has not been truncated
 */
