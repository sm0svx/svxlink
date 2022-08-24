/**
@file	 SquelchEvDev.cpp
@brief   A squelch detector that read squelch state from /dev/input/eventX.
@author  Tobias Blomberg / SM0SVX
@date	 2011-02-22

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2011 Tobias Blomberg / SM0SVX

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
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <linux/input.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <common.h>
#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SquelchEvDev.h"



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

SquelchEvDev::SquelchEvDev(void)
  : fd(-1), watch(0)
{
  
} /* SquelchEvDev::SquelchEvDev */


SquelchEvDev::~SquelchEvDev(void)
{
  delete watch;
  watch = 0;
  
  if (fd >= 0)
  {
    close(fd);
    fd = -1;
  }
} /* SquelchEvDev::~SquelchEvDev */


bool SquelchEvDev::initialize(Async::Config& cfg, const std::string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }

  std::string devname;
  if (!cfg.getValue(rx_name, "EVDEV_DEVNAME", devname))
  {
    std::cerr << "*** ERROR: Config variable " << rx_name
	      << "/EVDEV_DEVNAME not set\n";
    return false;
  }

  string value;
  if (!cfg.getValue(rx_name, "EVDEV_OPEN", value))
  {
    std::cerr << "*** ERROR: Config variable " << rx_name
	      << "/EVDEV_OPEN not set\n";
    return false;
  }
  SvxLink::splitStr(open_ev, value, ",");
  if (open_ev.size() != 3)
  {
    std::cerr << "*** ERROR: Wrong number of arguments for config variable "
	      << rx_name << "/EVDEV_OPEN. There should be three: "
	      << "type code value (e.g. 1,163,1)\n";
    return false;
  }

  if (!cfg.getValue(rx_name, "EVDEV_CLOSE", value))
  {
    std::cerr << "*** ERROR: Config variable " << rx_name
	      << "/EVDEV_CLOSE not set\n";
    return false;
  }
  SvxLink::splitStr(close_ev, value, ",");
  if (close_ev.size() != 3)
  {
    std::cerr << "*** ERROR: Wrong number of arguments for config variable "
	      << rx_name << "/EVDEV_CLOSE. There should be three: "
	      << "type code value (e.g. 1,163,0)\n";
    return false;
  }

  if ((fd = open(devname.c_str(), O_RDONLY)) == -1)
  {
    cerr << "*** ERROR: Could not open event device " << devname
	 << " specified in " << rx_name << "/EVDEV_DEVNAME: "
         << strerror(errno) << endl;
    return false;
  }
  
  watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
  assert(watch != 0);
  watch->activity.connect(mem_fun(*this, &SquelchEvDev::readEvDevData));
  
    // Print Device Name
  char name[256] = "Unknown";
  if (ioctl(fd, EVIOCGNAME (sizeof (name)), name) != -1)
  {
    cout << rx_name << ": EvDev Squelch: " << devname << "(" << name << ")\n";
  }
  else
  {
    cerr << "*** WARNING: Could not read EvDev squelch device name from "
         << "event device " << devname << " specified in " << rx_name
         << "/EVDEV_DEVNAME: " << strerror(errno) << endl;
  }
 
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

void SquelchEvDev::readEvDevData(FdWatch *w)
{
  struct input_event ev[64];
  int rd = read(fd, ev, sizeof(ev));
  if (rd < 0)
  {
    cerr << "*** WARNING: SquelchEvDev::readEvDevData read for receiver "
         << rxName() << ": " << strerror(errno) << endl;
    return;
  }

  for (int i=0; rd > 0; ++i, rd -= sizeof(ev[0]))
  {
    if (rd < (int)sizeof(ev[0]))
    {
      cerr << "*** WARNING: SquelchEvDev::readEvDevData for receiver "
           << rxName() << ": Short read\n";
      return;
    }
    
    /*
    cout << "type=" << ev[i].type << " code=" << ev[i].code
	<< " value=" << ev[i].value << endl;
    */
    
    int ev_arr_tmp[3] = {ev[i].type, ev[i].code, ev[i].value};
    vector<int> ev_arr(ev_arr_tmp, ev_arr_tmp+3);
    if (!signalDetected() && (open_ev == ev_arr))
    {
      setSignalDetected(true);
    }
    else if (signalDetected() && (close_ev == ev_arr))
    {
      setSignalDetected(false);
    }
  }
} /* SquelchEvDev::readEvDevData */


/*
 * This file has not been truncated
 */

