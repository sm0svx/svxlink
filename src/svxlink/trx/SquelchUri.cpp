/**
@file	 SquelchUri.cpp
@brief   A squelch detector that read squelch state from a Uri device
@author  Adi Bier / DL1HRC
@date	 2014-08-28

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
#include <sys/types.h>
#include <sys/stat.h>
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

#include "SquelchUri.h"



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

SquelchUri::SquelchUri(void)
  : fd(-1), active_low(false)
{
  
} /* SquelchUri::SquelchUri */


SquelchUri::~SquelchUri(void)
{
  if (fd >= 0)
  {
    close(fd);
    fd = -1;
  }
} /* SquelchUri::~SquelchUri */


bool SquelchUri::initialize(Async::Config& cfg, const std::string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }

  string devicename;
  if (!cfg.getValue(rx_name, "URI_DEVICE", devicename))
  {
    cerr << "*** ERROR: Config variable " << devicename <<
            "/URI_NR not set" << endl;
    return false;
  }

  string sql_pin;
  if (!cfg.getValue(rx_name, "URI_SQL_PIN", sql_pin) || sql_pin.empty())
  {
    cerr << "*** ERROR: Config variable " << rx_name <<
            "/URI_SQL_PIN not set or invalid\n";
    return false;
  }

  if ((sql_pin.size() > 1) && (sql_pin[0] == '!'))
  {
    active_low = true;
    sql_pin.erase(0, 1);
  }

  if ((fd = open(devicename.c_str(), O_RDWR)) == -1)
  {
    cout << "*** ERROR: Could not open event device " << devicename
         << " specified in " << rx_name << "/URI_DEVICE: " 
         << strerror(errno) << endl;
    return false;
  }

  if (!ioctl(fd, HIDIOCGRAWINFO, &hiddevinfo) && hiddevinfo.vendor == 0x0d8c)
  {
    cout << "--- URI sound chip is ";
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

  watch = new Async::FdWatch(fd, Async::FdWatch::FD_WATCH_RD);
  assert(watch != 0);
  watch->activity.connect(mem_fun(*this, &SquelchUri::uriActivity));

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
 * @brief  Called by a timer to periodically read the state of the Uri pin
 *
 */
void SquelchUri::uriActivity(FdWatch *watch)
{
  char buf[25];
  int rd = read(fd, buf, 5);

  if (rd < 0) 
  {
    cerr << "*** ERROR: reading device\n";
  }
  else
  {
    cout << "read() read " << rd << "bytes\n";
    for (int i=0; i < 5; i++)
    {
      printf("%hhx,", buf[i]);
    }
    cout << endl;
  }
/*
  if (!signalDetected() && (value == 1))
  {
    setSignalDetected(active_low ^ true);
  }
  else if (signalDetected() && (value == 0))
  {
    setSignalDetected(active_low ^ false);
  }*/
} /* SquelchUri::readUriValueData */



/*
 * This file has not been truncated
 */
