/**
@file    PttGpiod.cpp
@brief   A PTT hardware controller using a pin in a GPIO port
@author  Tobias Blomberg / SM0SVX
@date    2021-08-13

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
#include <iostream>
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

#include "PttGpiod.h"



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

PttGpiod::PttGpiod(void)
{
} /* PttGpiod::PttGpiod */


PttGpiod::~PttGpiod(void)
{
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
} /* PttGpiod::~PttGpiod */


bool PttGpiod::initialize(Async::Config &cfg, const std::string name)
{
  std::string chip("gpiochip0");
  cfg.getValue(name, "PTT_GPIOD_CHIP", chip);

  struct gpiod_line_request_config req_cfg;
  req_cfg.consumer = "SvxLink";
  req_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
  req_cfg.flags = 0;

  std::string line;
  if (!cfg.getValue(name, "PTT_GPIOD_LINE", line) || line.empty())
  {
    std::cerr << "*** ERROR: Config variable " << name
              << "/PTT_GPIOD_LINE not set or an illegal value was specified"
              << std::endl;
    return false;
  }
  bool active_low = false;
  if (line[0] == '!')
  {
    active_low = true;
    line.erase(0, 1);
    req_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
  }

  m_chip = gpiod_chip_open_lookup(chip.c_str());
  if (m_chip == nullptr)
  {
    std::cerr << "*** ERROR: Open GPIOD chip \"" << chip
              << "\" failed for TX " << name << ": "
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
              << "\" failed for TX " << name << ": "
              << std::strerror(errno) << std::endl;
    return false;
  }

  int ret = gpiod_line_request(m_line, &req_cfg, active_low ? 1 : 0);
  if (ret < 0)
  {
    std::cerr << "*** ERROR: Set GPIOD line \"" << line_num
              << "\" to output failed for TX " << name << ": "
              << std::strerror(errno) << std::endl;
    return false;
  }

  return true;
} /* PttGpiod::initialize */


bool PttGpiod::setTxOn(bool tx_on)
{
  //cerr << "### PttGpiod::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

  int ret = gpiod_line_set_value(m_line, tx_on ? 1 : 0);
  if (ret < 0)
  {
    std::cerr << "*** WARNING: PttGpiod::setTxOn: "
                 "gpiod_line_set_value failed: "
              << std::strerror(errno) << std::endl;
    return false;
  }

  return true;
} /* PttGpiod::setTxOn */



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
