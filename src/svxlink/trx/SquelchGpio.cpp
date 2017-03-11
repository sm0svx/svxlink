/**
@file	 SquelchGpio.cpp
@brief   A squelch detector that read squelch state from a GPIO port pin
@author  Jonny Roeker / DG9OAA
@date	 2013-09-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SquelchGpio.h"



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

SquelchGpio::SquelchGpio(void)
  : fd(-1), timer(0), active_low(false), gpio_path("/sys/class/gpio")
{
  
} /* SquelchGpio::SquelchGpio */


SquelchGpio::~SquelchGpio(void)
{
  delete timer;
  timer = 0;
  if (fd >= 0)
  {
    close(fd);
    fd = -1;
  }
} /* SquelchGpio::~SquelchGpio */


bool SquelchGpio::initialize(Async::Config& cfg, const std::string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }

  cfg.getValue(rx_name, "GPIO_PATH", gpio_path);

  string sql_pin;
  if (!cfg.getValue(rx_name, "GPIO_SQL_PIN", sql_pin) || sql_pin.empty())
  {
    cerr << "*** ERROR: Config variable " << rx_name <<
            "/GPIO_SQL_PIN not set or invalid\n";
    return false;
  }

  if ((sql_pin.size() > 1) && (sql_pin[0] == '!'))
  {
    active_low = true;
    sql_pin.erase(0, 1);
  }

  stringstream ss;
  ss << gpio_path << "/" << sql_pin << "/value";
  fd = open(ss.str().c_str(), O_RDONLY);
  if (fd < 0)
  {
    cerr << "*** ERROR: Could not open GPIO device " << ss.str()
         << " specified in " << rx_name << "/GPIO_SQL_PIN: "
         << strerror(errno) << endl;
    return false;
  }

  timer = new Timer(100, Timer::TYPE_PERIODIC);
  timer->expired.connect(
      hide(mem_fun(*this, &SquelchGpio::readGpioValueData)));

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
 * @brief  Called by a timer to periodically read the state of the GPIO pin
 *
 * An example of reading a GPIO ports can be found at:
 * http://elinux.org/RPi_Low-level_peripherals#C_.2B_sysfs
 * Note though that this example code is not 100% safe and not optimal. The
 * implementation below is better.
 * The file position must be reset before reading since we are keeping the
 * file descriptor open. The first byte in the stream will contain ASCII
 * '0' or '1' depending on the state of the GPIO pin. 0=GND, 1=3.3V. If the
 * next char is read it will be a newline character but we will ignore that.
 */
void SquelchGpio::readGpioValueData(void)
{
  char value = '?';
  if (lseek(fd, 0, SEEK_SET) == -1)
  {
    cerr << "*** WARNING: SquelchGpio::readGpioValueData: lseek failed: "
         << strerror(errno) << endl;
    return;
  }
  ssize_t cnt = read(fd, &value, 1);
  if (cnt == -1)
  {
    cerr << "*** WARNING: SquelchGpio::readGpioValueData: read failed"
         << strerror(errno) << endl;
    return;
  }
  else if (cnt != 1)
  {
    cerr << "*** WARNING: SquelchGpio::readGpioValueData: read returned "
         << cnt << " bytes instead of 1: " << strerror(errno) << endl;
    return;
  }

  bool is_active = active_low ^ (value == '1');
  if (signalDetected() != is_active)
  {
    setSignalDetected(is_active);
  }
} /* SquelchGpio::readGpioValueData */



/*
 * This file has not been truncated
 */
