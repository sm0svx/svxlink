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
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

#include <version/SVXLINK.h>
#include <Module.h>
#include <AudioPacer.h>



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

namespace Async
{
  class Timer;
  class SampleFifo;
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
    const char *compiledForVersion(void) const { return SVXLINK_VERSION; }

  private:
    Async::SampleFifo 	    *fifo;
    bool      	      	    squelch_is_open;
    AudioPacer	      	    pacer;
    int       	      	    repeat_delay;
    Async::Timer      	    *repeat_delay_timer;
    std::list<std::string>  cmd_queue;
    
    void activateInit(void);
    void deactivateCleanup(void);
    void dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    void squelchOpen(bool is_open);
    int audioFromRx(float *samples, int count);
    void allMsgsWritten(void);

    int audioFromFifo(float *samples, int count);
    void allSamplesWritten(void);
    void onRepeatDelayExpired(Async::Timer *t);
    void execCmdQueue(void);

};  /* class ModuleParrot */


//} /* namespace */

#endif /* MODULE_PARROT_INCLUDED */



/*
 * This file has not been truncated
 */
