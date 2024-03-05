/**
@file	 ModuleParrot.h
@brief   A module that implements a "parrot" function.
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This module implements a "parrot" function. It plays back everything you say
to it. This can be used as a simplex repeater or just so you can hear how
you sound.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2015 Tobias Blomberg / SM0SVX

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


#ifndef MODULE_PARROT_INCLUDED
#define MODULE_PARROT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>
#include <Module.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXLINK.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class SampleFifo;
  class AudioValve;
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
@brief	A module that provides a "parrot" function.h
@author Tobias Blomberg
@date   2004-03-07
*/
class ModuleParrot : public Module
{
  public:
    ModuleParrot(void *dl_handle, Logic *logic, const std::string& cfg_name);
    ~ModuleParrot(void);
    bool initialize(void);
    const char *compiledForVersion(void) const { return SVXLINK_APP_VERSION; }

  protected:
    /**
     * @brief 	Notify the module that the logic core idle state has changed
     * @param 	is_idle Set to \em true if the logic core is idle or else
     *	      	\em false.
     *
     * This function is called by the logic core when the idle state changes.
     */
    virtual void logicIdleStateChanged(bool is_idle);
  
  private:
    class FifoAdapter;
    friend class FifoAdapter;
    
    FifoAdapter       	    *adapter;
    Async::AudioFifo	    *fifo;
    Async::AudioValve 	    *valve;
    bool      	      	    squelch_is_open;
    Async::Timer      	    repeat_delay_timer;
    std::list<std::string>  cmd_queue;
    
    void activateInit(void);
    void deactivateCleanup(void);
    bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    void dtmfCmdReceivedWhenIdle(const std::string &cmd);
    void squelchOpen(bool is_open);

    void allSamplesWritten(void);
    void onRepeatDelayExpired(void);
    void execCmdQueue(void);

};  /* class ModuleParrot */


//} /* namespace */

#endif /* MODULE_PARROT_INCLUDED */



/*
 * This file has not been truncated
 */
