/**
@file	 LogicBase.h
@brief   The base of a logic core for the SvxLink Server application
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2022  Tobias Blomberg / SM0SVX

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


#ifndef LOGIC_BASE_INCLUDED
#define LOGIC_BASE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncPlugin.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LinkManager.h"



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class AudioSink;
  class AudioSource;
};


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
@brief	This class implements the base for core logic of SvxLink
@author Tobias Blomberg
@date   2017-02-10

The LogicBase class only have the most basic functionality
for a logic. The only thing it can do is work with the link manager to
connect to other logic cores and stream audio into and out of the logic.
The LogicBase class can be used to implement all sorts of audio sources
and sinks and not just transceiver based logic cores.
LogicBase is an abstact class so it cannot be instatiated on its own.

@example DummyLogic.h
*/
class LogicBase : public Async::Plugin, public sigc::trackable
{
  public:
    // Return the name of this plugin type
    static std::string typeName(void) { return "Logic"; }

    /**
     * @brief 	Default constructor
     */
    LogicBase(void) {}

    /**
     * @brief   Initialize the logic core
     * @param   cfgobj      A previously initialized configuration object
     * @param   plugin_name The name of the logic core
     * @return  Returns \em true on success or \em false on failure
     */
    virtual bool initialize(Async::Config& cfgobj,
                            const std::string& logic_name)
    {
      m_cfg = &cfgobj;
      m_name = logic_name;

      if (!m_cfg->getValue(m_name, "TYPE", m_type) || m_type.empty())
      {
        return false;
      }

      if (LinkManager::hasInstance())
      {
          // Register this logic in the link manager
        LinkManager::instance()->addLogic(this);
      }

      return true;
    }

    const std::string& name(void) const { return m_name; }

    const std::string& type(void) const { return m_type; }

    /**
     * @brief 	Get the configuration object associated with this logic core
     * @return	Returns the configuration object associated with this logic core
     */
    Async::Config &cfg(void) const { return *m_cfg; }

    /**
     * @brief 	Get the idle state of this logic core
     * @return	Returns \em true if the logic core is idle or \em false if not
     */
    bool isIdle(void) const { return m_is_idle; }

    /**
     * @brief   Set state for logic linking muting
     * @param   mute Set to \em true to mute this logic core
     *
     * Muting a logic core is about the same as disconnecting it from the logic
     * linking layer. Muting is used when needing to temporarity block
     * interaction with other logic cores.
     */
    void setMuteLinking(bool mute)
    {
      if (LinkManager::hasInstance())
      {
        LinkManager::instance()->setLogicMute(this, mute);
      }
    }

    /**
     * @brief 	Get the audio pipe sink used for writing audio into this logic
     * @return	Returns an audio pipe sink object
     */
    virtual Async::AudioSink *logicConIn(void) = 0;

    /**
     * @brief 	Get the audio pipe sink used for writing audio into this logic
     * @return	Returns an audio pipe sink object
     */
    virtual Async::AudioSource *logicConOut(void) = 0;

    /**
     * @brief   A command has been received from another logic
     * @param   cmd The received command
     *
     * This function is typically called when a link activation command is
     * issued to connect two or more logics together.
     */
    virtual void remoteCmdReceived(LogicBase* src_logic,
                                   const std::string& cmd) {}

    /**
     * @brief   Get the talk group associated with current reception
     * @return  Returns the current TG id if provided by the logic
     */
    uint32_t receivedTg(void) const { return m_received_tg; }


    /**
     * @brief   Play the given file
     * @param   path The full path to the file to play
     */
    virtual void playFile(const std::string& path) {}

    /**
     * @brief   Play the a length of silence
     * @param   length The length, in milliseconds, of silence to play
     */
    virtual void playSilence(int length) {}

    /**
     * @brief   Play a tone with the given properties
     * @param   fq The tone frequency
     * @param   amp The tone amplitude in "milliunits", 1000=full strength
     * @param   len The length of the tone in milliseconds
     */
    virtual void playTone(int fq, int amp, int len) {}

    /**
     * @brief   Play DTMF digits
     * @param   digits The DTMF digits to play
     * @param   amp The amplitude of the individual DTMF tones (0-1000)
     * @param   len The length in milliseconds of the digit
     */
    virtual void playDtmf(const std::string& digits, int amp, int len) {}

    /**
     * @brief   A linked logic has updated its recieved talk group
     * @param   logic The pointer to the remote logic object
     * @param   tg    The new received talk group
     */
    virtual void remoteReceivedTgUpdated(LogicBase *logic, uint32_t tg) {}

    /**
     * @brief   A linked logic has published a state event
     * @param   logic       The pointer to the remote logic object
     * @param   event_name  The name of the event
     * @param   msg The state update message
     *
     * This function is called when a linked logic has published a state update
     * event message. A state update message is a free text message that can be
     * used by subscribers to act on certain state changes within SvxLink. The
     * event name must be unique within SvxLink. The recommended format is
     * <context>:<name>, e.g. Rx:sql_state.
     */
    virtual void remoteReceivedPublishStateEvent(
        LogicBase *logic, const std::string& event_name,
        const std::string& msg) {}

    /**
     * @brief   A signal that is emitted when the idle state change
     * @param   is_idle \em True if the logic core is idle or \em false if not
     */
    sigc::signal<void, bool> idleStateChanged;

    /**
     * @brief   A signal that is emitted when the received talk group changes
     * @param   tg The new talk group
     */
    sigc::signal<void, uint32_t> receivedTgUpdated;

    /**
     * @brief   A signal that is emitted to publish a state update event
     * @param   event_name  The name of the event
     * @param   msg         The state update message
     *
     * This signal is emitted when this logic wish to publish a state update
     * message. A state update message is a free text message that can be used
     * by subscribers to act on certain state changes within SvxLink. The
     * event name must be unique within SvxLink. The recommended format is
     * <context>:<name>, e.g. Rx:sql_state.
     */
    sigc::signal<void, const std::string&,
                 const std::string&> publishStateEvent;

  protected:
    /**
     * @brief 	Destructor
     */
    virtual ~LogicBase(void) override
    {
      if (LinkManager::hasInstance())
      {
          // Unregister this logic from the link manager
        LinkManager::instance()->deleteLogic(this);
      }
    }

    /**
     * @brief   Used by derived classes to set the idle state of the logic core
     * @param   set_idle \em True to set to idle or \em false to set to active
     *
     * This function will set the idle state. If setting it change the current
     * state, the idleStateChanged signal will be emitted.
     */
    void setIdle(bool set_idle)
    {
      if (set_idle != m_is_idle)
      {
        m_is_idle = set_idle;
        idleStateChanged(m_is_idle);
      }
    }

    /**
     * @brief   Used by a logic to indicate received talk group
     * @param   tg The received talk group
     *
     * This function is used by a logic implementation to set which talk group
     * that local traffic is received on.
     */
    virtual void setReceivedTg(uint32_t tg)
    {
      m_received_tg = tg;
      receivedTgUpdated(tg);
    }

  private:
    Async::Config*     	  m_cfg           = nullptr;
    std::string       	  m_name;
    std::string           m_type;
    bool      	      	  m_is_idle       = true;
    uint32_t              m_received_tg   = 0;

};  /* class LogicBase */


//} /* namespace */

#endif /* LOGIC_BASE_INCLUDED */


/*
 * This file has not been truncated
 */
