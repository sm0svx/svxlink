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
#include <AsyncSigCAudioSink.h>
#include <AsyncPty.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDecimator.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SipLogic.h"
#include "multirate_filter_coeff.h"


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
      _Account() {}

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

  class _AudioMedia : public pj::AudioMedia
  {
    public:
      _AudioMedia(SipLogic &logic, int frameTimeLength)
        : slogic(logic)
      {
        createMediaPort(frameTimeLength);
        registerMediaPort(&mediaPort);
      }

      ~_AudioMedia()
      {
        unregisterMediaPort();
      }


    private:
      SipLogic &slogic;
      pjmedia_port mediaPort;


      static pj_status_t callback_getFrame(pjmedia_port *port, pjmedia_frame *frame)
      {
        SipLogic *slogic = static_cast<SipLogic *>(port->port_data.pdata);
        return slogic->mediaPortGetFrame(port, frame);
      } /* callback_getFrame */

      static pj_status_t callback_putFrame(pjmedia_port *port, pjmedia_frame *frame)
      {
        SipLogic *slogic = static_cast<SipLogic *>(port->port_data.pdata);
        return slogic->mediaPortPutFrame(port, frame);
      } /* callback_putFrame */


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

        mediaPort.port_data.pdata = &slogic;
        mediaPort.get_frame = &callback_getFrame;
        mediaPort.put_frame = &callback_putFrame;

      /* if (pjmedia_port_put_frame(&mediaPort, put_frame) == PJ_SUCCESS)
         {
           put_frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
           uint8_t *samples = reinterpret_cast<uint8_t*>(put_frame->buf);
         }
      */
      } /* _AudioMedia::createMediaPort */
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
    m_out_src(0), m_siploglevel(0), m_autoanswer(false),
    m_autoconnect(""), m_sip_port(5060),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_reg_timeout(300), m_callername("SvxLink"), dtmf_ctrl_pty(0),
    m_calltimeout(45), m_call_timeout_timer(45000, Timer::TYPE_ONESHOT, false)
{
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &SipLogic::flushTimeout));
  m_call_timeout_timer.expired.connect(
      mem_fun(*this, &SipLogic::callTimeout));
} /* SipLogic::SipLogic */


SipLogic::~SipLogic(void)
{
  delete m_logic_con_in;
  m_logic_con_in = 0;
  delete m_logic_con_out;
  m_logic_con_out = 0;
  delete m_out_src;
  m_out_src = 0;
  delete acc;
  delete dtmf_ctrl_pty;
  delete media;
  media = 0;
  for (std::vector<sip::_Call *>::iterator it=calls.begin();
          it != calls.end(); it++)
  {
    calls.erase(it);
    *it = 0;
  }
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

  cfg().getValue(name(), "AUTOANSWER", m_autoanswer); // auto pickup the call
  cfg().getValue(name(), "AUTOCONNECT", m_autoconnect); // auto connect a number
  cfg().getValue(name(), "CALLERNAME", m_callername);
  cfg().getValue(name(), "SIP_LOGLEVEL", m_siploglevel); // 0-6
  if (m_siploglevel < 0 || m_siploglevel > 6) m_siploglevel = 3;

  cfg().getValue(name(), "SIPPORT", m_sip_port); // SIP udp-port default: 5060
  cfg().getValue(name(), "REG_TIMEOUT", m_reg_timeout);
  if (m_reg_timeout < 60 || m_reg_timeout > 1000) m_reg_timeout = 300;

  cfg().getValue(name(), "CALL_TIMEOUT", m_calltimeout);
  if (m_calltimeout < 5 || m_calltimeout > 100) m_calltimeout = 45;
  m_call_timeout_timer.setTimeout(m_calltimeout * 1000);

   // create SipEndpoint - init library
  ep.libCreate();
  pj::EpConfig ep_cfg;
  ep_cfg.logConfig.level = m_siploglevel; // set the debug level of pj
  pj_log_set_level(m_siploglevel);
  ep.libInit(ep_cfg);
  ep.audDevManager().setNullDev(); // do not init a hw audio device

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
  acc_cfg.regConfig.timeoutSec = m_reg_timeout;

  acc_cfg.sipConfig.authCreds.push_back(AuthCredInfo(
                      m_schema, "*", m_username, 0, m_password));

  acc = new sip::_Account;
  try {
    acc->create(acc_cfg);
  } catch (Error& err) {
    cout << "*** ERROR: creating account: " << acc_cfg.idUri << endl;
    return false;
  }

   // sigc-callbacks in case of incoming call or registration change
  acc->onCall.connect(mem_fun(*this, &SipLogic::onIncomingCall));
  acc->onState.connect(mem_fun(*this, &SipLogic::onRegState));

  media = new sip::_AudioMedia(*this, 60);

  /****************************************************/

  AudioSource *prev_src = m_out_src;

   // incoming sip audio to this logic
  m_logic_con_in = new Async::AudioPassthrough;

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

  /****************************************************/

  AudioSink *m_in_src = 0;

    // adapt the differe sample rates SvxLink<->Sip (16k<->8k)
  if (INTERNAL_SAMPLE_RATE == 16000)
  {
      // SipLogic -> SvxLink core
    AudioInterpolator *i1 = new AudioInterpolator(2, coeff_16_8,
                                                  coeff_16_8_taps);
    prev_src->registerSink(i1, true);
    prev_src = i1;

      // SvxLink core -> SipLogic
    AudioDecimator *d2 = new AudioDecimator(2, coeff_16_8, coeff_16_8_taps);
    m_in_src->registerSource(d2);
    m_in_src = d2;
  }

  SigCAudioSink *as = m_in_src;


  m_logic_con_in->registerSink(m_in_src, false);

  m_logic_con_out = prev_src;

   // init this Logic
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
  sip::_Call *call = new sip::_Call(*acc);

  try {
    call->makeCall(dest_uri, prm);
    calls.push_back(call);
    m_call_timeout_timer.setEnable(true);
  } catch (Error& err) {
    cout << "*** ERROR: " << err.info() << endl;
  }
} /* SipLogic::makeCall */


void SipLogic::onIncomingCall(sip::_Account *acc, pj::OnIncomingCallParam &iprm)
{
  sip::_Call *call = new sip::_Call(*acc, iprm.callId);
  pj::CallInfo ci = call->getInfo();
  pj::CallOpParam prm;
  prm.opt.audioCount = 1;
  prm.opt.videoCount = 0;

  cout << "+++ Incoming Call: " <<  ci.remoteUri << " ["
            << ci.remoteContact << "]" << endl;

  calls.push_back(call);
  prm.statusCode = (pjsip_status_code)200;
  if (m_autoanswer)
  {
    call->answer(prm);
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
    if (ci.media[0].type == PJMEDIA_TYPE_AUDIO)
    {
      sip_buf = static_cast<sip::_AudioMedia *>(call->getMedia(0));
      sip_buf->startTransmit(*media);
      media->startTransmit(*sip_buf);
    }
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_NONE)
  {
    cout << "+++ Call currently has no media, or the media is not used."
         << endl;
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_LOCAL_HOLD)
  {
    cout << "+++ The media is currently put on hold by local endpoint."
         << endl;
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_REMOTE_HOLD)
  {
    cout << "+++ The media is currently put on hold by remote endpoint."
         << endl;
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_ERROR)
  {
    cout << "*** ERROR: The Sip audio media has reported an error." << endl;
  }
} /* SipLogic::onMediaState */


void SipLogic::audioStreamStateChange(bool is_active, bool is_idle)
{
} /* */


/*
 * store SvxLink audio stream into buffer
 */
int SipLogic::sendAudio(void *buf, int count)
{
  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }

  int pos = outsample.count;
  memcpy(outsample.sample_buf + pos, buf, count);
  outsample.count += count;
  return count;
} /* SipLogic::sendAudio */


/*
 * incoming SvxLink audio stream to SIP
 */
pj_status_t SipLogic::mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame)
{
  int count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
  int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
  frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

  if (outsample.count > count)
  {
    memcpy(samples, outsample.sample_buf, count);
    memmove(outsample.sample_buf, outsample.sample_buf + count, outsample.count - count);
    outsample.count -= count;
  }
  else
  {
    memcpy(samples, outsample.sample_buf, outsample.count);
    for (int i=outsample.count; i < count; i++)
    {
      samples[i] = 0;
    }
    outsample.count = 0;
  }

  return PJ_SUCCESS;
} /* SipLogic::mediaPortGetFrame */


/*
 * incoming SIP audio to SvxLink
 */
pj_status_t SipLogic::mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame)
{
  int count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
  float *samples = static_cast<float *>(frame->buf);
  frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

  m_out_src->writeSamples(samples, count);
  return PJ_SUCCESS;
} /* SipLogic::mediaPortPutFrame */


void SipLogic::onCallState(sip::_Call *call, pj::OnCallStateParam &prm)
{
  pj::CallInfo ci = call->getInfo();

  for (std::vector<sip::_Call *>::iterator it=calls.begin();
          it != calls.end(); it++)
  {
    if (*it == call)
    {
       // call disconnected
      if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
      {
        cout << "+++ call disconnected, duration "
             << (*it)->getInfo().totalDuration.sec << "."
             << (*it)->getInfo().totalDuration.msec << " secs" << endl;
        m_out_src->allSamplesFlushed();
        calls.erase(it);
      }

       // incoming call
      if (ci.state == PJSIP_INV_STATE_INCOMING)
      {
        cout << "+++ incoming call" << endl;
      }

       // connecting
      if (ci.state == PJSIP_INV_STATE_CONNECTING)
      {
        cout << "+++ connecting" << endl;
      }

       // calling
      if (ci.state == PJSIP_INV_STATE_CALLING)
      {
        cout << "+++ calling" << endl;
      }

      break;
    }
  }
} /* SipLogic::onCallState */


void SipLogic::onDtmfDigit(sip::_Call *call, pj::OnDtmfDigitParam &prm)
{
  cout << "+++ Dtmf digit received: " << prm.digit
       << " code=" << call->getId() << endl;
} /* SipLogic::onDtmfDigit */


void SipLogic::onRegState(sip::_Account *acc, pj::OnRegStateParam &prm)
{
  pj::AccountInfo ai = acc->getInfo();
  std::cout << "+++ " << (ai.regIsActive ? "Registered code=" :
            "Unregistered code=") << prm.code << std::endl;
} /* SipLogic::onRegState */


// hangup all calls
void SipLogic::hangupCalls(std::vector<sip::_Call *> calls)
{
  m_out_src->allSamplesFlushed();

  CallOpParam prm(true);

  for (std::vector<sip::_Call *>::iterator it=calls.begin();
       it != calls.end(); it++)
  {
    cout << "+++ hangup call " << (*it)->getInfo().remoteUri
         << ", duration " << (*it)->getInfo().totalDuration.sec
         << " secs" << endl;
    (*it)->hangup(prm);
    calls.erase(it);
  }
} /* SipLogic::hangupCalls */


// hangup a single call
void SipLogic::hangupCall(sip::_Call *call)
{
  CallOpParam prm(true);

  for (std::vector<sip::_Call *>::iterator it=calls.begin();
       it != calls.end(); it++)
  {
    if (*it == call)
    {
      m_out_src->allSamplesFlushed();
      cout << "+++ hangup call " << (*it)->getInfo().remoteUri
           << ", duration " << (*it)->getInfo().totalDuration.sec
           << " secs" << endl;
      (*it)->hangup(prm);
      calls.erase(it);
      break;
    }
  }
}


/**
 *  dial-out by sending a string over Pty device, e.g.
 *  echo "C12345#" > /tmp/sippty
 *  method converts it to a valid dial string:
 *  "sip:12345@sipserver"
**/
void SipLogic::dtmfCtrlPtyCmdReceived(const void *buf, size_t count)
{
  string m_dtmf_incoming = reinterpret_cast<const char*>(buf);

  if (acc != 0)
  {
     // hanging up all calls with "C#"
    if (m_dtmf_incoming == "C#")
    {
      hangupCalls(calls);
    }

     // calling a party with "C12345#"
    if (m_dtmf_incoming[0] == 'C' && count > 3)
    {
      // "C12345#" -> sip:12345@sipserver.de
      string tocall = "sip:";
      tocall += m_dtmf_incoming.substr(1, count-3);
      tocall += "@";
      tocall += m_sipserver;
      makeCall(acc, tocall);
      m_call_timeout_timer.setEnable(true);
    }
  }
} /* SipLogic::dtmfCtrlPtyCmdReceived */


void SipLogic::flushAudio(void)
{
  m_flush_timeout_timer.setEnable(true);
} /* SipLogic::flushAudio */


void SipLogic::callTimeout(Async::Timer *t)
{
  cout << "+++ called party is not at home." << endl;
  m_call_timeout_timer.setEnable(false);
  m_call_timeout_timer.reset();
} /* SipLogic::flushTimeout */


void SipLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_out_src->allSamplesFlushed();
} /* SipLogic::flushTimeout */

/*
 * This file has not been truncated
 */
