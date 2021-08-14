/**
@file	 SquelchGpiod.cpp
@brief   A squelch detector that read squelch state from a GPIO port pin
@author  Tobias Blomberg / SM0SVX
@date	 2021-08-13

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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

#include "SquelchGpiod.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/



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

SquelchGpiod::SquelchGpiod(void)
  : m_timer(100, Async::Timer::TYPE_PERIODIC)
{
} /* SquelchGpiod::SquelchGpiod */


SquelchGpiod::~SquelchGpiod(void)
{
  m_timer.setEnable(false);
  //m_watch.setEnabled(false);

  if (m_line != nullptr)
  {
    gpiod_line_release(m_line);
    m_line = nullptr;
  }

  if (m_chip != nullptr)
  {
    gpiod_chip_close(m_chip);
    m_chip = nullptr;
  }
} /* SquelchGpiod::~SquelchGpiod */


bool SquelchGpiod::initialize(Async::Config& cfg, const std::string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }

  std::string chip("gpiochip0");
  cfg.getValue(rx_name, "SQL_GPIOD_CHIP", chip);

  struct gpiod_line_request_config req_cfg;
  req_cfg.consumer = "SvxLink";
  //req_cfg.request_type = GPIOD_LINE_REQUEST_EVENT_BOTH_EDGES;
  req_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
  req_cfg.flags = 0;

  std::string line;
  if (!cfg.getValue(rx_name, "SQL_GPIOD_LINE", line) || line.empty())
  {
    std::cerr << "*** ERROR: Config variable " << rx_name
              << "/SQL_GPIOD_LINE not set or an illegal value was specified"
              << std::endl;
    return false;
  }
  if (line[0] == '!')
  {
    line.erase(0, 1);
    req_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
  }

  std::string bias;
  if (cfg.getValue(rx_name, "SQL_GPIOD_BIAS", bias))
  {
#if (GPIOD_VERSION_MAJOR >= 2) || \
    ((GPIOD_VERSION_MAJOR == 1) && (GPIOD_VERSION_MINOR >= 5))
    if (bias == "PULLUP")
    {
      req_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP;
    }
    else if (bias == "PULLDOWN")
    {
      req_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN;
    }
    else if (bias == "DISABLE")
    {
      req_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE;
    }
    else
    {
      std::cerr << "*** ERROR: Config variable " << rx_name
                << "/SQL_GPIOD_BIAS has an illegal value specified. "
                   "Valid values are: DISABLE, PULLUP and PULLDOWN."
                << std::endl;
      return false;
    }
#else
    std::cerr << "*** WARNING: Config variable " << rx_name
              << "/SQL_GPIOD_BIAS has been specified but the version "
                 "of libgpiod that SvxLink was compiled with ("
              << GPIOD_VERSION_MAJOR << "." << GPIOD_VERSION_MINOR
              << ") does not support configuring BIAS. Need libgpiod >= 1.5."
              << std::endl;
#endif
  }

  m_chip = gpiod_chip_open_lookup(chip.c_str());
  if (m_chip == nullptr)
  {
    std::cerr << "*** ERROR: Open GPIOD chip \"" << chip
              << "\" failed for RX \"" << rx_name << "\": "
              << std::strerror(errno) << std::endl;
    return false;
  }

  int line_num = -1;
  std::istringstream is(line);
  is >> line_num;
  if (!is.fail() && is.eof())
  {
    m_line = gpiod_chip_get_line(m_chip, line_num);
  }
  else
  {
    m_line = gpiod_chip_find_line(m_chip, line.c_str());
  }
  if (!m_line)
  {
    std::cerr << "*** ERROR: Get GPIOD line \"" << line_num
              << "\" failed for RX \"" << rx_name << "\": "
              << std::strerror(errno) << std::endl;
    return false;
  }

  int ret = gpiod_line_request(m_line, &req_cfg, 0);
  if (ret < 0)
  {
    std::cerr << "*** ERROR: Set GPIOD line \"" << line_num
              << "\" to input failed for RX \"" << rx_name << "\": "
              << std::strerror(errno) << std::endl;
    return false;
  }

#if 0
  int fd = gpiod_line_event_get_fd(m_line);
  if (fd < 0)
  {
    std::cerr << "*** ERROR: Retrieve GPIOD line \"" << line_num
              << "\" file descriptor failed for RX \"" << rx_name << "\": "
              << std::strerror(errno) << std::endl;
    return false;
  }

  m_watch.activity.connect(
      hide(mem_fun(*this, &SquelchGpiod::readGpioValueData)));
  m_watch.setFd(fd, Async::FdWatch::FD_WATCH_RD);
  m_watch.setEnabled(true);
#endif

  m_timer.expired.connect([&](Async::Timer*) {
        int val = gpiod_line_get_value(m_line);
        if (val < 0)
        {
          std::cerr << "*** WARNING: Read GPIOD line \"" << line_num
                    << "\" failed for RX \"" << rx_name << "\": "
                    << std::strerror(errno) << std::endl;
          return;
        }
        setSignalDetected(val > 0);
      });

  return true;
} /* SquelchGpiod::initialize */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

#if 0
void SquelchGpiod::readGpioValueData(void)
{
  struct gpiod_line_event event;
  int ret = gpiod_line_event_read_fd(m_watch.fd(), &event);
  if (ret < 0)
  {
    std::cerr << "*** WARNING: SquelchGpiod::readGpioValueData: "
                 "gpiod_line_event_read failed: "
              << std::strerror(errno) << std::endl;
    return;
  }

  int event_type = event.event_type;
  if (event_type == GPIOD_LINE_EVENT_RISING_EDGE)
  {
    setSignalDetected(true);
  }
  else if (event_type == GPIOD_LINE_EVENT_FALLING_EDGE)
  {
    setSignalDetected(false);
  }
} /* SquelchGpiod::readGpioValueData */
#endif


/*
 * This file has not been truncated
 */
