/**
@file	 SipLogic.cpp
@brief   A logic core that connect a Sip server e.g. Asterisk
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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <iterator>
#include <sigc++/sigc++.h>
#include <pjlib.h>
#include <pjsua-lib/pjsua.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioPassthrough.h>
#include <AsyncPty.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SipLogic.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace pj;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

namespace sip {

  class _Call : public pj::Call, public sigc::trackable
  {
    public:
      _Call(pj::Account &acc, int call_id = PJSUA_INVALID_ID)
      : pj::Call(acc, call_id), account(acc) {}

      virtual void onCallMediaState(pj::OnCallMediaStateParam &prm)
      {
        onMedia(this, prm);
      }

      virtual void onDtmfDigit(pj::OnDtmfDigitParam &prm)
      {
        onDtmf(this, prm);
      }

      virtual void onCallState(pj::OnCallStateParam &prm)
      {
        onCall(this, prm);
      }

      /**
       * This structure contains parameters for Call::onDtmfDigit() callback.
         string pj::OnDtmfDigitParam::digit
         DTMF ASCII digit.
       */
      sigc::signal<void, sip::_Call*, pj::OnDtmfDigitParam&> onDtmf;

      /**
       * This structure contains parameters for Call::onCallMediaState() callback.
       */
      sigc::signal<void, sip::_Call*, pj::OnCallMediaStateParam&> onMedia;

      /**
       * This structure contains parameters for Call::onCallState() callback.
       **/
      sigc::signal<void, sip::_Call*, pj::OnCallStateParam&> onCall;

    private:
      pj::Account &account;
  };

  class _Account : public pj::Account, public sigc::trackable
  {

    public:
      _Account() {};

      /**
       * Signal when registration state was changed
       */
      virtual void onRegState(pj::OnRegStateParam &prm)
      {
        onState(this, prm);
      }

      /**
       * Signal on an incoming call
       */
      virtual void onIncomingCall(pj::OnIncomingCallParam &iprm)
      {
        onCall(this, iprm);
      }

      /**
       * the sigc++ signal on registration change
       */
      sigc::signal<void, sip::_Account*, pj::OnRegStateParam&> onState;

      /**
       * the sigc++ signal on incoming call
       */
      sigc::signal<void, sip::_Account*, pj::OnIncomingCallParam&> onCall;

    private:
      friend class _Call;
  };

  class _AudioMedia : public pj::AudioMedia, public sigc::trackable
  {
    public:
      _AudioMedia(int frameTimeLength)
      {
        createMediaPort(frameTimeLength);
        registerMediaPort(&mediaPort);
      }

      ~_AudioMedia()
      {
        unregisterMediaPort();
      }

      sigc::signal<void, pjmedia_port, pjmedia_frame> callback_putFrame;

      sigc::signal<void, pjmedia_port, pjmedia_frame> callback_getFrame;


    private:
      pjmedia_port mediaPort;
      pjmedia_frame *get_frame;
      pjmedia_frame *put_frame;

/*    static pj_status_t callback_getFrame(pjmedia_port *port, pjmedia_frame *frame)
      {
        auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
        return communicator->mediaPortGetFrame(port, frame);
      }

      static pj_status_t callback_putFrame(pjmedia_port *port, pjmedia_frame *frame)
      {
        auto *communicator = static_cast<sip::PjsuaCommunicator *>(port->port_data.pdata);
        return communicator->mediaPortPutFrame(port, frame);
      }
*/

      void createMediaPort(int frameTimeLength)
      {
        pj_str_t name = pj_str((char *) "SvxLinkMediaPort");

        pj_status_t status = pjmedia_port_info_init(&(mediaPort.info),
                             &name,
                             PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                             INTERNAL_SAMPLE_RATE,
                             1,
                             16,
                             INTERNAL_SAMPLE_RATE * frameTimeLength / 1000);

        if (status != PJ_SUCCESS)
        {
          std::cout << "*** ERROR: while calling pjmedia_port_info_init()"
                    << std::endl;
        }

        if (pjmedia_port_get_frame(&mediaPort, get_frame) == PJ_SUCCESS)
        {
          callback_getFrame(mediaPort, *get_frame);
        }

        if (pjmedia_port_put_frame(&mediaPort, put_frame) == PJ_SUCCESS)
        {
          callback_putFrame(mediaPort, *put_frame);
        }
      }

  };
}


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SipLogic::SipLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_logic_con_in(0), m_logic_con_out(0),
    m_dec(0), m_enc(0), m_siploglevel(0), m_autoanswer(false),
    m_autoconnect(""), m_sip_port(5060),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_reg_timeout(500), m_callername("SvxLink"), dtmf_ctrl_pty(0),
    m_calltimeout(45)
{
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &SipLogic::flushTimeout));
} /* SipLogic::SipLogic */


SipLogic::~SipLogic(void)
{
  delete m_logic_con_in;
  m_logic_con_in = 0;
  delete m_logic_con_out;
  m_logic_con_out = 0;
  delete m_enc;
  m_enc = 0;
  delete m_dec;
  m_dec = 0;
  delete acc;
  delete dtmf_ctrl_pty;
  dtmf_ctrl_pty = 0;
  ep.libDestroy();
} /* SipLogic::~SipLogic */


bool SipLogic::initialize(void)
{
  if (!cfg().getValue(name(), "USERNAME", m_username))
  {
    cerr << "*** ERROR: " << name() << "/USERNAME missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "PASSWORD", m_password))
  {
    cerr << "*** ERROR: " << name() << "/PASSWORD missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "SIPSERVER", m_sipserver))
  {
    cerr << "*** ERROR: " << name() << "/SIPSERVER missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "SIPEXTENSION", m_sipextension))
  {
    cerr << "*** ERROR: " << name() << "/SIPEXTENSION missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "SIPSCHEMA", m_schema))
  {
    cerr << "*** ERROR: " << name() << "/SIPSCHEMA missing in configuration"
         << endl;
    return false;
  }

  std::string dtmf_ctrl_pty_path;
  cfg().getValue(name(), "DTMF_CTRL_PTY", dtmf_ctrl_pty_path);

  if (!dtmf_ctrl_pty_path.empty())
  {
    dtmf_ctrl_pty = new Async::Pty(dtmf_ctrl_pty_path);
    if (!dtmf_ctrl_pty->open())
    {
      cerr << "*** ERROR: Could not open dtmf sip PTY " << dtmf_ctrl_pty_path 
           << " as specified in configuration variable " << name()
           << "/" << "DTMF_CTRL_PTY" << endl;
      return false;
    }
    dtmf_ctrl_pty->dataReceived.connect(
               mem_fun(*this, &SipLogic::dtmfCtrlPtyCmdReceived));
  }

  cfg().getValue(name(), "AUTOANSWER", m_autoanswer);
  cfg().getValue(name(), "AUTOCONNECT", m_autoconnect);
  cfg().getValue(name(), "CALLERNAME", m_callername);
  cfg().getValue(name(), "SIP_LOGLEVEL", m_siploglevel);
  cfg().getValue(name(), "SIPPORT", m_sip_port);
  cfg().getValue(name(), "REG_TIMEOUT", m_reg_timeout);
  cfg().getValue(name(), "CALL_TIMEOUT", m_calltimeout);

   // create SipEndpoint - init library
  ep.libCreate();
  pj::EpConfig ep_cfg;
  ep_cfg.logConfig.level = m_siploglevel;
  ep.libInit(ep_cfg);

   // Transport
  TransportConfig tcfg;
  tcfg.port = m_sip_port;
  ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
  ep.libStart();

   // add SipAccount
  AccountConfig acc_cfg;
  acc_cfg.idUri = "\"";
  acc_cfg.idUri += m_callername;
  acc_cfg.idUri += "\"<sip:";
  acc_cfg.idUri += m_username;
  acc_cfg.idUri += "@";
  acc_cfg.idUri += m_sipserver;
  acc_cfg.idUri += ">";
  acc_cfg.regConfig.registrarUri = "sip:";
  acc_cfg.regConfig.registrarUri += m_sipserver;
/*
  acc_cfg.regConfig.firstRetryIntervalSec = 5;
  acc_cfg.regConfig.retryIntervalSec = 30;
  acc_cfg.regConfig.registerOnAdd = PJ_TRUE;
*/

  acc_cfg.sipConfig.authCreds.push_back(AuthCredInfo(
                      m_schema, "*", m_username, 0, m_password));

  acc = new sip::_Account;
  try {
    acc->create(acc_cfg);
  } catch (Error& err) {
    std::cout << "*** ERROR: creating account: " << acc_cfg.idUri
       << std::endl;
    return false;
  }

   // sigc-callbacks in case of incoming call or registration change
  acc->onCall.connect(mem_fun(*this, &SipLogic::onIncomingCall));
  acc->onState.connect(mem_fun(*this, &SipLogic::onRegState));

   // Create logic connection incoming audio passthrough
  m_logic_con_in = new Async::AudioPassthrough;

    // Create dummy audio codec used before setting the real encoder
  if (!setAudioCodec("DUMMY")) { return false; }
  AudioSource *prev_src = m_dec;

   // Create jitter FIFO if jitter buffer delay > 0
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    AudioFifo *fifo = new Async::AudioFifo(
        2 * jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
        //new Async::AudioJitterFifo(100 * INTERNAL_SAMPLE_RATE / 1000);
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
    prev_src->registerSink(fifo, true);
    prev_src = fifo;
  }
  else
  {
    AudioPassthrough *passthrough = new AudioPassthrough;
    prev_src->registerSink(passthrough, true);
    prev_src = passthrough;
  }
  m_logic_con_out = prev_src;

  if (!LogicBase::initialize())
  {
    cout << "*** ERROR initializing SipLogic" << endl;
    return false;
  }

   // auto create an outgoing call
  if (m_autoconnect.length() > 0)
  {
    makeCall(acc, m_autoconnect);
  }

  return true;
} /* SipLogic::initialize */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void SipLogic::makeCall(sip::_Account *acc, std::string dest_uri)
{
  cout << "+++ Calling \"" << dest_uri << "\"" << endl;

  CallOpParam prm(true);
  prm.opt.audioCount = 1;
  prm.opt.videoCount = 0;
  Call *call = new sip::_Call(*acc);

  try {
    call->makeCall(dest_uri, prm);
    calls.push_back(call);
  } catch (Error& err) {
    cout << "*** ERROR: " << err.info() << endl;
  }
} /* SipLogic::makeCall */


void SipLogic::onIncomingCall(sip::_Account *acc, pj::OnIncomingCallParam &iprm)
{
  sip::_Call *call = new sip::_Call(*acc, iprm.callId);
  pj::CallInfo ci = call->getInfo();
  pj::CallOpParam prm;

  std::cout << "+++ Incoming Call: " <<  ci.remoteUri << " ["
            << ci.stateText << "]" << std::endl;

  calls.push_back(call);
  prm.statusCode = (pjsip_status_code)200;
  if (m_autoanswer)
  {
    call->answer(prm);
    std::cout << "### auto answer call" << std::endl;
    call->onDtmf.connect(mem_fun(*this, &SipLogic::onDtmfDigit));
    call->onMedia.connect(mem_fun(*this, &SipLogic::onMediaState));
    call->onCall.connect(mem_fun(*this, &SipLogic::onCallState));
  }
} /* SipLogic::onIncomingCall */


void SipLogic::onMediaState(sip::_Call *call, pj::OnCallMediaStateParam &prm)
{
  pj::CallInfo ci = call->getInfo();

  if (ci.media.size() != 1)
  {
    cout << "*** ERROR: media size not 1" << endl;
    return;
  }

  if (ci.media[0].status == PJSUA_CALL_MEDIA_ACTIVE)
  {
    sip::_AudioMedia *aud_med=static_cast<sip::_AudioMedia *>(call->getMedia(0));
    aud_med->callback_putFrame.connect(mem_fun(*this, &SipLogic::sipWriteSamples));
//    aud_med->callback_getFrame.connect(mem_fun(*this, &SipLogic::sipGetSamples));
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_NONE)
  {
    // toDo
  }
} /* SipLogic::onMediaState */


void SipLogic::sipWriteSamples(pjmedia_port med_port, pjmedia_frame put_frame)
{

} /* SipLogic::sipWriteSamples */


void SipLogic::onCallState(sip::_Call *call, pj::OnCallStateParam &prm)
{
  pj::CallInfo ci = call->getInfo();
  if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
  {
    for (pj::vector<Call *>::iterator it=calls.begin();
          it != calls.end(); it++)
    {
      if (*it == call)
      {
        cout << "+++ call disconnected\n" << endl;
        m_enc->allEncodedSamplesFlushed();
        calls.erase(it);
        break;
      }
    }
  }
} /* SipLogic::onCallState */


bool SipLogic::setAudioCodec(const std::string& codec_name)
{
  delete m_enc;
  m_enc = Async::AudioEncoder::create(codec_name);
  if (m_enc == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize " << codec_name
         << " audio encoder" << endl;
    m_enc = Async::AudioEncoder::create("DUMMY");
    assert(m_enc != 0);
    return false;
  }
  m_enc->writeEncodedSamples.connect(
      mem_fun(*this, &SipLogic::sendEncodedAudio));
  m_enc->flushEncodedSamples.connect(
      mem_fun(*this, &SipLogic::flushEncodedAudio));
  m_logic_con_in->registerSink(m_enc, false);

  AudioSink *sink = 0;
  if (m_dec != 0)
  {
    sink = m_dec->sink();
    m_dec->unregisterSink();
    delete m_dec;
  }
  m_dec = Async::AudioDecoder::create(codec_name);
  if (m_dec == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize " << codec_name
         << " audio decoder" << endl;
    m_dec = Async::AudioDecoder::create("DUMMY");
    assert(m_dec != 0);
    return false;
  }
  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &SipLogic::allEncodedSamplesFlushed));
  if (sink != 0)
  {
    m_dec->registerSink(sink, true);
  }

  return true;
} /* SipLogic::setAudioCodec */


void SipLogic::onDtmfDigit(sip::_Call *call, pj::OnDtmfDigitParam &prm)
{
  std::cout << "+++ Dtmf digit received: " << prm.digit << std::endl;
} /* SipLogic::onDtmfDigit */


void SipLogic::onRegState(sip::_Account *acc, pj::OnRegStateParam &prm)
{
  pj::AccountInfo ai = acc->getInfo();
  std::cout << ">>>>> " << (ai.regIsActive ? "Register: code=" :
            "Unregister: code=") << prm.code << std::endl;
} /* SipLogic::onRegState */


void SipLogic::dtmfCtrlPtyCmdReceived(const void *buf, size_t count)
{
  string m_dtmf_incoming = reinterpret_cast<const char*>(buf);
  if (m_dtmf_incoming[0] == 'F' && count > 3)
  {
    if (acc != 0)
    {
      makeCall(acc, m_dtmf_incoming.substr(1, count-1));
    }
  }
} /* SipLogic::dtmfCtrlPtyCmdReceived */


void SipLogic::sendEncodedAudio(const void *buf, int count)
{
  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }
} /* SipLogic::sendEncodedAudio */


void SipLogic::flushEncodedAudio(void)
{
  m_flush_timeout_timer.setEnable(true);
} /* SipLogic::flushEncodedAudio */


void SipLogic::allEncodedSamplesFlushed(void)
{
} /* SipLogic::allEncodedSamplesFlushed */


void SipLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_enc->allEncodedSamplesFlushed();
} /* SipLogic::flushTimeout */

/*
 * This file has not been truncated
 */
