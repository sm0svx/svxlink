/**
@file    PttGpiod.cpp
@brief   A PTT hardware controller using a pin in a GPIO port
@author  Tobias Blomberg / SM0SVX
@date    2021-08-13

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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
#if GPIOD_VERSION_MAJOR >= 2
  if (m_request != nullptr)
  {
    gpiod_line_request_release(m_request);
    m_request = nullptr;
  }
#else
  if (m_line != nullptr)
  {
    gpiod_line_release(m_line);
    m_line = nullptr;
  }
#endif

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
  }

#if GPIOD_VERSION_MAJOR >= 2
  m_chip = gpiod_chip_open(chip.c_str());
#else
  m_chip = gpiod_chip_open_lookup(chip.c_str());
#endif

  if (m_chip == nullptr)
  {
    std::cerr << "*** ERROR: Open GPIOD chip \"" << chip << "\" failed for TX "
              << name << ": " << std::strerror(errno) << std::endl;
    return false;
  }

    // Parse line number or name
  int line_num = -1;
  std::istringstream is(line);
  is >> line_num;
  bool is_line_num = (!is.fail() && is.eof());
  if (is_line_num)
  {
#if GPIOD_VERSION_MAJOR >= 2
    m_line_offset = line_num;
#else
    m_line = gpiod_chip_get_line(m_chip, line_num);
#endif
  }
  else
  {
#if GPIOD_VERSION_MAJOR >= 2
    m_line_offset = gpiod_chip_get_line_offset_from_name(m_chip, line.c_str());
    if (m_line_offset < 0)
    {
      std::cerr << "*** ERROR: Get GPIOD line \"" << line
                << "\" failed for TX " << name << ": "
                << std::strerror(errno) << std::endl;
      return false;
    }
#else
    m_line = gpiod_chip_find_line(m_chip, line.c_str());
    if (!m_line)
    {
      std::cerr << "*** ERROR: Get GPIOD line \"" << line
                << "\" failed for TX " << name << ": "
                << std::strerror(errno) << std::endl;
      return false;
    }
#endif
  }

#if GPIOD_VERSION_MAJOR >= 2
  struct gpiod_line_settings* settings = gpiod_line_settings_new();
  if (settings == nullptr)
  {
    std::cerr << "*** ERROR: Failed to create line settings for TX "
              << name << std::endl;
    return false;
  }

  gpiod_line_settings_set_direction(settings, GPIOD_LINE_DIRECTION_OUTPUT);
  gpiod_line_settings_set_output_value(settings,
      active_low ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE);

  if (active_low)
  {
    gpiod_line_settings_set_active_low(settings, true);
  }

    // Create line config
  struct gpiod_line_config* config = gpiod_line_config_new();
  if (config == nullptr)
  {
    std::cerr << "*** ERROR: Failed to create line config for TX "
              << name << std::endl;
    gpiod_line_settings_free(settings);
    return false;
  }

  int ret = gpiod_line_config_add_line_settings(config, &m_line_offset, 1,
                                                settings);
  if (ret < 0)
  {
    std::cerr << "*** ERROR: Failed to add line settings for TX "
              << name << ": " << std::strerror(errno) << std::endl;
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    return false;
  }

    // Create request config
  struct gpiod_request_config* req_config = gpiod_request_config_new();
  if (req_config == nullptr)
  {
    std::cerr << "*** ERROR: Failed to create request config for TX "
              << name << std::endl;
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    return false;
  }
  gpiod_request_config_set_consumer(req_config, "SvxLink");

    // Request the line
  m_request = gpiod_chip_request_lines(m_chip, req_config, config);
  if (m_request == nullptr)
  {
    std::cerr << "*** ERROR: Request GPIOD line \"" << line
              << "\" failed for TX " << name << ": "
              << std::strerror(errno) << std::endl;
    gpiod_request_config_free(req_config);
    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    return false;
  }

    // Clean up temporary objects
  gpiod_request_config_free(req_config);
  gpiod_line_config_free(config);
  gpiod_line_settings_free(settings);
#else
  struct gpiod_line_request_config req_cfg;
  req_cfg.consumer = "SvxLink";
  req_cfg.request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT;
  req_cfg.flags = 0;

  if (active_low)
  {
    req_cfg.flags |= GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW;
  }

  int ret = gpiod_line_request(m_line, &req_cfg, active_low ? 1 : 0);
  if (ret < 0)
  {
    std::cerr << "*** ERROR: Set GPIOD line \"" << line
              << "\" to output failed for TX " << name << ": "
              << std::strerror(errno) << std::endl;
    return false;
  }
#endif

  return true;
} /* PttGpiod::initialize */


bool PttGpiod::setTxOn(bool tx_on)
{
  //cerr << "### PttGpiod::setTxOn(" << (tx_on ? "true" : "false") << ")\n";

#if GPIOD_VERSION_MAJOR >= 2
  enum gpiod_line_value value =
    tx_on ? GPIOD_LINE_VALUE_ACTIVE : GPIOD_LINE_VALUE_INACTIVE;
  int ret = gpiod_line_request_set_value(m_request, m_line_offset, value);
  if (ret < 0)
  {
    std::cerr << "*** WARNING: PttGpiod::setTxOn: "
                 "gpiod_line_request_set_value failed: "
              << std::strerror(errno) << std::endl;
    return false;
  }
#else
  int ret = gpiod_line_set_value(m_line, tx_on ? 1 : 0);
  if (ret < 0)
  {
    std::cerr << "*** WARNING: PttGpiod::setTxOn: "
                 "gpiod_line_set_value failed: "
              << std::strerror(errno) << std::endl;
    return false;
  }
#endif

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
