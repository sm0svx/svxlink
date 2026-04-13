/**
@file	 SquelchHidraw.cpp
@brief   A squelch detector that read squelch state from a linux/hidraw
         device
@author  Adi Bier / DL1HRC
@date	 2014-09-17

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

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

#include <unistd.h>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <sys/ioctl.h>

#include <cstring>
#include <cerrno>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
#include <common.h>


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
{
  m_reopen_timer.expired.connect(sigc::hide_return(sigc::hide(
      sigc::mem_fun(*this, &SquelchHidraw::openDevice)
      )));
  m_watch.activity.connect(sigc::hide(
      sigc::mem_fun(*this, &SquelchHidraw::hidrawActivity)
      ));
} /* SquelchHidraw::SquelchHidraw */


SquelchHidraw::~SquelchHidraw(void)
{
  closeDevice();
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
  m_cfg = &cfg;
  m_rx_name = rx_name;

  if (!Squelch::initialize(cfg, m_rx_name))
  {
    return false;
  }

  return openDevice();
} /* SquelchHidraw::initialize */


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

bool SquelchHidraw::openDevice(void)
{
  closeDevice();

  string devicename;
  if (!m_cfg->getValue(m_rx_name, "HID_DEVICE", devicename))
  {
    std::cerr << "*** ERROR: Config variable " << m_rx_name
              << "/HID_DEVICE not set"
              << std::endl;
    return false;
  }

  string sql_pin;
  if (!m_cfg->getValue(m_rx_name, "HID_SQL_PIN", sql_pin) || sql_pin.empty())
  {
    std::cerr << "*** ERROR: Config variable " << m_rx_name
              << "/HID_SQL_PIN not set or invalid"
              << std::endl;
    return false;
  }

  if ((sql_pin.size() > 1) && (sql_pin[0] == '!'))
  {
    m_active_low = true;
    sql_pin.erase(0, 1);
  }

  static const std::map<std::string, char> pin_mask{
    {"VOL_UP", 0x01},
    {"VOL_DN", 0x02},
    {"MUTE_PLAY", 0x04},
    {"MUTE_REC", 0x08}
  };

  auto it = pin_mask.find(sql_pin);
  if (it == pin_mask.end())
  {
    cerr << "*** ERROR: Invalid value for " << m_rx_name << "/HID_SQL_PIN="
         << sql_pin << ", must be VOL_UP, VOL_DN, MUTE_PLAY, MUTE_REC" << endl;
    return false;
  }
  m_pin = (*it).second;

  if ((m_fd = ::open(devicename.c_str(), O_RDWR, 0)) < 0)
  {
    std::cout << "*** ERROR: Could not open event device " << devicename
              << " specified in " << m_rx_name << "/HID_DEVICE: "
              << SvxLink::strError(errno) << std::endl;
    closeDevice();
    return false;
  }

  struct hidraw_devinfo hiddevinfo;
  if ((::ioctl(m_fd, HIDIOCGRAWINFO, &hiddevinfo) != -1) &&
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
    std::cerr << "*** ERROR: Unknown/unsupported sound chip detected..."
              << std::endl;
    closeDevice();
    return false;
  }

  m_watch.setFd(m_fd, Async::FdWatch::FD_WATCH_RD);
  m_watch.setEnabled(true);
  m_reopen_timer.setEnable(false);

  return true;
} /* SquelchHidraw::openDevice */


void SquelchHidraw::closeDevice(void)
{
  m_reopen_timer.setEnable(false);
  m_watch.setEnabled(false);

  m_active_low = false;
  m_pin = 0;

  if (m_fd >= 0)
  {
    ::close(m_fd);
    m_fd = -1;
  }
} /* SquelchHidraw::closeDevice */


/**
 * @brief  Called when state of Hidraw port has been changed
 */
void SquelchHidraw::hidrawActivity(void)
{
  char buf[5];
  int rd = ::read(m_fd, buf, sizeof(buf));
  if (rd < 0)
  {
    std::cerr << "*** ERROR: Failed to read HID_DEVICE: "
              << SvxLink::strError(errno)
              << std::endl;
    closeDevice();
    setSignalDetected(false);
    m_reopen_timer.setEnable(true);
    return;
  }

  bool pin_high = buf[0] & m_pin;
  setSignalDetected(pin_high != m_active_low);
} /* SquelchHidraw::hidrawActivity */


/*
 * This file has not been truncated
 */
