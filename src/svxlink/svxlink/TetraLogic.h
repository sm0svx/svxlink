/**
@file	 TetraLogic.h
@brief   Contains a Tetra logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2020-05-27

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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
#include <time.h>


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
#include "Squelch.h"
#include "DapNetClient.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/
namespace Async
{
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

class SquelchTetra : public Squelch
{
  public:
     /// The name of this class when used by the object factory 
  static constexpr const char* OBJNAME = "TETRA_SQL";

  SquelchTetra(void) {}

  ~SquelchTetra(void) {}

  void setSql(bool is_open)
  {
    setSignalDetected(is_open);
  }

  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    int processSamples(const float *samples, int count)
    {
      return count;
    }

  private:
    SquelchTetra(const SquelchTetra&);
    SquelchTetra& operator=(const SquelchTetra&);

}; /* SquelchTetra */


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

    /**
     * @brief   is called up when a command from an other logic has been received
     */
    virtual void remoteCmdReceived(LogicBase* src_logic,
                                   const std::string& cmd);

  protected:
    virtual void audioStreamStateChange(bool is_active, bool is_idle);
    virtual void squelchOpen(bool is_open);
    virtual void transmitterStateChange(bool is_transmitting);
    virtual void allMsgsWritten(void);

  private:

    class Call;

    bool  mute_rx_on_tx;
    bool  mute_tx_on_rx;
    bool  rgr_sound_always;
    std::string   mcc;
    std::string   mnc;
    std::string   issi;
    int  gssi;
    std::string port;
    int32_t baudrate;
    std::string initstr;

    Async::Serial *pei;
    Async::Pty    *sds_pty;
    DapNetClient  *dapnetclient;

    typedef std::vector<std::string> StrList;
    StrList initcmds;

    /*
     <CC instance >, <call status>, <AI service>,
     [<calling party identity type>], [<calling party identity>],
     [<hook>], [<simplex>], [<end to end encryption>],
     [<comms type>],
     [<slots/codec>], [<called party identity type>],
     [<called party identity>], [<priority level>]
    */
    struct Callinfo {
       int instance;
       int callstatus;
       int aistatus;
       int origin_cpit;
       int o_mcc;
       int o_mnc;
       int o_issi;
       int hook;
       int simplex;
       int e2eencryption;
       int commstype;
       int codec;
       int dest_cpit;
       int d_mcc;
       int d_mnc;
       int d_issi;
       int prio;
    };
    std::map<int, Callinfo> callinfo;

    struct QsoInfo {
      std::string tsi;
      time_t start;
      time_t stop;
      std::list<std::string> members;
    };
    QsoInfo Qso;

    // contain a sds (state and message)
    struct Sds {
      int id = 0;           // message reference id
      std::string tsi;      // destination TSI
      std::string remark;   // description/state/remark
      std::string message;  // Sds as text
      time_t tos = 0;       // Unix time of sending
      time_t tod = 0;       // Unix time of delivery
      int type;             // STATE, LIP_SHORT,..
      int direction;        // INCOMING, OUTGOING
      int nroftries = 0;    // number of tries
      int aiservice;        // AI service / type of service
    };

    Sds pending_sds;        // the Sds that will actually be handled

    std::map<int, Sds> sdsQueue; // the Sds-Queue that contain all stored Sds's

     // contain user data
    struct User {
      std::string issi;
      std::string call;
      std::string name;
      std::string comment;
      std::string location;
      float lat;
      float lon;
      std::string state;
      short reasonforsending;
      char aprs_sym;
      char aprs_tab;
      time_t last_activity = 0;
      time_t sent_last_sds = 0;
    };
    std::map<std::string, User> userdata;
    
    struct DmoRpt {
      int issi;
      std::string mni;
      int state;
      time_t last_activity;
    };
    std::map<int, DmoRpt> dmo_rep_gw;
    
    std::map<int, std::string> sds_on_activity;
    std::map<unsigned int, std::string> sds_to_command;

    int    peistate;
    std::string peistream;
    short  debug;
    std::string aprspath;
    bool talkgroup_up;

    typedef enum
    {
      IDLE, CHECK_AT, INIT, IGNORE_ERRORS, INIT_COMPLETE, WAIT, AT_CMD_WAIT
    } Peidef;
    Peidef    peirequest;

    // AI Service
    // This parameter is used to determine the type of service to be used
    // in air interface call set up signalling. The services are all
    // defined in EN 300 392-2 [3] or EN 300 396-3 [25].
    typedef enum
    {
      TETRA_SPEECH=0, UNPROTECTED_DATA=1, PACKET_DATA=8, SDS_TYPE1=9,
      SDS_TYPE2=10, SDS_TYPE3=11, SDS_TYPE4=12, STATUS_SDS=13
    } TypeOfService;

    // direction of Sds
    typedef enum
    {
      OUTGOING, INCOMING
    } SdsDirection;
    
     // type of Sds
    typedef enum
    {
      STATE, TEXT, LIP_SHORT, COMPLEX_SDS_TL, RAW
    } SdsType;
    
     // Sds sent state
    typedef enum
    {
      SDS_SEND_OK = 4, SDS_SEND_FAILED = 5
    } SdsSentState;

    bool sds_when_dmo_on;
    bool sds_when_dmo_off;
    bool sds_when_proximity;
    Async::Timer peiComTimer;
    Async::Timer peiActivityTimer;
    Async::Timer peiBreakCommandTimer;
    Call*    call;
    
    struct pSds {
      int sdstype;
      int aiservice;
      std::string fromtsi;
      std::string totsi;
      time_t last_activity;
    };
    pSds pSDS;
    
    std::map<unsigned int, std::string> state_sds;
    StrList m_cmds;
    int pending_sdsid;
    char t_aprs_sym;
    char t_aprs_tab;
    float proximity_warning;
    int time_between_sds;
    SquelchTetra* tetra_modem_sql;
    float own_lat;
    float own_lon;
    std::string endCmd;
    bool new_sds;
    int last_sdsinstance;
    bool inTransmission;
    bool cmgs_received;
    bool share_userinfo;
    Json::Value m_user_info;
    int current_cci;
    int dmnc;
    int dmcc;
    std::string infosds;
    bool is_tx;
    int last_sdsid;

    void initPei(void);
    void onCharactersReceived(char *buf, int count);
    void sendPei(std::string cmd);
    void handlePeiAnswer(std::string m_message);
    void handleSds(std::string sds_head);
    void handleSdsMsg(std::string sds);
    void handleCnumf(std::string m_message);
    std::string handleCtgs(std::string m_message);
    std::string handleCtdgr(std::string m_message);
    void handleClvl(std::string m_message);
    void handleCmgs(std::string m_message);
    std::string handleTextSds(std::string m_message);
    std::string handleSimpleTextSds(std::string m_message);
    void handleStateSds(unsigned int isds);
    void handleTxGrant(std::string txgrant);
    std::string getTSI(std::string issi);
    void onComTimeout(Async::Timer *timer);
    void onPeiActivityTimeout(Async::Timer *timer);
    void onPeiBreakCommandTimeout(Async::Timer *timer);
    void initGroupCall(int gssi);
    void cfmSdsReceived(std::string tsi);
    std::string handleAckSds(std::string m_message, std::string tsi);
    void cfmTxtSdsReceived(std::string message, std::string tsi);
    int handleMessage(std::string  m_message);
    void handleCallBegin(std::string m_message);
    void handleTransmissionEnd(std::string m_message);
    void sdsPtyReceived(const void *buf, size_t count);
    void sendInfoSds(std::string tsi, short reasonforsending);
    void handleCallReleased(std::string m_message);
    int queueSds(Sds t_sds);
    void firstContact(Sds tsds);
    bool checkSds(void);
    void clearOldSds(void);
    void getAiMode(std::string opmode);
    bool rmatch(std::string tok, std::string pattern);
    void sendUserInfo(void);
    void sendWelcomeSds(std::string tsi, short r4s);
    int handleCci(std::string m_message);
    void onPublishStateEvent(const std::string &event_name, const std::string &msg);
    void publishInfo(std::string type, Json::Value event);
    void onDapnetMessage(std::string, std::string message);
    void sendAprs(std::string call, std::string aprsmessage);
    bool checkIfDapmessage(std::string message);
    std::string joinList(std::list<std::string> members);

};  /* class TetraLogic */


//} /* namespace */

#endif /* TETRA_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

