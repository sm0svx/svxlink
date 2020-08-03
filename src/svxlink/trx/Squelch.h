/**
@file	 Squelch.h
@brief   Base class for implementing a squelch detector
@author  Tobias Blomberg / SM0SVX
@date	 2009-06-30

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
#include <AsyncFactory.h>


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
        m_delay_left(0), m_hangtime(0), m_extended_hangtime(0),
        m_extended_hangtime_enabled(false), m_current_hangtime(0),
        m_hangtime_left(0), m_timeout(0), m_timeout_left(0),
        m_signal_detected(false), m_signal_detected_filtered(false) {}

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
    virtual bool initialize(Async::Config& cfg, const std::string& rx_name);

    /**
     * @brief 	Set the squelch start delay
     * @param 	delay The delay in milliseconds to set
     *
     * Use this function to set the squelch startup delay. The delay is
     * specified in milliseconds. When a value > 0 is specified, the squelch
     * will not open within this time after the Squelch::reset function has
     * been called.
     */
    virtual void setStartDelay(int delay)
    {
      m_start_delay = (delay > 0) ? (delay * INTERNAL_SAMPLE_RATE / 1000) : 0;
    }

    /**
     * @brief 	Set the time the squelch should hang open after squelch close
     * @param 	hang The number of milliseconds to hang
     */
    virtual void setHangtime(int hang)
    {
      m_hangtime = hang;
      if (!m_extended_hangtime_enabled)
      {
        setCurrentHangtime(hang);
      }
    }

    /**
     * @brief 	Set extended time squelch hang open after squelch close
     * @param 	hang The number of milliseconds to hang
     *
     * This is the squelch hangtime that is used in extended hangtime mode.
     */
    virtual void setExtendedHangtime(int hang)
    {
      m_extended_hangtime = hang;
      if (m_extended_hangtime_enabled)
      {
        setCurrentHangtime(hang);
      }
    }

    /**
     * @brief   Choose if extended hangtime mode should be active or not
     * @param   enable Set to \em true to enable or \em false to disable
     *
     * Using extended hangtime mode it is possible to temporarily prolong the
     * time that the squelch will hang open. This can be of use in low signal
     * strength conditions for example. The switch to extended hangtime is not
     * handled by this class so the condition for switching must be handled by
     * the user of this class.
     */
    virtual void enableExtendedHangtime(bool enable)
    {
      m_extended_hangtime_enabled = enable;
      setCurrentHangtime(enable ? m_extended_hangtime : m_hangtime);
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
    virtual void setSqlTimeout(int timeout)
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
      enableExtendedHangtime(false);
    }

    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    virtual int writeSamples(const float *samples, int count);

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
    virtual bool isOpen(void) const { return m_open || (m_hangtime_left > 0); }

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
     * @brief   Set the time the squelch should hang open after squelch close
     * @param   hang The number of milliseconds to hang
     */
    virtual void setCurrentHangtime(int hang)
    {
      m_current_hangtime =
        (hang > 0) ? (hang * INTERNAL_SAMPLE_RATE / 1000) : 0;
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
    int       	m_extended_hangtime;
    bool        m_extended_hangtime_enabled;
    int         m_current_hangtime;
    int       	m_hangtime_left;
    int         m_timeout;
    int         m_timeout_left;
    bool	m_signal_detected;
    bool	m_signal_detected_filtered;

    Squelch(const Squelch&);
    Squelch& operator=(const Squelch&);

    void cfgUpdated(Async::Config& cfg, const std::string& section,
                    const std::string& tag);
    void setSignalDetectedP(bool is_detected);
    void setOpen(bool is_open);

};  /* class Squelch */


/**
 * @brief   Convenience typedef for easier access to the factory members
 *
 * This typedef make it easier to access the members in the Async::Factory
 * class e.g. SquelchFactory::validFactories().
 */
typedef Async::Factory<Squelch> SquelchFactory;


/**
 * @brief   Convenience struct to make specific factory instantiation easier
 *
 * This struct make it easier to create a specific factory for a Squelch
 * class. The constant OBJNAME must be declared in the class that the specific
 * factory is for. To instantiate a specific factory is then as easy as:
 *
 *   static SquelchSpecificFactory<SquelchCtcss> ctcss;
 */
template <class T>
struct SquelchSpecificFactory : public Async::SpecificFactory<Squelch, T>
{
  SquelchSpecificFactory(void)
    : Async::SpecificFactory<Squelch, T>(T::OBJNAME) {}
};


/**
 * @brief   Create a named Squelch-derived object
 * @param   name The OBJNAME of the class
 * @return  Returns a new Squelch or nullptr on failure
 */
Squelch* createSquelch(const std::string& sql_name);


//} /* namespace */

#endif /* SQUELCH_INCLUDED */



/*
 * This file has not been truncated
 */
