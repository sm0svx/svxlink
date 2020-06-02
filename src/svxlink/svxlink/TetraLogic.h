/**
@file	 TetraLogic.h
@brief   Contains a Tetra logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2020-05-27

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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


#ifndef TETRA_LOGIC_INCLUDED
#define TETRA_LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>
#include <list>


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
@brief	This class implements a Tetra logic core
@author Adi Bier
@date   2020-05-27
*/
class TetraLogic : public Logic
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg A previously opened configuration
     * @param 	name The configuration section name of this logic
     */
    TetraLogic(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Destructor
     */
    ~TetraLogic(void);

    /**
     * @brief 	Initialize the Tetra logic core
     * @return	Returns \em true if the initialization was successful or else
     *	      	\em false is returned.
     */
    bool initialize(void);

  protected:
    virtual void squelchOpen(bool is_open);
    virtual void transmitterStateChange(bool is_transmitting);

  private:
    class Call;

    bool  mute_rx_on_tx;
    bool  mute_tx_on_rx;
    bool  rgr_sound_always;
    int_fast16_t   mcc;
    int_fast16_t   mnc;
    int_fast16_t   issi;
    int_fast16_t   gssi;
    std::string port;
    int32_t baudrate;
    std::string initstr;

    Async::Serial *pei;

    typedef std::vector<std::string> StrList;
    StrList initcmds;

    /* <CC instance >, <call status>, <AI service>,
       [<calling party identity type>], [<calling party identity>],
       [<hook>], [<simplex>], [<end to end encryption>],
       [<comms type>],
       [<slots/codec>], [<called party identity type>],
       [<called party identity>], [<priority level>] */
    struct Callinfo {
       int instance;
       int callstatus;
       int aistatus;
       int o_mcc;
       int o_mnc;
       int o_issi;
       int origin_cpit;
       int hook;
       int simplex;
       int e2eencryption;
       int commstype;
       int codec;
       int d_mcc;
       int d_mnc;
       int d_issi;
       int dest_cpit;
       int prio;
    };

     // contain user data
    struct user {
      std::string name;
      float lat;
      float lon;
      int16_t state;
      char aprs_icon[2];
    };

    int    peistate;
    std::string peistream;

    typedef enum
    {
       IDLE, CHECK_AT, INIT, INIT_COMPLETE, WAIT
    } Peidef;
    Peidef    peirequest;


    Async::Timer peiComTimer;
    Async::Timer peiActivityTimer;
    Call*    call;

    std::map<int32_t, user> userdata;
    std::map<int32_t, Callinfo> callinfo;

    StrList m_cmds;

    std::string m_ccinstance;
    std::string m_callstatus;
    std::string m_ai_service;
    std::string m_sender;
    std::string m_destination;
    std::string m_calling_party;
    std::string m_cp_identity;
    std::string m_hook;
    std::string m_simplex;
    std::string m_e2e_enc;
    std::string m_codec;
    std::string m_commstype;


    void initPei(void);
    void onCharactersReceived(char *buf, int count);
    void sendPei(std::string cmd);
    int getNextVal(std::string &h);
    std::string getNextStr(std::string& h);
    void onComTimeout(Async::Timer *timer);
    void onPeiActivityTimeout(Async::Timer *timer);
    int handleMessage(std::string mesg);
    void handleGroupcallBegin(std::string);
    void handleGroupcallEnd(std::string);
    bool rmatch(std::string tok, std::string pattern);

};  /* class TetraLogic */


//} /* namespace */

#endif /* TETRA_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

