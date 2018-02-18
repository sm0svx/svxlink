/**
@file	 SipLogic.h
@brief   A logic core that connect to a Sip Server e.g. Asterisk
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2018-02-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

#ifndef SIP_LOGIC_INCLUDED
#define SIP_LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <string>
#include <iostream>
#include <sigc++/sigc++.h>

#include <pjmedia.h>
#include <pjsua-lib/pjsua.h>
#include <pjsua2.hpp>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioDecoder.h>
#include <AsyncAudioEncoder.h>
#include <AsyncTimer.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioPassthrough.h>


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
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

namespace sip
{
  class _Account;
  class _Call;
  class _AudioMedia;
}


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
@brief	A logic core that connect to a Sip Server e.g. Asterisk
@author Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date   2018-02-12
*/
class SipLogic : public LogicBase
{
  public:
    /**
     * @brief 	Constructor
     * @param   cfg A previously initialized configuration object
     * @param   name The name of the logic core
     */
    SipLogic(Async::Config& cfg, const std::string& name);

    /**
     * @brief 	Destructor
     */
    ~SipLogic(void);

    /**
     * @brief 	Initialize the logic core
     * @return	Returns \em true on success or \em false on failure
     */
    virtual bool initialize(void);

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

  private:

    std::string               m_username;
    std::string               m_password;
    std::string               m_sipserver;
    std::string               m_sipextension;
    std::string               m_schema;
    Async::AudioPassthrough*  m_logic_con_in;
    Async::AudioSource*       m_logic_con_out;
    Async::AudioDecoder*      m_dec;
    Async::AudioEncoder*      m_enc;
    uint16_t                  m_siploglevel;
    bool                      m_autoanswer;
    std::string               m_autoconnect;
    uint16_t                  m_sip_port;
    pj::Endpoint              ep;
    sip::_Account             *acc;
    std::vector<pj::Call *>  calls;
    Async::Timer              m_heartbeat_timer;
    Async::Timer              m_flush_timeout_timer;
    uint16_t                  m_reg_timeout;
    std::string               m_callername;

    Async::Timer              m_reconnect_timer;
    uint16_t                  m_next_udp_tx_seq;
    uint16_t                  m_next_udp_rx_seq;
    struct timeval            m_last_talker_timestamp;

    SipLogic(const SipLogic&);
    SipLogic& operator=(const SipLogic&);
    void makeCall(sip::_Account *acc, std::string dest_uri);
    void onIncomingCall(sip::_Account *acc, pj::OnIncomingCallParam &iprm);
    void onRState(sip::_Account *acc, pj::OnRegStateParam &prm);
    bool setAudioCodec(const std::string& codec_name);
    void onDtmfDigit(pj::Call *call, pj::OnDtmfDigitParam &prm);
    void onCallState(pj::Call *call, pj::OnCallStateParam &prm);
    void sendEncodedAudio(const void *buf, int count);
    void onMediaState(pj::Call *call, pj::OnCallMediaStateParam &prm);
    void flushEncodedAudio(void);
    void allEncodedSamplesFlushed(void);
    void flushTimeout(Async::Timer *t=0);

};  /* class SipLogic */


#endif /* SIP_LOGIC_INCLUDED */


/*
 * This file has not been truncated
 */
