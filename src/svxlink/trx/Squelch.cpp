/**
@file    Squelch.cpp
@brief   Base class for implementing a squelch detector
@author  Tobias Blomberg / SM0SVX
@date    2020-07-27

This file contains the base class for implementing a squelch detector

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2020  Tobias Blomberg / SM0SVX

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

#include "SquelchVox.h"
#include "SquelchCtcss.h"
#include "SquelchSerial.h"
#include "SquelchSigLev.h"
#include "SquelchEvDev.h"
#include "SquelchGpio.h"
#include "SquelchCombine.h"
#include "SquelchPty.h"
#include "SquelchOpen.h"
#ifdef HAS_HIDRAW_SUPPORT
#include "SquelchHidraw.h"
#endif
#include "Squelch.h"


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

namespace {
  typedef const char *CfgTag;
  CfgTag CFG_SQL_DELAY                    = "SQL_DELAY";
  CfgTag CFG_SQL_START_DELAY              = "SQL_START_DELAY";
  CfgTag CFG_SQL_TIMEOUT                  = "SQL_TIMEOUT";
  CfgTag CFG_SQL_HANGTIME                 = "SQL_HANGTIME";
  CfgTag CFG_SQL_EXTENDED_HANGTIME        = "SQL_EXTENDED_HANGTIME";
};


/****************************************************************************
 *
 * Public functions
 *
 ****************************************************************************/

Squelch* createSquelch(const std::string& sql_name)
{
  static SquelchSpecificFactory<SquelchOpen> open_factory;
  static SquelchSpecificFactory<SquelchVox> vox_factory;
  static SquelchSpecificFactory<SquelchCtcss> ctcss_factory;
  static SquelchSpecificFactory<SquelchSerial> serial_factory;
  static SquelchSpecificFactory<SquelchSigLev> siglev_factory;
  static SquelchSpecificFactory<SquelchEvDev> evdev_factory;
  static SquelchSpecificFactory<SquelchGpio> gpio_factory;
  static SquelchSpecificFactory<SquelchPty> pty_factory;
#ifdef HAS_HIDRAW_SUPPORT
  static SquelchSpecificFactory<SquelchHidraw> hidraw_factory;
#endif
  static SquelchSpecificFactory<SquelchCombine> combine_factory;

  return SquelchFactory::createNamedObject(sql_name);
} /* createSquelch */


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

bool Squelch::initialize(Async::Config& cfg, const std::string& rx_name)
{
  m_name = rx_name;

  unsigned delay = 0;
  if (cfg.getValue(rx_name, CFG_SQL_DELAY, delay))
  {
    setDelay(delay);
  }

  unsigned start_delay = 0;
  if (cfg.getValue(rx_name, CFG_SQL_START_DELAY, start_delay))
  {
    setStartDelay(start_delay);
  }

  unsigned timeout = 0;
  if (cfg.getValue(rx_name, CFG_SQL_TIMEOUT, timeout))
  {
    setSqlTimeout(timeout);
  }

  int hangtime = 0;
  if (cfg.getValue(rx_name, CFG_SQL_HANGTIME, hangtime))
  {
    setHangtime(hangtime);
  }

  int ext_hangtime = 0;
  if (cfg.getValue(rx_name, CFG_SQL_EXTENDED_HANGTIME, ext_hangtime))
  {
    setExtendedHangtime(ext_hangtime);
  }

  cfg.valueUpdated.connect(
      sigc::bind<0>(sigc::mem_fun(*this, &Squelch::cfgUpdated), cfg));

  return true;
} /* Squelch::initialize */


int Squelch::writeSamples(const float *samples, int count)
{
  int orig_count = count;

  if (m_timeout_left > 0)
  {
    m_timeout_left -= count;
    if (m_timeout_left <= 0)
    {
      std::cerr << "*** WARNING: The squelch was open for too long for "
                << "receiver " << m_name << ". " << "Forcing it closed.\n";
      setOpen(false);
    }
  }

  if (m_start_delay_left > 0)
  {
    int sample_cnt = std::min(count, m_start_delay_left);
    m_start_delay_left -= sample_cnt;
    count -= sample_cnt;
    samples += sample_cnt;

    if (count == 0)
    {
      return orig_count;
    }
  }

  while (count > 0)
  {
    int ret_count = processSamples(samples, count);
    if (ret_count <= 0)
    {
      std::cout << "*** WARNING: " << count
                << " samples dropped in squelch detector for receiver "
                << m_name << std::endl;
      break;
    }
    samples += ret_count;
    count -= ret_count;
  }

  if (m_hangtime_left > 0)
  {
    m_hangtime_left -= orig_count;
    if (m_hangtime_left <= 0)
    {
      m_signal_detected_filtered = false;
      setOpen(false);
    }
  }

  if (m_delay_left > 0)
  {
    m_delay_left -= orig_count;
    if (m_delay_left <= 0)
    {
      m_signal_detected_filtered = true;
      setOpen(true);
    }
  }

  return orig_count;
} /* Squelch::writeSamples */


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

void Squelch::cfgUpdated(Async::Config& cfg, const std::string& section,
                         const std::string& tag)
{
  //std::cout << "### Squelch::cfgUpdated: "
  //          << section << "/" << tag << "=" << cfg.getValue(section, tag)
  //          << std::endl;
  if (section == m_name)
  {
    if (tag == CFG_SQL_HANGTIME)
    {
      int hangtime = 0;
      if (cfg.getValue(m_name, CFG_SQL_HANGTIME, hangtime))
      {
        setHangtime(hangtime);
      }
      std::cout << "Setting " << CFG_SQL_HANGTIME << " to " << hangtime
                << " for receiver " << m_name << std::endl;
    }
    else if (tag == CFG_SQL_EXTENDED_HANGTIME)
    {
      int ext_hangtime = 0;
      cfg.getValue(m_name, CFG_SQL_EXTENDED_HANGTIME, ext_hangtime);
      std::cout << "Setting " << CFG_SQL_EXTENDED_HANGTIME << " to "
                << ext_hangtime
                << " for receiver " << m_name << std::endl;
    }
  }
} /* LocalRxBase::cfgUpdated */


void Squelch::setSignalDetectedP(bool is_detected)
{
  //std::cout << "### Squelch::setSignalDetectedP: is_detected="
  //          << is_detected << std::endl;

  m_signal_detected = is_detected;

  if (is_detected)
  {
    m_hangtime_left = 0;
    if (m_delay == 0)
    {
      if (!m_signal_detected_filtered)
      {
        m_signal_detected_filtered = true;
        setOpen(true);
      }
    }
    else
    {
      if (!m_signal_detected_filtered && (m_delay_left <= 0))
      {
        m_delay_left = m_delay;
      }
    }
  }
  else
  {
    m_delay_left = 0;
    if (m_current_hangtime == 0)
    {
      if (m_signal_detected_filtered)
      {
        m_signal_detected_filtered = false;
        setOpen(false);
      }
    }
    else
    {
      if (m_signal_detected_filtered && (m_hangtime_left <= 0))
      {
        m_hangtime_left = m_current_hangtime;
      }
    }
  }
} /* Squelch::setSignalDetectedP */


void Squelch::setOpen(bool is_open)
{
  if (is_open == m_open)
  {
    return;
  }

  if (is_open)
  {
    m_timeout_left = m_timeout;
  }
  else
  {
    m_timeout_left = 0;
  }

  m_open = is_open;
  squelchOpen(is_open);
} /* Squelch::setOpen */


/*
 * This file has not been truncated
 */
