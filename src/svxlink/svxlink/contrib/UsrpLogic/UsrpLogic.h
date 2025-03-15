/**
@file	 UsrpLogic.h
@brief   A logic core that connect to the SvxUsrp
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2021-04-24

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

#ifndef USRP_LOGIC_INCLUDED
#define USRP_LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <iostream>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioDecoder.h>
#include <AsyncAudioEncoder.h>
#include <AsyncTimer.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioStreamStateDetector.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LogicBase.h"
#include "UsrpMsg.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class UdpSocket;
  class AudioValve;
};
class EventHandler;


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
@brief	A logic core that connect to the SvxUsrp
@author Tobias Blomberg / SM0SVX
@date   2021-04-24
*/
class UsrpLogic : public LogicBase
{
  public:
    /**
     * @brief 	Constructor
     * @param   cfg A previously initialized configuration object
     * @param   name The name of the logic core
     */
    UsrpLogic(void);

    /**
     * @brief 	Initialize the logic core
     * @return	Returns \em true on success or \em false on failure
     */
    virtual bool initialize(Async::Config &cfgobj,
                    const std::string &logic_name) override;

    /**
     * @brief 	Get the audio pipe sink used for writing audio into this logic
     * @return	Returns an audio pipe sink object
     */
    virtual Async::AudioSink *logicConIn(void) { return m_logic_con_in; }

    /**
     * @brief 	Get the audio pipe source used for reading audio from this logic
     * @return	Returns an audio pipe source object
     */
    virtual Async::AudioSource *logicConOut(void) { return m_logic_con_out; }

  protected:
    /**
     * @brief 	Destructor
     */
    virtual ~UsrpLogic(void) override {};


  private:
    struct MonitorTgEntry
    {
      uint32_t    tg;
      uint8_t     prio;
      mutable int timeout;
      MonitorTgEntry(uint32_t tg=0) : tg(tg), prio(0), timeout(0) {}
      bool operator<(const MonitorTgEntry& mte) const { return tg < mte.tg; }
      bool operator==(const MonitorTgEntry& mte) const { return tg == mte.tg; }
      operator uint32_t(void) const { return tg; }
    };
    
    typedef std::set<MonitorTgEntry> MonitorTgsSet;
    
         // contain user data
    struct User {
      std::string id;
      std::string mode;
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

    struct Talker {
      std::string name;
      std::string callsign;
      bool isTalking;
    };
    Talker talker;

    std::string                       m_usrp_host;
    uint16_t                          m_usrp_port;
    uint16_t                          m_usrp_rx_port;
    Async::UdpSocket*                 m_udp_rxsock;
    Async::AudioStreamStateDetector*  m_logic_con_in;
    Async::AudioStreamStateDetector*  m_logic_con_out;
    Async::AudioDecoder*              m_dec;
    Async::Timer                      m_flush_timeout_timer;
    struct timeval                    m_last_talker_timestamp;
    Async::AudioEncoder*              m_enc;
    unsigned                          m_tg_select_timeout;
    uint32_t                          m_selected_tg;
    Async::Timer                      m_report_tg_timer;
    MonitorTgsSet                     m_monitor_tgs;
    Async::AudioSource*               m_enc_endpoint;
    // Async::Timer                   m_tmp_monitor_timer;
    int                               udp_seq;
    // Async::Timer                   m_qsy_pending_timer;
    int                               stored_samples;
    int16_t                           *r_buf;
    std::string                       m_callsign;
    bool                              ident;
    uint32_t                          m_dmrid;
    uint32_t                          m_rptid;
    uint8_t                           m_selected_cc;
    uint8_t                           m_selected_ts;
    float                             preamp_gain;
    float                             net_preamp_gain;
    EventHandler*                     m_event_handler;
    uint32_t                          m_last_tg;
    std::string                       m_last_call;
    uint32_t                          m_last_dmrid;
    short                             debug;
    Json::Value                       m_user_info;
    bool                              share_userinfo;
    Async::Timer                      m_delay_timer;
    uint8_t                           m_last_ts;
    uint8_t                           m_last_cc;
    std::string                       m_last_mode;
    std::string                       callsign;

    UsrpLogic(const UsrpLogic&);
    UsrpLogic& operator=(const UsrpLogic&);
    void handleMsgError(std::istream& is);
    void sendEncodedAudio(const void *buf, int count);
    void flushEncodedAudio(void);
    void udpDatagramReceived(const Async::IpAddress& addr, uint16_t port,
                             void *buf, int count);
    void handleStreamStop(void);
    void handleVoiceStream(UsrpAudioMsg usrp);
    void sendAudioMsg(UsrpAudioMsg& usrp);
    void sendStopMsg(void);
    void sendMetaMsg(void);
    void sendUdpMessage(std::ostringstream& ss);
    void sendHeartbeat(void);
    void sendInfoJson(void);
    void allEncodedSamplesFlushed(void);
    void flushTimeout(Async::Timer *t=0);
    void onDelayTimeout(Async::Timer *t);
    void handleTimerTick(Async::Timer *t);
    bool setAudioCodec(void);
    void onLogicConInStreamStateChanged(bool is_active, bool is_idle);
    void onLogicConOutStreamStateChanged(bool is_active, bool is_idle);
    void checkIdle(void);
    bool isIdle(void);
    void handleMetaData(std::string metadata);
    void switchMode(uint8_t mode);
    void processEvent(const std::string& event);
    void handleSettingsMsg(std::string infomsg);
    void handlePlayFile(const std::string& path);
    void handlePlaySilence(int duration);
    void handlePlayTone(int fq, int amp, int duration);
    void handlePlayDtmf(const std::string& digit, int amp,
                                    int duration);
    void onPublishStateEvent(const std::string &event_name, const std::string &msg);
    void sendUserInfo(void);
    void publishInfo(std::string type, Json::Value event);
    
};  /* class UsrpLogic */


#endif /* USRP_LOGIC_INCLUDED */


/*
 * This file has not been truncated
 */
