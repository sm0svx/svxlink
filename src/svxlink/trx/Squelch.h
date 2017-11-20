/**
@file	 Squelch.h
@brief   Base class for implementing a squelch detector
@author  Tobias Blomberg / SM0SVX
@date	 2009-06-30

This file contains the base class for implementing a squelch detector

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_INCLUDED
#define SQUELCH_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <string>
#include <iostream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	The base class for implementing a squelch detector
@author Tobias Blomberg / SM0SVX
@date   2005-08-02

This class is used as the base when implementing a squelch detector. The
detector should implement the \em processSamples function. In that function,
call \em setSignalDetected to indicate if a signal is detected ot not.
*/
class Squelch : public sigc::trackable, public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit Squelch(void)
      : m_open(false), m_start_delay(0), m_start_delay_left(0), m_delay(0),
        m_delay_left(0), m_hangtime(0), m_hangtime_left(0), m_timeout(0),
	m_timeout_left(0), m_signal_detected(false),
	m_signal_detected_filtered(false) {}

    /**
     * @brief 	Destructor
     */
    virtual ~Squelch(void) {}

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      m_name = rx_name;

      unsigned delay = 0;
      if (!cfg.getValue(rx_name, "SQL_DELAY", delay, true))
      {
        std::cerr << "*** ERROR: Error reading config variable "
                  << rx_name << "/SQL_DELAY\n";
        return false;
      }
      setDelay(delay);

      unsigned start_delay = 0;
      if (!cfg.getValue(rx_name, "SQL_START_DELAY", start_delay, true))
      {
        std::cerr << "*** ERROR: Error reading config variable "
                  << rx_name << "/SQL_START_DELAY\n";
        return false;
      }
      setStartDelay(start_delay);

      unsigned timeout = 0;
      if (!cfg.getValue(rx_name, "SQL_TIMEOUT", timeout, true))
      {
        std::cerr << "*** ERROR: Error reading config variable "
                  << rx_name << "/SQL_TIMEOUT\n";
        return false;
      }
      setSqlTimeout(timeout);

      return true;
    }

    /**
     * @brief 	Set the squelch start delay
     * @param 	delay The delay in milliseconds to set
     *
     * Use this function to set the squelch startup delay. The delay is
     * specified in milliseconds. When a value > 0 is specified, the squelch
     * will not open within this time after the Squelch::reset function has
     * been called.
     */
    void setStartDelay(int delay)
    {
      m_start_delay = (delay > 0) ? (delay * INTERNAL_SAMPLE_RATE / 1000) : 0;
    }

    /**
     * @brief 	Set the time the squelch should hang open after squelch close
     * @param 	hang The number of milliseconds to hang
     */
    virtual void setHangtime(int hang)
    {
      m_hangtime = (hang > 0) ? (hang * INTERNAL_SAMPLE_RATE / 1000) : 0;
    }

    /**
     * @brief 	Set the time a squelch open should be delayed
     * @param 	delay The delay in milliseconds
     */
    virtual void setDelay(int delay)
    {
      m_delay = ((delay > 0) ? (delay * INTERNAL_SAMPLE_RATE / 1000) : 0);
    }

    /**
     * @brief 	Set the maximum time the squelch is allowed to stay open
     * @param 	timeout The squelch timeout in seconds
     */
    void setSqlTimeout(int timeout)
    {
      m_timeout = ((timeout > 0) ? (timeout * INTERNAL_SAMPLE_RATE) : 0);
    }

    /**
     * @brief 	Reset the squelch detector
     *
     *  Reset the squelch so that the detection process starts from
     *	the beginning again.
     */
    virtual void reset(void)
    {
      m_open = false;
      m_hangtime_left = 0;
      m_start_delay_left = m_start_delay;
      m_delay_left = 0;
      m_timeout_left = 0;
      m_signal_detected = false;
      m_signal_detected_filtered = false;
    }

    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    int writeSamples(const float *samples, int count)
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
    }

    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    void flushSamples(void)
    {
      sourceAllSamplesFlushed();
    }

    /**
     * @brief 	Get the state of the squelch
     * @return	Return \em true if the squelch is open, or else \em false
     */
    bool isOpen(void) const { return m_open || (m_hangtime_left > 0); }

    /**
     * @brief 	A signal that indicates when the squelch state changes
     * @param 	is_open Is set to \em true if the squelch is open or else
     *	      	\em false
     */
    sigc::signal<void, bool> squelchOpen;

  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    virtual int processSamples(const float *samples, int count)
    {
      return count;
    }

    /**
     * @brief 	Used by the actual squelch detector to indicate signal presence
     * @param 	is_detected Set to \em true if a signal is detected
     */
    inline void setSignalDetected(bool is_detected)
    {
      if (m_signal_detected != is_detected)
      {
	setSignalDetectedP(is_detected);
      }
    }

    /**
     * @brief 	Return the state of the signal detector
     * @return	Return \em true if a signal is detected
     */
    bool signalDetected(void)
    {
      return m_signal_detected;
    }

    /**
     * @brief   Return the name of the associated receiver
     * @return  Returns the name of the associated receiver
     */
    const std::string &rxName(void) const { return m_name; }

  private:
    std::string	m_name;
    bool      	m_open;
    int       	m_start_delay;
    int       	m_start_delay_left;
    int       	m_delay;
    int       	m_delay_left;
    int       	m_hangtime;
    int       	m_hangtime_left;
    int         m_timeout;
    int         m_timeout_left;
    bool	m_signal_detected;
    bool	m_signal_detected_filtered;

    Squelch(const Squelch&);
    Squelch& operator=(const Squelch&);

    void setSignalDetectedP(bool is_detected)
    {
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
	if (m_hangtime == 0)
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
	    m_hangtime_left = m_hangtime;
	  }
	}
      }
    }

    /**
     * @brief 	Set the state of the squelch
     * @param 	is_open Set to \em true if the squelch is open or \em false
     *			if it's not
     */
    void setOpen(bool is_open)
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
    }

};  /* class Squelch */


//} /* namespace */

#endif /* SQUELCH_INCLUDED */



/*
 * This file has not been truncated
 */
