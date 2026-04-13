/**
@file	 PttHidraw.cpp
@brief   A PTT hardware controller using the Hidraw Board from DMK
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
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

#include <common.h>


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
{
} /* PttHidraw::PttHidraw */


PttHidraw::~PttHidraw(void)
{
  closeDevice();
} /* PttHidraw::~PttHidraw */


bool PttHidraw::initialize(Async::Config& cfg, const std::string name)
{
  m_cfg = &cfg;
  m_tx_name = name;

  return openDevice();
} /* PttHidraw::initialize */



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

bool PttHidraw::openDevice(void)
{
  static const std::map<string, char> pin_mask{
    {"GPIO1", 0x01},
    {"GPIO2", 0x02},
    {"GPIO3", 0x04},
    {"GPIO4", 0x08}
  };

  std::string hidraw_dev;
  if (!m_cfg->getValue(m_tx_name, "HID_DEVICE", hidraw_dev) ||
      hidraw_dev.empty())
  {
    std::cerr << "*** ERROR: Config variable " << m_tx_name
              << "/HID_DEVICE not set"
              << std::endl;
    return false;
  }

  std::string hidraw_pin;
  if (!m_cfg->getValue(m_tx_name, "HID_PTT_PIN", hidraw_pin) ||
      hidraw_pin.empty())
  {
    std::cerr << "*** ERROR: Config variable " << m_tx_name
              << "/HID_PTT_PIN not set"
              << std::endl;
    return false;
  }

  auto it = pin_mask.find(hidraw_pin);
  if (it == pin_mask.end())
  {
    std::cerr << "*** ERROR: Wrong value for " << m_tx_name << "/HID_PTT_PIN="
              << hidraw_pin << ", valid values are GPIO1, GPIO2, GPIO3, GPIO4"
              << std::endl;
    return false;
  }
  m_pin = (*it).second;

  if ((m_fd = ::open(hidraw_dev.c_str(), O_WRONLY, 0)) < 0)
  {
    std::cerr << "*** ERROR: Can't open HIDRAW device '" << hidraw_dev
              << "': " << SvxLink::strError(errno)
              << std::endl;
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
    cerr << "*** ERROR: unknown/unsupported sound chip detected...\n";
    closeDevice();
    return false;
  }

  if (hidraw_pin[0] == '!')
  {
    m_active_low = true;
    hidraw_pin.erase(0, 1);
  }

  return true;
} /* PttHidraw::openDevice */


void PttHidraw::closeDevice(void)
{
  m_active_low = false;
  m_pin = 0;

  if (m_fd >= 0)
  {
    ::close(m_fd);
    m_fd = -1;
  }
} /* PttHidraw::closeDevice */


bool PttHidraw::setTxOn(bool tx_on)
{
  //cerr << "### PttHidraw::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

  const char a[5] = {
    '\000',
    '\000',
    (tx_on ^ m_active_low ? m_pin : '\000'),
    m_pin,
    '\000'
  };

  if ((m_fd < 0) && !openDevice())
  {
    return false;
  }

  if (::write(m_fd, a, sizeof(a)) == -1)
  {
    std::cerr << "*** ERROR: Failed to write to HIDRAW device: "
              << SvxLink::strError(errno)
              << std::endl;
    closeDevice();
    return false;
  }

  return true;
} /* PttHidraw::setTxOn */


/*
 * This file has not been truncated
 */

