/**
@file	 SipLogic.cpp
@brief   A logic core that connect a Sip server e.g. Asterisk
@author  Tobias Blomberg / SM0SVX & Christian Stussak & Adi Bier / DL1HRC
@date	 2018-02-12

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
#include <AsyncAudioValve.h>
#include <AsyncAudioReader.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioClipper.h>
#include <AsyncAudioCompressor.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioFilter.h>
#include <AsyncTcpClient.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "EventHandler.h"
#include "MsgHandler.h"
#include "SipLogic.h"
#include "SquelchVox.h"
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
#define DEFAULT_SIPLIMITER_THRESH  -1.0
#define PJSIP_VERSION "14032023"


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

      virtual void onInstantMessage(pj::OnInstantMessageParam &prm)
      {
        onMessage(this, prm);
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

      /**
       * This structure contains parameters for Call::onMessage() callback
       **/
       sigc::signal<void, sip::_Call*, pj::OnInstantMessageParam&> onMessage;

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
       * Signal on an incoming message
       */
      virtual void onInstandMessage(pj::OnInstantMessageParam &imprm)
      {
        onMessage(this, imprm);
      }

      /**
       * the sigc++ signal on registration change
       */
      sigc::signal<void, sip::_Account*, pj::OnRegStateParam&> onState;

      /**
       * the sigc++ signal on incoming call
       */
      sigc::signal<void, sip::_Account*, pj::OnIncomingCallParam&> onCall;

      /**
       * the sigc++ signal on incoming message
       */
      sigc::signal<void, sip::_Account*, pj::OnInstantMessageParam&> onMessage;

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
                             //PJMEDIA_SIG_CLASS_PORT_AUD('s', 'i'),
                             PJMEDIA_SIG_PORT_STREAM,
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
 * Exported Global functions
 *
 ****************************************************************************/

extern "C" {
  LogicBase* construct(void) { return new SipLogic; }
}


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

SipLogic::SipLogic(void)
  : m_logic_con_out(0),
    m_outto_sip(0), m_infrom_sip(0), m_autoanswer(false),
    m_sip_port(5060), dtmf_ctrl_pty(0),
    m_call_timeout_timer(45000, Timer::TYPE_ONESHOT, false),
    squelch_det(0), accept_incoming_regex(0), reject_incoming_regex(0),
    accept_outgoing_regex(0), reject_outgoing_regex(0),
    logic_msg_handler(0), logic_event_handler(0), report_events_as_idle(false),
    startup_finished(false), logicselector(0), semi_duplex(false),
    sip_preamp_gain(0), m_autoconnect(""), sip_event_handler(0), 
    sip_msg_handler(0), sipselector(0), m_siploglevel(0), ignore_reg(false)
{
  m_call_timeout_timer.expired.connect(
      mem_fun(*this, &SipLogic::callTimeout));
} /* SipLogic::SipLogic */


bool SipLogic::initialize(Async::Config& cfgobj, const std::string& logic_name)
{
   // Create the AudioSelector for Sip and message audio streams
  m_logic_con_out = new Async::AudioSelector;

   // Handler for audio stream from logic to sip
  m_logic_con_in = new Async::AudioPassthrough;

   // Init this LogicBase
  if (!LogicBase::initialize(cfgobj, logic_name))
  {
    cout << name() << ":*** ERROR initializing SipLogic." << endl;
    return false;
  }

  std::string m_username;
  if (!cfg().getValue(name(), "USERNAME", m_username))
  {
    cerr << "*** ERROR: " << name() << "/USERNAME missing in configuration"
         << endl;
    return false;
  }

  std::string m_password;
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

  std::string m_sipextension;
  if (!cfg().getValue(name(), "SIPEXTENSION", m_sipextension))
  {
    cerr << "*** ERROR: " << name() << "/SIPEXTENSION missing in configuration"
         << endl;
    return false;
  }

  std::string m_schema;
  if (!cfg().getValue(name(), "SIPSCHEMA", m_schema))
  {
    cerr << "*** ERROR: " << name() << "/SIPSCHEMA missing in configuration"
         << endl;
    return false;
  }

  cfg().getValue(name(), "SIPPORT", m_sip_port); // SIP udp-port default: 5060

  std::string m_sipregistrar;
  cfg().getValue(name(), "SIPREGISTRAR", m_sipregistrar);
  std::string t_sreg = ":";
  t_sreg += to_string(m_sip_port);

  if (m_sip_port != 5060 && (m_sipregistrar.find(t_sreg) == std::string::npos))
  {
    cout << "+++ WARNING: The SIPPORT is not the default (5060), so the param "
         << "SIPREGISTRAR should be configured as 'SIPREGISTRAR=" 
         << m_sipregistrar << ":" << m_sip_port << "'." << endl;
  }

  std::string dtmf_ctrl_pty_path;
  cfg().getValue(name(), "SIP_CTRL_PTY", dtmf_ctrl_pty_path);

  if (!dtmf_ctrl_pty_path.empty())
  {
    dtmf_ctrl_pty = new Async::Pty(dtmf_ctrl_pty_path);
    if (!dtmf_ctrl_pty->open())
    {
      cerr << "*** ERROR: Could not open dtmf sip PTY " << dtmf_ctrl_pty_path
           << " as specified in configuration variable " << name()
           << "/" << "SIP_CTRL_PTY" << endl;
      return false;
    }
    dtmf_ctrl_pty->dataReceived.connect(
               mem_fun(*this, &SipLogic::dtmfCtrlPtyCmdReceived));
  }

  cfg().getValue(name(), "AUTOANSWER", m_autoanswer); // auto pickup the call

  cfg().getValue(name(), "SEMI_DUPLEX", semi_duplex); // only for RepeaterLogics

  if (cfg().getValue(name(), "AUTOCONNECT", m_autoconnect)) // auto connect a number
  {
    size_t pos = m_autoconnect.find("sip:");
    size_t pos2 = m_autoconnect.find('@');
    if (pos == std::string::npos || pos2 == std::string::npos)
    {
      cout << name()
           << ": *** WARNING AUTOCONNECT=" << m_autoconnect
           << " is incorrect. It must begin with \"sip:\" and have "
           << "an uri, e.g.:\n\"AUTOCONNECT=sip:1234567@sipserver.com\"\n"
           << "*** Autoconnect failed! ***\n" << endl;
      m_autoconnect = "";
    }
  }

  std::string m_callername("SvxLink");
  cfg().getValue(name(), "CALLERNAME", m_callername);

  cfg().getValue(name(), "SIP_LOGLEVEL", m_siploglevel); // 0-6
  if (m_siploglevel < 0 || m_siploglevel > 6) m_siploglevel = 3;

  uint16_t m_reg_timeout = 300;
  cfg().getValue(name(), "REG_TIMEOUT", m_reg_timeout);
  if (m_reg_timeout < 60 || m_reg_timeout > 1000) m_reg_timeout = 300;

  uint16_t m_calltimeout = 45;
  cfg().getValue(name(), "CALL_TIMEOUT", m_calltimeout);
  if (m_calltimeout < 5 || m_calltimeout > 100) m_calltimeout = 45;
  m_call_timeout_timer.setTimeout(m_calltimeout * 1000);

  std::string value;
  if (!cfg().getValue(name(), "ACCEPT_INCOMING", value))
  {
    value = "^.*$";
  }
  accept_incoming_regex = new regex_t;
  size_t err_size = 0;
  ((void)(err_size)); // Suppress warning about unused variable
  int err = regcomp(accept_incoming_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, accept_incoming_regex, 0, 0);
    char msg[msg_size];
    err_size = regerror(err, accept_incoming_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << name() << "/ACCEPT_INCOMING: "
         << msg << endl;
    return false;
  }

  if (!cfg().getValue(name(), "REJECT_INCOMING", value))
  {
    value = "^$";
  }
  reject_incoming_regex = new regex_t;
  err = regcomp(reject_incoming_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, reject_incoming_regex, 0, 0);
    char msg[msg_size];
    err_size = regerror(err, reject_incoming_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << name() << "/REJECT_INCOMING: "
         << msg << endl;
    return false;
  }

  if (!cfg().getValue(name(), "ACCEPT_OUTGOING", value))
  {
    value = "^.*$";
  }
  accept_outgoing_regex = new regex_t;
  err = regcomp(accept_outgoing_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, accept_outgoing_regex, 0, 0);
    char msg[msg_size];
    err_size = regerror(err, accept_outgoing_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << name() << "/ACCEPT_OUTGOING: "
         << msg << endl;
    return false;
  }

  if (!cfg().getValue(name(), "REJECT_OUTGOING", value))
  {
    value = "^$";
  }
  reject_outgoing_regex = new regex_t;
  err = regcomp(reject_outgoing_regex, value.c_str(),
                REG_EXTENDED | REG_NOSUB | REG_ICASE);
  if (err != 0)
  {
    size_t msg_size = regerror(err, reject_outgoing_regex, 0, 0);
    char msg[msg_size];
    err_size = regerror(err, reject_outgoing_regex, msg, msg_size);
    assert(err_size == msg_size);
    cerr << "*** ERROR: Syntax error in " << name() << "/REJECT_OUTGOING: "
         << msg << endl;
    return false;
  }

  // set Tg's depending on the incoming sip phone number
  std::string phonenumbers_to_tg;
  if (cfg().getValue(name(), "PHONENUMBERS_TO_TG", phonenumbers_to_tg))
  {
    vector<string> nrlist;
    SvxLink::splitStr(nrlist, phonenumbers_to_tg, ",");
    for (vector<string>::const_iterator nr_it = nrlist.begin();
       nr_it != nrlist.end(); nr_it++)
    {
      size_t pos;
      if ((pos = (*nr_it).find(':')) != string::npos)
      {
        std::string r = (*nr_it).substr(0, pos);
        uint32_t t = atoi((*nr_it).substr(pos+1, (*nr_it).length()-pos).c_str());
        phoneNrTgVec[r] = t;
      }
      else
      {
        cout << "+++ WARNING: Wrong format in param " << name()
             << "/PHONENUMBERS_TO_TG, ignoring." << endl;
      }
    }
  }

   // create SipEndpoint - init library
  try {
    pj::EpConfig ep_cfg;
    ep_cfg.logConfig.level = m_siploglevel; // set the debug level of pj
    pj_log_set_level(m_siploglevel);
    ep.libCreate();
    ep.libInit(ep_cfg);
    ep.audDevManager().setNullDev(); // do not init a hw audio device
  } catch (Error& err) {
    cout << "*** ERROR creating Sip transport layer in "
         << name() << endl;
    return false;
  }

    // use UDP or TCP?
  bool use_tcp = false;
  cfg().getValue(name(), "USE_TCP", use_tcp);

  // Sip transport layer creation
  try {
    TransportConfig tcfg;
    tcfg.port = m_sip_port;
    if (use_tcp)
    {
      ep.transportCreate(PJSIP_TRANSPORT_TCP, tcfg);
    }
    else
    {
      ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);
    }
    ep.libStart();
  } catch (Error& err) {
    cout << "*** ERROR creating transport layer in "
         << name() << endl;
    return false;
  }

   // add SipAccount
  AccountConfig acc_cfg;
  acc_cfg.idUri = "\"";
  acc_cfg.idUri += m_callername;
  acc_cfg.idUri += "\"<sip:";
  acc_cfg.idUri += m_username;
  acc_cfg.idUri += "@";
  acc_cfg.idUri += m_sipserver;
  if (use_tcp)
  {
    acc_cfg.idUri += ";transport=tcp";
  }
  acc_cfg.idUri += ">";
  acc_cfg.regConfig.registrarUri = "sip:";
  acc_cfg.regConfig.registrarUri += m_sipregistrar;
  acc_cfg.regConfig.timeoutSec = m_reg_timeout;
  acc_cfg.sipConfig.authCreds.push_back(AuthCredInfo(
                      m_schema, "*", m_username, 0, m_password));

  acc = new sip::_Account;
  try {
    acc->create(acc_cfg);
  } catch (Error& err) {
    cout << ":*** ERROR creating account: "
         << acc_cfg.idUri << " in " << name() << endl;
    return false;
  }

   // sigc-callbacks in case of incoming call or registration change
  acc->onCall.connect(mem_fun(*this, &SipLogic::onIncomingCall));
  acc->onState.connect(mem_fun(*this, &SipLogic::onRegState));
  acc->onMessage.connect(mem_fun(*this, &SipLogic::onMessage));

  // setup dns lookup method
  dns.setLookupParams(getCallerUri(m_sipserver));
  dns.resultsReady.connect(mem_fun(*this, &SipLogic::onDnsResultsReady));
  dns.lookup();

  cfg().getValue(name(), "IGNORE_REGISTRATION", ignore_reg);

   // number of samples = INTERNAL_SAMPLE_RATE * frameTimeLen /1000
  media = new sip::_AudioMedia(*this, 48);

  /*************** incoming from sip ****************************/
  // handler for incoming sip audio stream
  m_out_src = new AudioPassthrough;
  AudioSource *prev_src = m_out_src;

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

  cfg().getValue(name(), "SIP_PREAMP", sip_preamp_gain);
    // If a preamp was configured, create it
  if (sip_preamp_gain != 0)
  {
    AudioAmp *sippreamp = new AudioAmp;
    sippreamp->setGain(sip_preamp_gain);
    prev_src->registerSink(sippreamp, true);
    prev_src = sippreamp;
  }

  // Add a limiter to smoothly limit the audio before hard clipping it
  double limiter_thresh = DEFAULT_SIPLIMITER_THRESH;
  cfg().getValue(name(), "SIP_LIMITER_THRESH", limiter_thresh);
  if (limiter_thresh != 0.0)
  {
    AudioCompressor *limit = new AudioCompressor;
    limit->setThreshold(limiter_thresh);
    limit->setRatio(0.1);
    limit->setAttack(2);
    limit->setDecay(20);
    limit->setOutputGain(1);
    prev_src->registerSink(limit, true);
    prev_src = limit;
  }

    // Clip audio to limit its amplitude
  AudioClipper *clipper = new AudioClipper;
  clipper->setClipLevel(0.98);
  prev_src->registerSink(clipper, true);
  prev_src = clipper;

    // Remove high frequencies generated by the previous clipping
#if (INTERNAL_SAMPLE_RATE == 16000)
  AudioFilter *splatter_filter = new AudioFilter("LpCh9/-0.05/5000");
#else
  AudioFilter *splatter_filter = new AudioFilter("LpCh9/-0.05/3500");
#endif
  prev_src->registerSink(splatter_filter, true);
  prev_src = splatter_filter;

  AudioSplitter *splitter = new AudioSplitter;
  prev_src->registerSink(splitter, true);
  prev_src = splitter;

  m_infrom_sip = new AudioValve;
  m_infrom_sip->setOpen(false);
  prev_src->registerSink(m_infrom_sip, true);
  prev_src = m_infrom_sip;

  unsigned sql_hangtime;

  if (!semi_duplex)
  {
    squelch_det = new SquelchVox;
    if (!squelch_det->initialize(cfg(), name()))
    {
      cerr << name() << ":*** ERROR: Squelch detector initialization failed"
           << endl;
      delete squelch_det;
      squelch_det = 0;
      // FIXME: Cleanup
      return false;
    }
    if (cfg().getValue(name(), "SQL_HANGTIME", sql_hangtime))
    {
      squelch_det->setHangtime(sql_hangtime);
    }
    else
    {
      squelch_det->setHangtime(1300);
    }
    squelch_det->squelchOpen.connect(mem_fun(*this, &SipLogic::onSquelchOpen));
    splitter->addSink(squelch_det, true);
    cout << name() << ": Simplexmode, using VOX squelch for Sip." << endl;
  }
  else
  {
    cout << name() << ": Semiduplexmode, no Sql used for Sip." << endl;
  }

   // Attach the AudioSelector for Sip and message audio streams
  m_logic_con_out->addSource(prev_src);
  m_logic_con_out->enableAutoSelect(prev_src, 0);
  m_logic_con_out->setFlushWait(prev_src, false);

    // Create the message handler for announcements
  logic_msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  logic_msg_handler->allMsgsWritten.connect(mem_fun(*this, &SipLogic::allMsgsWritten));

  m_logic_con_out->addSource(logic_msg_handler);
  m_logic_con_out->enableAutoSelect(logic_msg_handler, 10);
  m_logic_con_out->setFlushWait(logic_msg_handler, false);

    // the event handler
  string event_handler_str;
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str))
  {
    cerr << name() << ":*** ERROR: Config variable EVENT_HANDLER not set"
         << endl;
    return false;
  }
  logic_event_handler = new EventHandler(event_handler_str, name());
  logic_event_handler->setVariable("logic_name", name().c_str());
  logic_event_handler->setVariable("logic_type", type());
  logic_event_handler->setVariable("sip_ctrl_pty", dtmf_ctrl_pty_path);

  logic_event_handler->playFile.connect(mem_fun(*this, &SipLogic::playLogicFile));
  logic_event_handler->playSilence.connect(mem_fun(*this, &SipLogic::playLogicSilence));
  logic_event_handler->playTone.connect(mem_fun(*this, &SipLogic::playLogicTone));
  logic_event_handler->playDtmf.connect(mem_fun(*this, &SipLogic::playLogicDtmf));
  using namespace std::placeholders;
  logic_event_handler->addCommand("initCall",
      std::bind(&SipLogic::initCallHandler, this, _1, _2));

  logic_event_handler->processEvent("namespace eval " + name() + " {}");

    // Make logic configuration variables available in TCL event handlers
  logic_event_handler->processEvent("namespace eval Logic {}");
  for (const auto& varname : cfg().listSection(name()))
  {
    std::string var = "Logic::CFG_" + varname;
    std::string value;
    cfg().getValue(name(), varname, value);
    logic_event_handler->setVariable(var, value);
  }

  if (!logic_event_handler->initialize())
  {
    cout << name() << ":*** ERROR initializing Logic eventhandler in SipLogic."
         << endl;
    return false;
  }

  /*************** outgoing to sip ********************/

  sip_event_handler = new EventHandler(event_handler_str, name());
  sip_event_handler->setVariable("logic_name", name().c_str());
  sip_event_handler->setVariable("logic_type", type());
  sip_event_handler->setVariable("sip_ctrl_pty", dtmf_ctrl_pty_path);
  sip_event_handler->setVariable("sip_proxy_server", m_sipserver);
  sip_event_handler->setVariable("sip_proxy_port", m_sip_port);

  sip_event_handler->playFile.connect(mem_fun(*this, &SipLogic::playSipFile));
  sip_event_handler->playSilence.connect(mem_fun(*this, &SipLogic::playSipSilence));
  sip_event_handler->playTone.connect(mem_fun(*this, &SipLogic::playSipTone));
  sip_event_handler->playDtmf.connect(mem_fun(*this, &SipLogic::playSipDtmf));
  sip_event_handler->processEvent("namespace eval " + name() + " {}");

    // Make logic configuration variables available in TCL event handlers
  sip_event_handler->processEvent("namespace eval Logic {}");
  for (const auto& varname : cfg().listSection(name()))
  {
    std::string var = "Logic::CFG_" + varname;
    std::string value;
    cfg().getValue(name(), varname, value);
    sip_event_handler->setVariable(var, value);
  }

  if (!sip_event_handler->initialize())
  {
    cout << name() << ":*** ERROR initializing Sip eventhandler in SipLogic."
         << endl;
    return false;
  }

    // Create the message handler for announcements
  sip_msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  sip_msg_handler->allMsgsWritten.connect(mem_fun(*this, &SipLogic::allMsgsWritten));

  sipselector = new AudioSelector;
  sipselector->addSource(m_logic_con_in);
  sipselector->enableAutoSelect(m_logic_con_in, 0);
  sipselector->setFlushWait(m_logic_con_in, false);

  sipselector->addSource(sip_msg_handler);
  sipselector->enableAutoSelect(sip_msg_handler, 10);
  sipselector->setFlushWait(sip_msg_handler, false);

   // the audio valve to control the incoming samples
  m_outto_sip = new AudioValve;
  m_outto_sip->setOpen(false);
  sipselector->registerSink(m_outto_sip, true);

   // the Audioreader to get and handle the samples later
   // when connected
  m_ar = new AudioReader;
  m_ar->registerSource(m_outto_sip);

   // auto create an outgoing call
  if (m_autoconnect.length() > 0)
  {
    makeCall(acc, m_autoconnect);
  }

  processLogicEvent("startup");

  cout << ">>> Started SvxLink with special SipLogic extension (v"
       << PJSIP_VERSION << ")" << endl;
  cout << ">>> No guarantee! Please send a bug report to\n"
       << ">>> Adi/DL1HRC <dl1hrc@gmx.de> or use the groups.io mailing list"
       << endl;

  // enable the execution of external tcl procedures since it handles start
  // information (registrations, ...) too. The sip-startup procedure must
  // beeing completely finished before the tcl procedures could be started
  startup_finished = true;

  return true;
} /* SipLogic::initialize */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

SipLogic::~SipLogic(void)
{
  startup_finished = false;
  hangupAllCalls();   calls.clear();
  m_call_timeout_timer.setEnable(false);
  if (accept_incoming_regex != 0)
  {
    regfree(accept_incoming_regex);
    delete accept_incoming_regex;
    accept_incoming_regex = 0;
  }
  if (reject_incoming_regex != 0)
  {
    regfree(reject_incoming_regex);
    delete reject_incoming_regex;
    reject_incoming_regex = 0;
  }
  if (accept_outgoing_regex != 0)
  {
    regfree(accept_outgoing_regex);
    delete accept_outgoing_regex;
    accept_outgoing_regex = 0;
  }
  if (reject_outgoing_regex != 0)
  {
    regfree(reject_outgoing_regex);
    delete reject_outgoing_regex;
    reject_outgoing_regex = 0;
  }
  delete media;           media = 0;
  delete acc;             acc = 0;
  delete m_logic_con_in;  m_logic_con_in = 0;
  delete m_out_src;       m_out_src = 0;
  delete dtmf_ctrl_pty;   dtmf_ctrl_pty = 0;
  delete m_ar;            m_ar = 0;
  delete logic_event_handler;  logic_event_handler = 0;
  delete sip_event_handler;    sip_event_handler = 0;
  delete logic_msg_handler;    logic_msg_handler = 0;
  delete sip_msg_handler;      sip_msg_handler = 0;
  delete logicselector;        logicselector = 0;
} /* SipLogic::~SipLogic */


std::string SipLogic::initCallHandler(int argc, const char* argv[])
{
  if (argc != 2)
  {
    return std::string("Usage: initCall: <phone number>");
  }
  std::string phonenumber(argv[1]);
  //std::cout << "### initCall(" << phonenumber << ")" << std::endl;
  makeCall(acc, phonenumber);
  return std::string();
} /* SipLogic::initCallHandler */


void SipLogic::allMsgsWritten(void)
{
  //cout << "SipLogic::allMsgsWritten\n";
} /* SipLogic::allMsgsWritten */


void SipLogic::checkIdle(void)
{
  //setIdle(getIdleState());
} /* SipLogic::checkIdle */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void SipLogic::makeCall(sip::_Account *acc, std::string dest_uri)
{
  stringstream ss;
  std::string caller = getCallerNumber(dest_uri);

  if ((regexec(reject_outgoing_regex, caller.c_str(),
	       0, 0, 0) == 0) ||
      (regexec(accept_outgoing_regex, caller.c_str(),
	       0, 0, 0) != 0))
  {
    ss << "drop_outgoing_call \"" << dest_uri << "\"";
    processLogicEvent(ss.str());
    return;
  }

  ss << "calling \"" << dest_uri << "\"";
  processLogicEvent(ss.str());
  
  cout << "+++ Outgoing call to " << dest_uri << endl;
  
  for (std::map<std::string, uint32_t>::const_iterator it = phoneNrTgVec.begin();
    it != phoneNrTgVec.end(); it++)
  {
    size_t pos;
    if ((pos = caller.find(it->first)) == 0)
    {
      uint32_t tg = it->second;
      if (m_siploglevel >= 3)
      {
        cout << name() << ", setting new TG=" << tg
             << " due to configuration (PHONENUMBERS_TO_TG)" << endl;
      }
      setReceivedTg(tg);
    }
  }

  CallOpParam prm(true);
  prm.opt.audioCount = 1;
  prm.opt.videoCount = 0;
  sip::_Call *call = new sip::_Call(*acc);

  try {
    call->makeCall(dest_uri, prm);
    registerCall(call);
    m_call_timeout_timer.setEnable(true);
  } catch (Error& err) {
    cout << name() << ": *** ERROR: " << err.info() << endl;
  }
} /* SipLogic::makeCall */


void SipLogic::onIncomingCall(sip::_Account *acc, pj::OnIncomingCallParam &iprm)
{

  // todo: accept more than just one call
  if (calls.size() > 1) return;

  stringstream ss;
  sip::_Call *call = new sip::_Call(*acc, iprm.callId);
  pj::CallInfo ci = call->getInfo();
  pj::CallOpParam prm;
  prm.opt.audioCount = 1;
  prm.opt.videoCount = 0;

  // check if it is a valid call
  if (!checkCaller(ci.remoteUri))
  {
    cout << "+++ Ignoring invalid call from " << ci.remoteUri
         << " since it isn't registered at our sip server (" << m_sipserver
         << ")" << endl;
    ss << "invalid_call " << ci.remoteUri;
    processLogicEvent(ss.str());
    return;
  }

  std::string caller = getCallerNumber(ci.remoteUri);
  for (std::map<std::string, uint32_t>::const_iterator it = phoneNrTgVec.begin();
      it != phoneNrTgVec.end(); it++)
  {
    size_t pos;
    if ((pos = caller.find(it->first)) == 0)
    {
      uint32_t tg = it->second;
      setReceivedTg(tg);
    }
  }

  ss << "ringing \"" << caller << "\"";
  processLogicEvent(ss.str());
  ss.clear();

  if (regexec(reject_incoming_regex, caller.c_str(), 0, 0, 0) == 0)
  {
    ss << "reject_incoming_call \"" << caller << "\"";
    processSipEvent(ss.str());
    return;
  }

  if (regexec(accept_incoming_regex, caller.c_str(), 0, 0, 0) == 0)
  {
    prm.statusCode = (pjsip_status_code)200;
    registerCall(call);

    if (m_autoanswer)
    {
      call->answer(prm);
    }
  }
} /* SipLogic::onIncomingCall */


bool SipLogic::checkCaller(std::string caller)
{
  if (ignore_reg)
  {
    cout << "Do not check the credentials due to "
         << "configuration (IGNORE_REGISTRATION=1)" << endl;
    return true;
  }

  for (const auto& addr : dns.addresses())
  {
    if (!addr.isEmpty())
    {
      if (addr.toString() == getCallerUri(caller))
      {
        cout << "Accepting incoming call from" << caller << "." << endl;
        return true;
      }
    }
  }
  return false;
} /* SipLogic::checkCaller */


void SipLogic::registerCall(sip::_Call *call)
{
  call->onDtmf.connect(mem_fun(*this, &SipLogic::onDtmfDigit));
  call->onMedia.connect(mem_fun(*this, &SipLogic::onMediaState));
  call->onCall.connect(mem_fun(*this, &SipLogic::onCallState));
  call->onMessage.connect(mem_fun(*this, &SipLogic::onMessageInfo));

  pj::CallInfo ci = call->getInfo();
  std::string caller = getCallerNumber(ci.remoteUri);

  stringstream ss;
  ss << "call_registered \"" << caller << "\"";
  processLogicEvent(ss.str());
  calls.push_back(call);
} /* SipLogic::registerCall */


void SipLogic::onMediaState(sip::_Call *call, pj::OnCallMediaStateParam &prm)
{

  pj::CallInfo ci = call->getInfo();

  if (ci.media.size() != 1)
  {
    cout << "*** ERROR: media size not 1 in " << name() << endl;
    return;
  }

  if (ci.media[0].status == PJSUA_CALL_MEDIA_ACTIVE)
  {
    if (ci.media[0].type == PJMEDIA_TYPE_AUDIO)
    {
      if (call->hasMedia() && call->getMedia(0) != NULL) {
        sip_buf = static_cast<pj::AudioMedia *>(call->getMedia(0));
        sip_buf->startTransmit(*media);
        media->startTransmit(*sip_buf);
        m_outto_sip->setOpen(true);
        m_infrom_sip->setOpen(semi_duplex);
        processSipEvent("remote_greeting");
      }
    }
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_NONE)
  {
    cout << name()
         << ":+++ Call currently has no media, or the media is not used."
         << endl;
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_LOCAL_HOLD)
  {
    cout << name()
         << ":+++ The media is currently put on hold by local endpoint."
         << endl;
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_REMOTE_HOLD)
  {
    cout << name()
         << ":+++ The media is currently put on hold by remote endpoint."
         << endl;
  }
  else if (ci.media[0].status == PJSUA_CALL_MEDIA_ERROR)
  {
    cout << name()
         << ":*** ERROR: The Sip audio media has reported an error."
         << endl;
  }
} /* SipLogic::onMediaState */


/*
 * incoming SvxLink audio stream to SIP client
 */
pj_status_t SipLogic::mediaPortGetFrame(pjmedia_port *port, pjmedia_frame *frame)
{

  int got = 0;
  int count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);
  float* smpl = new float[count+1]();
  pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
  frame->type = PJMEDIA_FRAME_TYPE_AUDIO;

  if (!smpl)
  {
    cout << "*** Race condition smpl-pointer is null\n";
    return PJ_SUCCESS;
  }

  if ((got = m_ar->readSamples(smpl, count)) > 0)
  {
    int i = 0;
    for (float* s = smpl; s < smpl + sizeof(float)*got; s += sizeof(float))
    {
      samples[i] = (pj_int16_t)(smpl[i] * 32768);
      i++;
    }
  }

  delete [] smpl;

  /*
    The pjsip framework requests 768 samples on every call. The SvxLink
    framework can only deliver samples if the sql isn't closed and
    m_logic_con_in provides some. Since the documentation of pjsip is poor
    I can not predict the behaviour if we provide less samples than the
    requested number. So we will fill the buffer with 0
  */
  while (++got < count)
  {
    samples[got] = (pj_int16_t) 0;
  }

  return PJ_SUCCESS;
} /* SipLogic::mediaPortGetFrame */


/*
 * incoming SIP audio stream to SvxLink
 */
pj_status_t SipLogic::mediaPortPutFrame(pjmedia_port *port, pjmedia_frame *frame)
{
  int pos = 0;
  int count = frame->size / 2 / PJMEDIA_PIA_CCNT(&port->info);

  if (count > 0)
  {
    pj_int16_t *samples = static_cast<pj_int16_t *>(frame->buf);
    frame->type = PJMEDIA_FRAME_TYPE_AUDIO;
    float* smpl = new float[count+1]();
    for (int i=pos; i < count; i++)
    {
      smpl[i] = (float) (samples[i] / 32768.0);
    }
    do {
      int ret = m_out_src->writeSamples(smpl + pos, count - pos);
      pos += ret;
    } while (pos < count);
    delete [] smpl;
  }

  return PJ_SUCCESS;
} /* SipLogic::mediaPortPutFrame */


void SipLogic::onCallState(sip::_Call *call, pj::OnCallStateParam &prm)
{
  stringstream ss;
  pj::CallInfo ci = call->getInfo();
  std::string caller = getCallerNumber(ci.remoteUri);
  std::string uri = getCallerUri(ci.remoteUri);

    // incoming call
  if (ci.state == PJSIP_INV_STATE_INCOMING)
  {
    ss << "incoming_call " << caller;
  }
  else if (ci.state == PJSIP_INV_STATE_CONNECTING) // connecting
  {
    ss << "call_connected " << caller;
    m_call_timeout_timer.setEnable(false);
    m_call_timeout_timer.reset();
  }
  else if (ci.state == PJSIP_INV_STATE_CALLING)     // calling
  {
    ss << "outgoing_call " << caller;
    cout << name() << ": Calling" << endl;
  }
  else if (ci.state == PJSIP_INV_STATE_CONFIRMED)
  {
    ss << "call_state_confirmed " << caller;
  }
  else if (ci.state == PJSIP_INV_STATE_DISCONNECTED)    // call disconnected
  {
    if (!startup_finished) return;
    cout << name()
         << ": Call hangup (" << caller << "), duration "
         << ci.totalDuration.sec << "."
         << ci.totalDuration.msec << " secs" << endl;

    m_outto_sip->setOpen(false);
    m_infrom_sip->setOpen(false);

    unregisterCall(call);
    ss << "hangup_call " << caller << " "
       << ci.totalDuration.sec << "."
       << ci.totalDuration.msec;

    setReceivedTg(0);

    // if no one is connected anymore, call out to autoconnect party
    if (calls.empty() && m_autoconnect.length() > 0)
    {
      makeCall(acc, m_autoconnect);
    }
  }
  else if (ci.state == PJSIP_INV_STATE_EARLY)
  {
    ss << "pjsip_state_early " << caller;
  }
  else if (ci.state == PJSIP_INV_STATE_NULL)
  {
    ss << "pjsip_state_null " << caller;
  }
  else
  {
    cout << "unknown_callstate " << ci.state;
  }
  processLogicEvent(ss.str());
} /* SipLogic::onCallState */


void SipLogic::onMessageInfo(sip::_Call *call, pj::OnInstantMessageParam &prm)
{
  std::string body = prm.msgBody;
  std::string uri = prm.contactUri;
  SipRxData message = prm.rdata;
  std::string contactUri = prm.contactUri;
  stringstream ss;
  ss << "text_message_received " << uri << " \"" 
     << message.wholeMsg << "\"";
  processSipEvent(ss.str());
  // ToDO
} /* SipLogic::onMessageInfo */


void SipLogic::onMessage(sip::_Account *acc, pj::OnInstantMessageParam &imprm)
{
  std::string uri = imprm.contactUri;
  SipRxData message = imprm.rdata;
  stringstream ss;
  ss << "account_text_message_received " << uri << " \"" 
     << message.wholeMsg << "\"";
  processSipEvent(ss.str());
} /* SipLogic::onMessage */


void SipLogic::onDtmfDigit(sip::_Call *call, pj::OnDtmfDigitParam &prm)
{
  pj::CallInfo ci = call->getInfo();
  stringstream ss;
  ss << "dtmf_digit_received " << prm.digit << " " << getCallerNumber(ci.remoteUri);
  processLogicEvent(ss.str());
} /* SipLogic::onDtmfDigit */


void SipLogic::onRegState(sip::_Account *acc, pj::OnRegStateParam &prm)
{
  pj::AccountInfo ai = acc->getInfo();
  stringstream ss;
  ss << "registration_state " << m_sipserver << " "
     << ai.regIsActive << " " << prm.code;
  processLogicEvent(ss.str());
  dns.setLookupParams(getCallerUri(m_sipserver));
  dns.lookup();
} /* SipLogic::onRegState */


void SipLogic::hangupAllCalls(void)
{
  CallOpParam prm(true);
  for (std::vector<sip::_Call *>::iterator it=calls.begin();
       it != calls.end(); it++)
  {
    (*it)->hangup(prm);
  }
} /* SipLogic::hangupAllCalls */


// hangup all calls
void SipLogic::hangupCalls(std::vector<sip::_Call *> calls)
{
  CallOpParam prm(true);

  for (std::vector<sip::_Call *>::iterator it=calls.begin();
       it != calls.end(); it++)
  {
    hangupCall(*it);
  }
} /* SipLogic::hangupCalls */


// hangup a single call
void SipLogic::hangupCall(sip::_Call *call)
{
  CallOpParam prm(true);
  m_out_src->allSamplesFlushed();

  for (std::vector<sip::_Call *>::iterator it=calls.begin();
       it != calls.end(); it++)
  {
    if (*it == call)
    {
      (*it)->hangup(prm);
      break;
    }
  }
  m_outto_sip->setOpen(false);
} /* SipLogic::hangupCall */


/**
 *  dial-out by sending a string over Pty device, e.g.
 *  echo "C12345#" > /tmp/sip_pty
 *  method converts it to a valid dial string:
 *  "sip:12345@sipserver.com:5060"
**/
void SipLogic::dtmfCtrlPtyCmdReceived(const void *buf, size_t count)
{
  const char *buffer = reinterpret_cast<const char*>(buf);

  if (acc != 0 && count > 1)
  {
      // hanging up all calls with "C#"
    if (buffer[0] == 'C')
    {
      if (buffer[1] == '#')
      {
        hangupCalls(calls);
        processLogicEvent("call_hangup_by_user");
        return;
      }

      // Answer incoming call by "CA"
      if (buffer[1] == 'A')
      {
        if (!calls.empty())
        {
          pj::CallOpParam prm;
          prm.opt.audioCount = 1;
          prm.opt.videoCount = 0;
          prm.statusCode = (pjsip_status_code)200;
          std::vector<sip::_Call *>::iterator call = calls.begin();
          (*call)->answer(prm);
          processLogicEvent("incoming_call_answered");
        }
        return;
      }

        // calling a party with "C12345#"
        // "C12345#" -> sip:12345@sipserver.de
      string tocall = "sip:";
      for (size_t i=1; i<count; ++i)
      {
        const char &ch = buffer[i];
        if (::isdigit(ch))
        {
          tocall += ch;
        }
      }
      tocall += "@";
      tocall += m_sipserver;
      makeCall(acc, tocall);
      m_call_timeout_timer.setEnable(true);
    }
  }
} /* SipLogic::dtmfCtrlPtyCmdReceived */


void SipLogic::flushAudio(void)
{
} /* SipLogic::flushAudio */


void SipLogic::allSamplesFlushed(void)
{
} /* SipLogic::allSamplesFlushed */


std::string SipLogic::getCallerNumber(std::string uri)
{
  size_t pos = uri.find(':');
  size_t pos2 = uri.find('@');

  if (pos != std::string::npos && pos2 != std::string::npos)
  {
    return uri.substr(pos + 1 , pos2 - pos - 1);
  }
  return "unknown";
} /* SipLogic::getCallerNumber */


void SipLogic::callTimeout(Async::Timer *t)
{

  for (std::vector<sip::_Call *>::iterator it=calls.begin();
       it != calls.end(); it++)
  {
    if (!(*it)->hasMedia())
    {
      hangupCall(*it);
    }
  }

  stringstream ss;
  ss << "call_timeout";
  processLogicEvent(ss.str());
  m_call_timeout_timer.setEnable(false);
  m_call_timeout_timer.reset();
} /* SipLogic::flushTimeout */


void SipLogic::flushTimeout(Async::Timer *t)
{
  m_out_src->allSamplesFlushed();
} /* SipLogic::flushTimeout */


void SipLogic::onSquelchOpen(bool is_open)
{
  cout << name() << ": The Sip squelch is "
       << (is_open ? "OPEN" : "CLOSED") << endl;
  m_infrom_sip->setOpen(is_open);
} /* SipLogic::onSquelchOpen */


void SipLogic::processLogicEvent(const string& event)
{
  if (!startup_finished) return;
  logic_msg_handler->begin();
  logic_event_handler->processEvent(name() + "::" + event);
  logic_msg_handler->end();
} /* SipLogic::processLogicEvent */


void SipLogic::processSipEvent(const string& event)
{
  if (!startup_finished) return;
  sip_msg_handler->begin();
  sip_event_handler->processEvent(name() + "::" + event);
  sip_msg_handler->end();
} /* SipLogic::processSipEvent */


void SipLogic::playLogicFile(const string& path)
{
  logic_msg_handler->playFile(path, report_events_as_idle);
} /* SipLogic::playLogicFile */


void SipLogic::playLogicSilence(int length)
{
  logic_msg_handler->playSilence(length, report_events_as_idle);
} /* SipLogic::playLogicSilence */


void SipLogic::playLogicTone(int fq, int amp, int len)
{
  logic_msg_handler->playTone(fq, amp, len, report_events_as_idle);
} /* SipLogic::playLogicTone */


void SipLogic::playLogicDtmf(const std::string& digits, int amp, int len)
{
  for (string::size_type i=0; i < digits.size(); ++i)
  {
    logic_msg_handler->playDtmf(digits[i], amp, len);
    logic_msg_handler->playSilence(50, report_events_as_idle);
  }
} /* SipLogic::playLogicDtmf */


void SipLogic::playSipFile(const string& path)
{
  sip_msg_handler->playFile(path, report_events_as_idle);
} /* SipLogic::playSipFile */


void SipLogic::playSipSilence(int length)
{
  sip_msg_handler->playSilence(length, report_events_as_idle);
} /* SipLogic::playSipSilence */


void SipLogic::playSipTone(int fq, int amp, int len)
{
  sip_msg_handler->playTone(fq, amp, len, report_events_as_idle);
} /* SipLogic::playSipTone */


void SipLogic::playSipDtmf(const std::string& digits, int amp, int len)
{
  for (string::size_type i=0; i < digits.size(); ++i)
  {
    sip_msg_handler->playDtmf(digits[i], amp, len);
    sip_msg_handler->playSilence(50, report_events_as_idle);
  }
} /* SipLogic::playSipDtmf */


void SipLogic::unregisterCall(sip::_Call *call)
{
  for (std::vector<sip::_Call *>::iterator it=calls.begin();
         it != calls.end(); it++)
  {
    if (*it == call)
    {
      calls.erase(it);
      break;
    }
  }

  if (calls.empty())
  {
    m_outto_sip->setOpen(false);
    m_infrom_sip->setOpen(false);
    //squelch_det->reset();
  }
} /* SipLogic::unregisterCall */


std::string SipLogic::getCallerUri(std::string uri)
{
  size_t pos = uri.find('@');
  if (pos != std::string::npos)
  {
    uri.erase(0, pos+1);
  }

  pos = uri.find(':');
  if (pos != std::string::npos)
  {
    uri = uri.substr(0, pos);
  }

  pos = uri.find('>');
  if (pos != std::string::npos)
  {
    uri = uri.substr(0,pos);
  }
  return uri;
} /* SipLogic::getCallerUri */


void SipLogic::onDnsResultsReady(Async::DnsLookup& dns_lookup)
{
} /* SipLogic::onDnsResult */


/*
 * This file has not been truncated
 */
