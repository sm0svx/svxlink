/**
@file	 AnalogPhoneLogic.h
@brief   Contains a class that implements a repeater controller
@author  Tobias Blomberg / SM0SVX
@date	 2004-04-24

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-20089 Tobias Blomberg / SM0SVX

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


#ifndef ANALOGPHONE_LOGIC_INCLUDED
#define ANALOGPHONE_LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <list>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/
#include <AsyncSerial.h>


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
class AnalogPhoneLogic : public Logic
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg A previously initialized config object
     * @param 	name The name of this logic
     */
    AnalogPhoneLogic(Async::Config& cfg, const std::string& name);

    /**
     * @brief 	Destructor
     */
    ~AnalogPhoneLogic(void);

    /**
     * @brief 	Initialize this logic
     * @return	Returns \em true on success or \em false on failure
     */
    bool initialize(void);

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
     * @brief 	Called when a DTMF digit has been detected
     * @param 	digit The detected digit
     * @param 	duration The duration of the detected digit
     */
    virtual void dtmfDigitDetected(char digit, int duration);

    /** pickup phoneline by external Logic (Repeater or Simplex) */
    void pickupRemote(const std::string reason);

    /** hangup phoneline by external Logic (Repeater or Simplex) */
    void hangupRemote(const std::string reason);

    /** **/
    void requestAuth(std::string reason);

    /** **/
    void discPhone(std::string reason);


  protected:
    virtual void allMsgsWritten(void);
    virtual void audioStreamStateChange(bool is_active, bool is_idle);


  private:
    typedef enum
    {
      SQL_FLANK_OPEN, SQL_FLANK_CLOSE
    } SqlFlank;

    typedef std::map<int, std::string> modemCmd;
    modemCmd modem_cmds;

    typedef std::map<std::string, std::string> usersMap;
    usersMap users_map;

    typedef std::map<int, int> toneMap;
    toneMap busy_tone_len;

    typedef std::vector<std::string> StrList;
    StrList         user_list;

    Async::Serial   *modem;
    Async::Timer    *max_timer;
    Async::Timer    *dialtone_timer;
    int      	    required_sql_open_duration;
    struct timeval  rpt_close_timestamp;
    int		        open_on_sql_after_rpt_close;
    bool      	    activate_on_sql_close;
    Async::Timer    *open_on_sql_timer;
    SqlFlank  	    open_sql_flank;
    struct timeval  sql_up_timestamp;
    int       	    short_sql_open_cnt;
    int       	    sql_flap_sup_min_time;
    int       	     sql_flap_sup_max_cnt;
    int             busy_max;
    int             busy_min;
   // std::string     open_reason;
    Async::Timer    *auth_timer;
    Async::Timer    *pickup_timer;
    Async::Timer    *modem_to_timer;
    Async::Timer    *modem_dial_timer;
    Async::Timer    *modem_reset_timer;
    Async::Timer    *modem_hangup_timer;
    Async::Timer    *modem_voicecmd_timer;
    int             modem_speed;
    std::string     modem_port;
    std::string     m_response[10];

    bool           auth;
    bool           auth_request;
    int            auth_timeout;
    int            busy_tone_freq;
    int            cnt;
    bool           dialtone_timeout;
    int            hangup_counter;
    int		       ident_nag_timeout;
    int		       ident_nag_min_time;
    Async::Timer    *ident_nag_timer;
    int       	   idle_sound_interval;
    Async::Timer    *idle_sound_timer;
    int      	   idle_timeout;
    int            line_state;
    bool           max_timeout;
    int            max_rings;
    bool           modem_init_ok;
    int            norepeat;
    char      	    open_on_dtmf;
    bool      	    phoneline_is_up;
    int            pickup_delay;
    bool           rgr_enable;
    int            rings;
    int            vcon_timeout;
    int            modem_response_timeout;
    bool           voice_ok;
    std::string     autodial_nr;
    std::string     in_pin;
    std::string     access_pin;
    std::string     phonenumber;
    std::string     phoneuser;
    Async::Timer    *up_timer;
    Async::Timer    *wait_phone_connection_timer;
    bool           debug;
    void idleTimeout(Async::Timer *t);
    void authExpired(Async::Timer *t);
    void maxTimeout(Async::Timer *t);
    bool checkforDialtone(void);
    void dialtoneTimeout(Async::Timer *t);
    void pickupLine(Async::Timer *t);
    void authTimeout(Async::Timer *t);
    void pickupLocal(std::string reason);
    void hangupLocal(std::string reason);
    void setIdle(bool idle);
    void setUp(bool up, std::string reason);
    void detectedTone(long len);
    void playIdleSound(Async::Timer *t);
    void openOnSqlTimerExpired(Async::Timer *t);
    void waitPhoneConnection(Async::Timer *t);
    void activateOnOpenOrClose(SqlFlank flank);
    void identNag(Async::Timer *t);
    void pickup(void);
    void initModem(int extime, int modem_ser);
    void resetModem(void);
    void modemDialTimeout(Async::Timer *t);
    void modemInitTimeout(Async::Timer *t);
    void modemResetTimeout(Async::Timer *t);
    void modemHangupTimeout(Async::Timer *t);
    void pickupTimeout(Async::Timer *t);
    void dialPhoneNumber(std::string number);
    void writeModem(std::string modemstr);
    void squelchOpen(bool is_open);
    void onModemDataReceived(char *buf, int count);
    int splitStr(StrList& L, const std::string& seq, const std::string& delims);

};  /* class AnalogPhoneLogic */


//} /* namespace */

#endif /* ANALOGPHONE_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

