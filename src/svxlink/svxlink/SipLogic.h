/**
@file	 SipLogic.h
@brief   A logic core that connect to a Sip Server e.g. Asterisk
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2018-02-12

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#include <AsyncTimer.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioDecoder.h>
#include <AsyncAudioEncoder.h>
#include <AsyncAudioReader.h>


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
};
namespace Async
{
  class Pty;
};


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

    pj_status_t mediaPortGetFrame(pjmedia_port *med_port, pjmedia_frame *put_frame);
    pj_status_t mediaPortPutFrame(pjmedia_port *med_port, pjmedia_frame *put_frame);


  protected:


  private:

    std::string               m_username;
    std::string               m_password;
    std::string               m_sipserver;
    std::string               m_sipextension;
    std::string               m_schema;
    Async::AudioPassthrough*  m_logic_con_in;
    Async::AudioSource*       m_logic_con_out;
    Async::AudioPassthrough*  m_out_src;
    Async::AudioDecoder*      m_dec;
    Async::AudioReader*       m_ar;
    //Async::AudioSource*       m_src_in;
    Async::AudioPassthrough*  m_out_pt;
    uint16_t                  m_siploglevel;
    bool                      m_autoanswer;
    std::string               m_autoconnect;
    uint16_t                  m_sip_port;
    pj::Endpoint              ep;
    sip::_Account             *acc;
    std::vector<sip::_Call *> calls;
    Async::Timer              m_heartbeat_timer;
    Async::Timer              m_flush_timeout_timer;
    uint16_t                  m_reg_timeout;
    std::string               m_callername;

    Async::Timer              m_reconnect_timer;
    struct timeval            m_last_talker_timestamp;
    Async::Pty                *dtmf_ctrl_pty;
    uint16_t                  m_calltimeout;
    sip::_AudioMedia          *sip_buf;
    Async::Timer              m_call_timeout_timer;

    struct smpl
    {
      uint16_t               *sample_buf;
      int                    count;
    };
    smpl                     outsample;

    sip::_AudioMedia         *media;

    SipLogic(const SipLogic&);
    SipLogic& operator=(const SipLogic&);
    void makeCall(sip::_Account *acc, std::string dest_uri);
    void onIncomingCall(sip::_Account *acc, pj::OnIncomingCallParam &iprm);
    void onRegState(sip::_Account *acc, pj::OnRegStateParam &prm);
    bool setAudioCodec(const std::string& codec_name);
    void onDtmfDigit(sip::_Call *call, pj::OnDtmfDigitParam &prm);
    void onCallState(sip::_Call *call, pj::OnCallStateParam &prm);
    void hangupCalls(std::vector<sip::_Call *> calls);
    void hangupCall(sip::_Call *call);
    void dtmfCtrlPtyCmdReceived(const void *buf, size_t count);
    void onMediaState(sip::_Call *call, pj::OnCallMediaStateParam &prm);
    void sendAudio(const void *buf, int count);
    void allSamplesFlushed(void);
    void flushAudio(void);
    void callTimeout(Async::Timer *t=0);
    void flushTimeout(Async::Timer *t=0);

};  /* class SipLogic */


#endif /* SIP_LOGIC_INCLUDED */


/*
 * This file has not been truncated
 */
