/**
@file	 RepeaterLogic.h
@brief   Contains a class that implements a repeater controller
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2022 Tobias Blomberg / SM0SVX

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


#ifndef REPEATER_LOGIC_INCLUDED
#define REPEATER_LOGIC_INCLUDED


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


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Logic.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
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

class Module;
  

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
@brief	This class implements a repeater controller
@author Tobias Blomberg / SM0SVX
@date   2004-04-24

This class implements a SvxLink logic core repeater controller. It adds
"repeater behaviour" to the Logic base class.
*/
class RepeaterLogic : public Logic
{
  public:
    /**
     * @brief 	Default constructor
     */
    RepeaterLogic(void);

    /**
     * @brief 	Initialize this logic
     * @param 	cfgobj      A previously initialized config object
     * @param 	plugin_name The name of this logic
     * @return	Returns \em true on success or \em false on failure
     */
    virtual bool initialize(Async::Config& cfgobj,
                            const std::string& logic_name) override;

    /**
     * @brief 	Process an event
     * @param 	event Event string
     * @param 	module The calling module or 0 if it's a core event
     */
    virtual void processEvent(const std::string& event, const Module *module=0);
    
    /**
     * @brief 	Called when a module is activated
     * @param 	module The module to activate
     * @return	Returns \em true if the activation went well or \em false if not
     */
    virtual bool activateModule(Module *module);
    
    /**
     * @brief   Set the node to offline or online
     * @param   online Set to \em true to set the node online
     */
    virtual void setOnline(bool online);

    /**
     * @brief 	Called when a DTMF digit has been detected
     * @param 	digit The detected digit
     * @param 	duration The duration of the detected digit
     */
    virtual void dtmfDigitDetected(char digit, int duration);

    /**
     * @brief 	Called when a valid selcall sequence has been detected
     * @param 	sequence The detected sequence
     */
    virtual void selcallSequenceDetected(std::string sequence);


  protected:
    /**
     * @brief 	Destructor
     */
    virtual ~RepeaterLogic(void) override {};

    virtual void allMsgsWritten(void);
    virtual void audioStreamStateChange(bool is_active, bool is_idle);
    virtual void dtmfCtrlPtyCmdReceived(const void *buf, size_t count);
    virtual void setReceivedTg(uint32_t tg) override;

  private:
    typedef enum
    {
      SQL_FLANK_OPEN, SQL_FLANK_CLOSE
    } SqlFlank;
    
    bool      	    repeater_is_up;
    Async::Timer    up_timer;
    Async::Timer    idle_sound_timer;
    struct timeval  rpt_close_timestamp;
    int		    open_on_sql_after_rpt_close;
    char      	    open_on_dtmf;
    std::string     open_on_sel5;
    std::string     close_on_sel5;
    bool      	    activate_on_sql_close;
    bool            no_repeat;
    Async::Timer    open_on_sql_timer;
    SqlFlank  	    open_sql_flank;
    struct timeval  sql_up_timestamp;
    int       	    short_sql_open_cnt;
    int       	    sql_flap_sup_min_time;
    int       	    sql_flap_sup_max_cnt;
    bool            rgr_enable;
    std::string     open_reason;
    int		    ident_nag_min_time;
    Async::Timer    ident_nag_timer;
    uint32_t        delayed_tg_activation;
    Async::Timer    open_on_ctcss_timer;

    void idleTimeout(Async::Timer *t);
    void setIdle(bool idle);
    void setUp(bool up, std::string reason);
    void squelchOpen(bool is_open);
    void detectedTone(float fq);
    void playIdleSound(Async::Timer *t);
    void openOnSqlTimerExpired(Async::Timer *t);
    void activateOnOpenOrClose(SqlFlank flank);
    void identNag(Async::Timer *t);

};  /* class RepeaterLogic */


//} /* namespace */

#endif /* REPEATER_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

