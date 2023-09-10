/**
@file	 AnnounceLogic.h
@brief   Contains a Announce logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2022-06-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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


#ifndef ANNOUNCE_LOGIC_INCLUDED
#define ANNOUNCE_LOGIC_INCLUDED


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

#include <AsyncAudioPassthrough.h>
#include <AsyncAtTimer.h>
 

/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LogicBase.h"


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

class MsgHandler;
class EventHandler;


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
@brief	This class implements a Announce logic core
@author Tobias Blomberg & Adi Bier / DL1HRC
@date   2022-06-08
*/
class AnnounceLogic : public LogicBase
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg A previously opened configuration
     * @param 	name The configuration section name of this logic
     */
    AnnounceLogic(void);

    /**
     * @brief 	Initialize the Announce logic core
     * @return	Returns \em true if the initialization was successful or else
     *	      	\em false is returned.
     */
    virtual bool initialize(Async::Config &cfgobj, const std::string &logic_name);

    /**
     * @brief 	Get the audio pipe sink used for writing audio into this logic
     * @return	Returns an audio pipe sink object
     */
    virtual Async::AudioSink *logicConIn(void) { return m_logic_con_in; }

    /**
     * @brief 	Get the audio pipe source used for reading audio from this logic
     * @return	Returns an audio pipe source object
     */
    virtual Async::AudioSelector *logicConOut(void) { return m_logic_con_out; }


  protected:

    /**
     * @brief 	Destructor
     */
    virtual ~AnnounceLogic(void) override;

    virtual void allMsgsWritten(void);


  private:

    Async::AudioSelector*     m_logic_con_out;
    Async::AudioPassthrough*  m_logic_con_in;
    bool                      report_events_as_idle;
    EventHandler              *event_handler;
    MsgHandler                *msg_handler;
    int                       pre_interval;
    int                       day_of_week;
    std::vector<std::string>  days;
    Async::AtTimer            every_minute_timer;
    int                       start_prenotify_before;
    int                       hour_of_qst;
    int                       minute_of_qst;
    struct tm                 next_qst_tm;
    int                       cnt;

    AnnounceLogic(const AnnounceLogic&);
    AnnounceLogic& operator=(const AnnounceLogic&);

    void processEvent(const std::string& event);
    void playFile(const std::string& path);
    void playSilence(int length);
    void playTone(int fq, int amp, int len);
    void playDtmf(const std::string& digits, int amp, int len);
    void timeoutNextMinute(void);
    void everyMinute(Async::AtTimer *t);
    void prenotification(void);
    void announceQst(void);
    bool check_week_of_month(struct tm t);
    bool getConfigValue(const std::string& section, const std::string& tag,
                        std::string& value);

};  /* class AnnounceLogic */


//} /* namespace */

#endif /* ANNOUNCE_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

