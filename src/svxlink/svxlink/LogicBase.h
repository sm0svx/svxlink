/**
@file	 LogicBase.h
@brief   The base of a logic core for the SvxLink Server application
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2017  Tobias Blomberg / SM0SVX

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

That LogicBase class only have the most basic functionality
for a logic. The only thing it can do is work with the link manager to
connect to other logic cores and stream audio into and out of the logic.
The LogicBase class can be used to implement all sorts of audio sources
and sinks and not just transceiver based logic cores.
LogicBase is a pure virtual class so it cannot be instatiated on its own.

@example DummyLogic.h
*/
class LogicBase : public sigc::trackable
{
  public:

    /**
     * @brief 	Constuctor
     * @param   cfg A previously initialized configuration object
     * @param   name The name of the logic core
     */
    LogicBase(Async::Config& cfg, const std::string& name)
      : m_cfg(cfg), m_name(name), m_is_idle(true) {}

    /**
     * @brief 	Destructor
     */
    virtual ~LogicBase(void) {}

    /**
     * @brief 	Initialize the logic core
     * @return	Returns \em true on success or \em false on failure
     */
    virtual bool initialize(void)
    {
      if (LinkManager::hasInstance())
      {
          // Register this logic in the link manager
        LinkManager::instance()->addLogic(this);
      }

      return true;
    }

    const std::string& name(void) const { return m_name; }

    /**
     * @brief 	Get the configuration object associated with this logic core
     * @return	Returns the configuration object associated with this logic core
     */
    Async::Config &cfg(void) const { return m_cfg; }

    /**
     * @brief 	Get the idle state of this logic core
     * @return	Returns \em true if the logic core is idle or \em false if not
     */
    bool isIdle(void) const { return m_is_idle; }

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
     * @brief   A signal that is emitted when the idle state change
     * @param   is_idle \em True if the logic core is idle or \em false if not
     */
    sigc::signal<void, bool> idleStateChanged;

  protected:
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

  private:
    Async::Config     	  &m_cfg;
    std::string       	  m_name;
    bool      	      	  m_is_idle;

};  /* class LogicBase */


//} /* namespace */

#endif /* LOGIC_BASE_INCLUDED */


/*
 * This file has not been truncated
 */
