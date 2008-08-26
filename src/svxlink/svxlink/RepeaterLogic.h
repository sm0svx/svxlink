/**
@file	 RepeaterLogic.h
@brief   Contains a class that implements a repeater controller
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008 Tobias Blomberg / SM0SVX

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
  class Timer;
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
     * @brief 	Constuctor
     */
    RepeaterLogic(Async::Config& cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~RepeaterLogic(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool initialize(void);
    
    virtual void processEvent(const std::string& event, const Module *module=0);
    virtual bool activateModule(Module *module);
    virtual void dtmfDigitDetected(char digit, int duration);


  protected:
    virtual void allMsgsWritten(void);
    virtual void audioStreamStateChange(bool is_active, bool is_idle);


  private:
    typedef enum
    {
      SQL_FLANK_OPEN, SQL_FLANK_CLOSE
    } SqlFlank;
    
    bool      	    repeater_is_up;
    Async::Timer    *up_timer;
    int      	    idle_timeout;
    Async::Timer    *idle_sound_timer;
    int       	    idle_sound_interval;
    //bool      	    preserve_idle_state;
    int      	    required_sql_open_duration;
    char      	    open_on_dtmf;
    bool      	    activate_on_sql_close;
    bool            no_repeat;
    Async::Timer    *open_on_sql_timer;
    SqlFlank  	    open_sql_flank;
    struct timeval  sql_up_timestamp;
    int       	    short_sql_open_cnt;
    int       	    sql_flap_sup_min_time;
    int       	    sql_flap_sup_max_cnt;
    
    void idleTimeout(Async::Timer *t);
    void setIdle(bool idle);
    void setUp(bool up, bool ident=true);
    void squelchOpen(bool is_open);
    void detectedTone(float fq);
    void playIdleSound(Async::Timer *t);
    void openOnSqlTimerExpired(Async::Timer *t);
    void activateOnOpenOrClose(SqlFlank flank);

};  /* class RepeaterLogic */


//} /* namespace */

#endif /* REPEATER_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

