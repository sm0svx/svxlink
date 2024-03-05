/**
@file	 ModuleDtmfRepeater.h
@brief   The DTMF repeater module main file
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-05

\verbatim
A module (plugin) for the svxlink server, a multi purpose tranciever
frontend system.
Copyright (C) 2004-2019 Tobias Blomberg / SM0SVX

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


#ifndef MODULE_DTMF_REPEATER_INCLUDED
#define MODULE_DTMF_REPEATER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>



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
@brief	A_brief_description_of_this_class
@author Tobias Blomberg
@date   2006-08-05
*/
class ModuleDtmfRepeater : public Module
{
  public:
    ModuleDtmfRepeater(void *dl_handle, Logic *logic, const std::string& cfg_name);
    ~ModuleDtmfRepeater(void);
    const char *compiledForVersion(void) const { return SVXLINK_APP_VERSION; }

  protected:
    virtual void resumeOutput(void);
    virtual void allSamplesFlushed(void);
    virtual int writeSamples(const float *samples, int count);
    virtual void flushSamples(void);
  
  private:
    std::string   received_digits;
    Async::Timer  repeat_delay_timer;
    bool          deactivate_on_sql_close;
    
    bool initialize(void);
    void activateInit(void);
    void deactivateCleanup(void);
    bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceivedWhenIdle(const std::string &cmd);
    void squelchOpen(bool is_open);
    void allMsgsWritten(void);

    void setupRepeatDelay(void);
    void onRepeatDelayExpired(void);
    void sendStoredDigits(void);

};  /* class ModuleDtmfRepeater */


//} /* namespace */

#endif /* MODULE_DTMF_REPEATER_INCLUDED */



/*
 * This file has not been truncated
 */
