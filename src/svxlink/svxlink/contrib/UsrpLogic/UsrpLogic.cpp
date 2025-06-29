/**
@file	 UsrpLogic.cpp
@brief   A logic core that connect to the SvxUsrp
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2021-04-26

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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <string.h>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncUdpSocket.h>
#include <version/SVXLINK.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDecimator.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioFilter.h>
#include <AsyncAudioClipper.h>
#include <AsyncAudioCompressor.h>
#include <json/json.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "UsrpLogic.h"
#include "../trx/multirate_filter_coeff.h"
#include "EventHandler.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define USRPSOFT "SvxLink-Usrp"
#define USRPVERSION "08072024a"

#define LOGERROR 0
#define LOGWARN 1
#define LOGINFO 2
#define LOGDEBUG 3

#define DEFAULT_NETLIMITER_THRESH  -1.0
#define DEFAULT_LOCALLIMITER_THRESH  -1.0

/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



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

 extern "C" {
  LogicBase* construct(void) { return new UsrpLogic; }
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

UsrpLogic::UsrpLogic(void)
  : m_logic_con_in(0), m_logic_con_out(0), m_dec(0),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_enc(0), m_tg_select_timeout(DEFAULT_TG_SELECT_TIMEOUT),
    m_selected_tg(0), udp_seq(0), stored_samples(0), m_callsign("N0CALL"),
    ident(false), m_dmrid(0), m_rptid(0), m_selected_cc(0), m_selected_ts(1),
    preamp_gain(0), net_preamp_gain(0), m_event_handler(0), m_last_tg(0),
    debug(LOGERROR), share_userinfo(true),
    m_delay_timer(5000, Timer::TYPE_ONESHOT, false), callsign("N0CALL")
{
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &UsrpLogic::flushTimeout));
  timerclear(&m_last_talker_timestamp);
  m_delay_timer.expired.connect(
      mem_fun(*this, &UsrpLogic::onDelayTimeout));
} /* UsrpLogic::UsrpLogic */


bool UsrpLogic::initialize(Async::Config& cfgobj, const std::string& logic_name)
{

  /* section for outgoing audio to usrp net */
  // Create logic connection incoming audio passthrough
  m_logic_con_in = new Async::AudioStreamStateDetector;
  m_logic_con_in->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &UsrpLogic::onLogicConInStreamStateChanged));

  m_logic_con_out = new Async::AudioStreamStateDetector;
  m_logic_con_out->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &UsrpLogic::onLogicConOutStreamStateChanged));

  if (!LogicBase::initialize(cfgobj, logic_name))
  {
    cout << "*** ERROR: Initializing LogicBase " << name() << endl;
    return false;
  }

  cfg().getValue(name(),"DEBUG", debug);
  if (!cfg().getValue(name(), "USRP_HOST", m_usrp_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration"
         << endl;
    return false;
  }
  if (debug > LOGERROR)
  {
    cout << "    USRP_HOST=" << m_usrp_host << endl;
  }

  m_usrp_port = 41234;
  cfg().getValue(name(), "USRP_TX_PORT", m_usrp_port);
  if (debug > LOGERROR)
  {
    cout << "    USRP_TX_PORT=" << m_usrp_port << endl;
  }

  m_usrp_rx_port = 41233;
  cfg().getValue(name(), "USRP_RX_PORT", m_usrp_rx_port);
  if (debug > LOGERROR)
  {
    cout << "    USRP_RX_PORT=" << m_usrp_rx_port << endl;
  }

  m_udp_rxsock = new UdpSocket(m_usrp_rx_port);
  m_udp_rxsock->dataReceived.connect(
       mem_fun(*this, &UsrpLogic::udpDatagramReceived));

  if (!cfg().getValue(name(), "CALL", m_callsign))
  {
    cout << "*** ERROR: No " << name() << "/CALL= configured" << endl;
    return false;
  }

  if(m_callsign.length() > 6)
  {
    cout << "*** ERROR: Callsign ("<< m_callsign << ") is to long"
         << " should have 6 digits maximum." << endl;
    return false;
  }
  callsign = m_callsign;

  if (!cfg().getValue(name(), "DMRID", m_dmrid))
  {
    m_dmrid = 0;
    cout << "+++ WARNING: No " << name() << "/DMRID= configured, "
         << "using " << m_dmrid << endl;
  }

  if (!cfg().getValue(name(), "RPTID", m_rptid))
  {
    m_rptid = 0;
  }

  if (!cfg().getValue(name(), "DEFAULT_TG", m_selected_tg))
  {
    m_selected_tg = 0;
  }

  string in;
  if (!cfg().getValue(name(), "DEFAULT_CC", in))
  {
    m_selected_cc = 0x01;
  }
  else
  {
    m_selected_cc = atoi(in.c_str()) & 0xff;
  }

  if (!cfg().getValue(name(), "DEFAULT_TS", in))
  {
    m_selected_ts = 0x01;
  }
  else
  {
    m_selected_ts = atoi(in.c_str()) & 0xff;
  }

  string event_handler_str;
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str))
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/EVENT_HANDLER not set\n";
    return false;
  }

  m_event_handler = new EventHandler(event_handler_str, name());
  if (LinkManager::hasInstance())
  {
    m_event_handler->playFile.connect(
          sigc::mem_fun(*this, &UsrpLogic::handlePlayFile));
    m_event_handler->playSilence.connect(
          sigc::mem_fun(*this, &UsrpLogic::handlePlaySilence));
    m_event_handler->playTone.connect(
          sigc::mem_fun(*this, &UsrpLogic::handlePlayTone));
    m_event_handler->playDtmf.connect(
          sigc::mem_fun(*this, &UsrpLogic::handlePlayDtmf));
  }
  m_event_handler->setConfigValue.connect(
      sigc::mem_fun(cfg(), &Async::Config::setValue<std::string>));
  m_event_handler->setVariable("logic_name", name().c_str());
  m_event_handler->processEvent("namespace eval Logic {}");

  if (!m_event_handler->initialize())
  {
    cout << "*** ERROR: Could not initialize event handler." << endl;
    return false;
  }

  AudioSource *prev_src = m_logic_con_in;

  cfg().getValue(name(), "PREAMP", preamp_gain);

   // If a preamp was configured, create it
  if (preamp_gain != 0)
  {
    AudioAmp *preamp = new AudioAmp;
    preamp->setGain(preamp_gain);
    prev_src->registerSink(preamp, true);
    prev_src = preamp;
  }

  if (INTERNAL_SAMPLE_RATE == 16000)
  {
    AudioDecimator *d1 = new AudioDecimator(2, coeff_16_8,
					    coeff_16_8_taps);
    prev_src->registerSink(d1, true);
    prev_src = d1;
  }

  std::string audio_to_usrp;
  if (cfg().getValue(name(), "FILTER_TO_USRP", audio_to_usrp))
  {
    AudioFilter *usrp_out_filt = new AudioFilter(audio_to_usrp);
    prev_src->registerSink(usrp_out_filt, true);
    prev_src = usrp_out_filt;
  }

  // Add a limiter to smoothly limit the audio before hard clipping it
  double local_limiter_thresh = DEFAULT_LOCALLIMITER_THRESH;
  cfg().getValue(name(), "LOCAL_LIMITER_THRESH", local_limiter_thresh);
  if (local_limiter_thresh != 0.0)
  {
    AudioCompressor *locallimit = new AudioCompressor;
    locallimit->setThreshold(local_limiter_thresh);
    locallimit->setRatio(0.1);
    locallimit->setAttack(2);
    locallimit->setDecay(20);
    locallimit->setOutputGain(1);
    prev_src->registerSink(locallimit, true);
    prev_src = locallimit;
  }

    // Clip audio to limit its amplitude
  AudioClipper *localclipper = new AudioClipper;
  localclipper->setClipLevel(0.98);
  prev_src->registerSink(localclipper, true);
  prev_src = localclipper;

    // Remove high frequencies generated by the previous clipping
  AudioFilter *local_splatter_filter = new AudioFilter("HpCh12/-0.05/300 x LpCh9/-0.05/8000");
  prev_src->registerSink(local_splatter_filter, true);
  prev_src = local_splatter_filter;

  m_enc_endpoint = prev_src;

  /* section for the net audio incoming from usrp */
  prev_src = 0;
    // Create dummy audio codec used before setting the real encoder
  if (!setAudioCodec()) { return false; }
  prev_src = m_dec;

    // Create jitter buffer
  AudioFifo *fifo = new Async::AudioFifo(2*INTERNAL_SAMPLE_RATE);
  prev_src->registerSink(fifo, true);
  prev_src = fifo;
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
  }

  std::string audio_from_usrp;
  if (cfg().getValue(name(), "FILTER_FROM_USRP", audio_from_usrp))
  {
    AudioFilter *usrp_in_filt = new AudioFilter(audio_from_usrp);
    prev_src->registerSink(usrp_in_filt, true);
    prev_src = usrp_in_filt;
  }

   // If a net_preamp was configured, create it
  cfg().getValue(name(), "NET_PREAMP", net_preamp_gain);
  if (net_preamp_gain != 0)
  {
    AudioAmp *net_preamp = new AudioAmp;
    net_preamp->setGain(net_preamp_gain);
    prev_src->registerSink(net_preamp, true);
    prev_src = net_preamp;
  }

  /* Limiter that controls audio from USRP -> local */
  // Add a limiter to smoothly limit the audio before hard clipping it
  double net_limiter_thresh = DEFAULT_NETLIMITER_THRESH;
  cfg().getValue(name(), "NET_LIMITER_THRESH", net_limiter_thresh);
  if (net_limiter_thresh != 0.0)
  {
    AudioCompressor *netlimit = new AudioCompressor;
    netlimit->setThreshold(net_limiter_thresh);
    netlimit->setRatio(0.1);
    netlimit->setAttack(2);
    netlimit->setDecay(20);
    netlimit->setOutputGain(1);
    prev_src->registerSink(netlimit, true);
    prev_src = netlimit;
  }

    // Clip audio to limit its amplitude
  AudioClipper *netclipper = new AudioClipper;
  netclipper->setClipLevel(0.98);
  prev_src->registerSink(netclipper, true);
  prev_src = netclipper;

    // Remove high frequencies generated by the previous clipping
  AudioFilter *net_splatter_filter = new AudioFilter("HpCh12/-0.05/300 x LpCh9/-0.05/8000");
  prev_src->registerSink(net_splatter_filter, true);
  prev_src = net_splatter_filter;

  if (INTERNAL_SAMPLE_RATE == 16000)
  {
     // Interpolate sample rate to 16kHz
    AudioInterpolator *i1 = new AudioInterpolator(2, coeff_16_8,
                                                  coeff_16_8_taps);
    prev_src->registerSink(i1, true);
    prev_src = i1;
  }

  prev_src->registerSink(m_logic_con_out, true);
  prev_src = 0;

  r_buf = new int16_t[USRP_AUDIO_FRAME_LEN*2];

  cfg().getValue(name(),"SHARE_USERINFO", share_userinfo);

  // reads information from /etc/svxlink/dv_users.json
  std::string user_info_file;
  if (cfg().getValue(name(), "DV_USER_INFOFILE", user_info_file))
  {
    cout << "+++ Reading users data from \"" << user_info_file << "\"" << endl;
    std::ifstream user_info_is(user_info_file.c_str(), std::ios::in);
    if (user_info_is.good())
    {
      try
      {
        if (!(user_info_is >> m_user_info))
        {
          std::cerr << "*** ERROR: Failure while reading user information file "
                       "\"" << user_info_file << "\""
                    << std::endl;
          return false;
        }
      }
      catch (const Json::Exception& e)
      {
        std::cerr << "*** ERROR: Failure while reading user information "
                     "file \"" << user_info_file << "\": "
                  << e.what()
                  << std::endl;
        return false;
      }
    }
    else
    {
      std::cerr << "*** ERROR: Could not open user information file "
                   "\"" << user_info_file << "\""
                << std::endl;
      return false;
    }

    User m_user;
    for (Json::Value::ArrayIndex i = 0; i < m_user_info.size(); i++)
    {
      Json::Value& t_userdata = m_user_info[i];
      m_user.id = t_userdata.get("id", "").asString();
      if (m_user.id.length() < 1)
      {
        cout << "*** ERROR: The TSI must have more than 1 digit.\n"
          << "\" Check dataset " << i + 1 << " in \"" << user_info_file
          << "\"" << endl;
        return false;
      }
      m_user.name = t_userdata.get("name","").asString();
      m_user.mode = t_userdata.get("mode","").asString();
      m_user.call = t_userdata.get("call","").asString();
      m_user.location = t_userdata.get("location","").asString();
      if (t_userdata.get("symbol","").asString().length() != 2)
      {
        cout << "*** ERROR: Aprs symbol in \"" << user_info_file 
           << "\" dataset " << i + 1 << " is not correct, must have 2 digits!"
           << endl;
        return false;
      }
      else 
      {
        m_user.aprs_sym = t_userdata.get("symbol","").asString()[0];
        m_user.aprs_tab = t_userdata.get("symbol","").asString()[1];
      }
      m_user.comment = t_userdata.get("comment","").asString();
      struct tm mtime = {0}; // set default date/time 31.12.1899
      m_user.last_activity = mktime(&mtime);
      m_user.sent_last_sds = mktime(&mtime);
      userdata[m_user.id] = m_user;
      if (debug >= LOGINFO)
      {
        cout << "id=" << m_user.id << ",call=" << m_user.call << ",name="
             << m_user.name << ",mode=" << m_user.mode << ",location="
             << m_user.location << ",comment=" << m_user.comment << endl;
      }
    }
  }

    // receive interlogic messages
  publishStateEvent.connect(
          mem_fun(*this, &UsrpLogic::onPublishStateEvent));

  m_delay_timer.setEnable(true);

  return true;
} /* UsrpLogic::initialize */


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


void UsrpLogic::handleMsgError(std::istream& is)
{
} /* UsrpLogic::handleMsgError */


void UsrpLogic::sendEncodedAudio(const void *buf, int count)
{
  // identify as 1st frame
  if (!ident)
  {
    sendMetaMsg();
  }

  UsrpAudioMsg usrp;
  usrp.setType(USRP_TYPE_VOICE);
  usrp.setKeyup(true);

  int len = (int)(count * sizeof(char) / sizeof(int16_t));

  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }

  const int16_t *t = reinterpret_cast<const int16_t*>(buf);

  memcpy(r_buf+stored_samples, t, sizeof(int16_t)*len);
  stored_samples += len;

  while (stored_samples >= USRP_AUDIO_FRAME_LEN)
  {
    usrp.setAudioData(r_buf);
    sendAudioMsg(usrp);
    memmove(r_buf, r_buf + USRP_AUDIO_FRAME_LEN, 
              sizeof(int16_t)*(stored_samples-USRP_AUDIO_FRAME_LEN));
    stored_samples -= USRP_AUDIO_FRAME_LEN;
  }
} /* UsrpLogic::sendEncodedAudio */


void UsrpLogic::flushEncodedAudio(void)
{
  m_enc->allEncodedSamplesFlushed();
  m_flush_timeout_timer.setEnable(true);
} /* UsrpLogic::flushEncodedAudio */


void UsrpLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  //cout << "incoming packet from " << addr.toString() << ", len=" << count
  //     << endl;
  stringstream si;
  si.write(reinterpret_cast<const char*>(buf), count);
  UsrpHeaderMsg usrp;

  if (!usrp.unpack(si))
  {
    cout << "*** WARNING[" << name()
         << "]: Unpacking failed for UDP UsrpHeaderMsg" << endl;
    return;
  }

  uint32_t utype = usrp.type();

   // if we receive a fram USRP_TYPE_VOICE and keyup is true
   // then  handle it as incoming audio
  if (utype == USRP_TYPE_VOICE)
  {
    if (usrp.keyup() == false)
    {
      handleStreamStop();  // this was an audio stop frame
    }
    else
    {
      stringstream si;
      si.write(reinterpret_cast<const char*>(buf), count);
      UsrpAudioMsg usrpaudio;
      if (!usrpaudio.unpack(si))
      {
        cout << "*** WARNING[" << name()
             << "]: Unpacking failed for UDP UsrpAudioMsg" << endl;
        return;
      }
      handleVoiceStream(usrpaudio);
    }
  }
  else if (utype == USRP_TYPE_TEXT)
  {
    // check type of TXT message before further handling
    // if it's a TLV (0x08) then re-serialize the message
    stringstream shead;
    shead.write(reinterpret_cast<const char*>(buf), count);

    UsrpMetaTextMsg usrpmeta;
    if (!usrpmeta.unpack(shead))
    {
      cout << "*** WARNING[" << name()
           << "]: Unpacking failed for UDP stream to UsrpMetaTextMsg" << endl;
      return;
    }

    if (usrpmeta.isTlv())
    {
      stringstream stlv;
      stlv.write(reinterpret_cast<const char*>(buf), count);

      UsrpTlvMetaMsg usrptlvmsg;
      if (!usrptlvmsg.unpack(stlv))
      {
        cout << "*** WARNING[" << name()
             << "]: Unpacking failed for UDP stream to UsrpTlvMetaMsg" << endl;
        return;
      }
      m_last_call = usrptlvmsg.getCallsign();
      m_last_tg = usrptlvmsg.getTg();
      m_last_dmrid = usrptlvmsg.getDmrId();
    }
    else
    {
      size_t found;
      stringstream sp;
      sp.write(reinterpret_cast<const char*>(buf), count);
      std::string metadata = sp.str().substr(USRP_HEADER_LEN, 
                                count-USRP_HEADER_LEN);

      uint32_t ti = time(NULL);
      Json::CharReaderBuilder rbuilder;
      std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
      std::string jsonReaderError;
      Json::Value value;
      Json::Value event(Json::arrayValue);
      Json::Value userinfo(Json::objectValue);
      userinfo["last_activity"] = ti;
      userinfo["gwcallsign"] = m_callsign;

      if ((found = metadata.find("INFO:MSG:")) != string::npos)
      {
        string infomessage = metadata.erase(0, metadata.find_last_of(" ") + 1);
        infomessage.erase(remove_if(infomessage.begin(),infomessage.end(),
               [](char c){return !(c>=32 && c <128);}), infomessage.end()); 
        handleSettingsMsg(infomessage);
        userinfo["mode"] = infomessage;
      }
      else if ((found = metadata.find("INFO:{")) != string::npos)
      {
        /*
        INFO:{"ab":{"version":"1.6.2","date":"Wed.Mar.31.13:52:10.EDT.2021"},
            "dv3000":{"ip":"127.0.0.1","port":"2460","use_serial":"false"},
            "use_fallback":"true",
            "use_emulator":"true",
            "mute":"OFF",
            "usrp":
              {
                "ip":"127.0.0.1","rx_port":"14321","tx_port":"14321",
                "ping":"10",
                "to_pcm":{"shape":"AUDIO_USE_AGC","gain":"4.00"},
                "to_ambe":{"shape":"AUDIO_USE_GAIN","gain":"0.35"}
              },
              "tlv":{"ip":"127.0.0.1","tx_port":"0","rx_port":"0",
                        "ambe_size":"72","ambe_mode":"NXDN"},
              "digital": {"gw":"1234567","rpt":"123456789","tg":"7",
                        "ts":"2","cc":"1","call":"N0CALL"},
              "last_tune":"7"
            }
        */
        metadata.erase(0,5); // remove "INFO:"
        if (!reader->parse(metadata.c_str(), metadata.c_str() + metadata.size(), 
              &value, &jsonReaderError))
        {
          cout << "*** ERROR: Failed to parse json: " 
               << jsonReaderError << endl;
          return;
        }
        else
        {
          m_last_call = value["digital"]["call"].asString();
          m_last_tg = atoi(value["digital"]["tg"].asString().c_str());
          m_last_dmrid = atoi(value["digital"]["rpt"].asString().c_str());
          m_last_ts =  atoi(value["digital"]["ts"].asString().c_str());
          m_last_cc =  atoi(value["digital"]["cc"].asString().c_str());
          m_last_mode = value["digital"]["ambe_mode"].asString();
          userinfo["callsign"] = m_last_call;
          userinfo["tg"] = m_last_tg;
          userinfo["mode"] = m_last_mode;
          userinfo["gateway"] = m_last_dmrid;
        }
      }
      else if ((found = metadata.find("INFO:")) != string::npos)
      {
        metadata.erase(0,5); // to do
        return;
      }
      event.append(userinfo);
      publishInfo("DvUsers:info", event);
    }

    if (m_last_tg > 0 && m_last_call.length() > 0)
    {
      stringstream ss;
      ss << "usrp_stationdata_received " << m_last_call << " "
         << m_last_tg << " " << m_last_dmrid;
      processEvent(ss.str());
    }
  }
  else
  {
    cout << "*** unknown type of message:" << utype << endl;
  }
} /* UsrpLogic::udpDatagramReceived */


void UsrpLogic::handleVoiceStream(UsrpAudioMsg usrp)
{
  gettimeofday(&m_last_talker_timestamp, NULL);
  std::array<int16_t, 160> m_audio_data;
  for (size_t i = 0; i<usrp.audioData().size(); i++)
  {
    m_audio_data[i] = ntohs(usrp.audioData()[i]);
  }
  m_dec->writeEncodedSamples(&m_audio_data, 
                        sizeof(int16_t)*USRP_AUDIO_FRAME_LEN);
} /* UsrpLogic::handleVoiceStream */


void UsrpLogic::handleStreamStop(void)
{
  m_dec->flushEncodedSamples();
  checkIdle();
  m_enc->allEncodedSamplesFlushed();
  timerclear(&m_last_talker_timestamp);

  stringstream ss;
  ss << "talker_stop " << m_last_tg << " " << m_last_call;
  processEvent(ss.str());
} /* UsrpLogic::handleStreamStop */


void UsrpLogic::sendInfoJson(void)
{
  stringstream ss;
  ss << "{\"ab\":{\"version\":\""
  << USRPSOFT << "," << USRPVERSION << "\"},"
  << "\"digital\":{\"gw\":\""
  << m_dmrid << "\",\"rpt\":\""
  << m_rptid << "\",\"tg\":\""
  << m_selected_tg << "\",\"ts\":\""
  << m_selected_ts << "\",\"cc\":\""
  << m_selected_cc <<  "\",\"call\":\""
  << m_callsign << "\"}}";

  cout << ss.str() << endl;
} /* UsrpLogic::sendInfoJson */


void UsrpLogic::handleSettingsMsg(std::string infomsg)
{
  stringstream ss;
  ss << "setting_mode " << infomsg;
  processEvent(ss.str());
} /* UsrpLogic::handleSettingsMsg */


void UsrpLogic::handleMetaData(std::string metadata)
{
  Json::Value user_arr;
  Json::CharReaderBuilder rbuilder;
  std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
  std::string jsonReaderError;

  if (!reader->parse(metadata.c_str(), metadata.c_str() + metadata.size(), 
       &user_arr, &jsonReaderError))
  {
    cout << "*** Error: parsing StateEvent message ("
         << jsonReaderError << ")" << endl;
    return;
  }

  stringstream ss;
  for (Json::Value::ArrayIndex i = 0; i != user_arr.size(); i++)
  {
    Json::Value& t_userdata = user_arr[i];
    ss << t_userdata.get("digital","").asString();
  }
  cout << "+++ Metadata received: " << ss.str() << endl;

} /* UsrpLogic::handleMetaData */


void UsrpLogic::sendAudioMsg(UsrpAudioMsg& usrp)
{
  if (udp_seq++ > 0x7fff) udp_seq = 0;
  usrp.setSeq(udp_seq);

  ostringstream ss;
  if (!usrp.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack UDP Usrp message\n";
    return;
  }

  sendUdpMessage(ss);
} /* UsrpLogic::sendMsg */


void UsrpLogic::sendStopMsg(void)
{
  UsrpHeaderMsg usrp;

  if (udp_seq++ > 0x7fff) udp_seq = 0;
  usrp.setSeq(udp_seq);

  ostringstream os;
  if (!usrp.pack(os))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack UDP Usrp message\n";
    return;
  }
  sendUdpMessage(os);
  ident = false;

  stringstream ss;
  ss << "transmission_stop " << m_selected_tg;
  processEvent(ss.str());
  
  callsign = m_callsign; // reset to own callsign
} /* UsrpLogic::sendStopMsg */


void UsrpLogic::sendMetaMsg(void)
{
  UsrpTlvMetaMsg usrp;
  usrp.setTg(m_selected_tg);
  usrp.setRptId(m_rptid);
  usrp.setCC(m_selected_cc);
  usrp.setTS(m_selected_ts);
  usrp.setTg(m_selected_tg);

  if (talker.isTalking)
  {
    usrp.setCallsign(talker.callsign);
    usrp.setDmrId(0);
  }
  else
  {
    usrp.setCallsign(m_callsign);
    usrp.setDmrId(m_dmrid);
  }

  if (udp_seq++ > 0x7fff) udp_seq = 0;
  usrp.setSeq(udp_seq);

  ostringstream os;
  if (!usrp.pack(os))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack UDP Usrp message\n";
    return;
  }

  sendUdpMessage(os);
  ident = true;

  stringstream ss;
  ss << "transmission_start " << m_selected_tg;
  processEvent(ss.str());
} /* UsrpLogic::sendMetaMsg */


void UsrpLogic::sendUdpMessage(ostringstream& ss)
{
  IpAddress usrp_addr(m_usrp_host); 
  m_udp_rxsock->write(usrp_addr, m_usrp_port, ss.str().data(), ss.str().size());
} /* UsrpLogic::sendUdpMessage */


void UsrpLogic::sendHeartbeat(void)
{
} /* UsrpLogic::sendHeartbeat */


void UsrpLogic::allEncodedSamplesFlushed(void)
{
  //sendUdpMsg(MsgUdpAllSamplesFlushed());
} /* UsrpLogic::allEncodedSamplesFlushed */


void UsrpLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_enc->allEncodedSamplesFlushed();
} /* UsrpLogic::flushTimeout */


void UsrpLogic::onDelayTimeout(Async::Timer *t)
{
  // send userinfo to reflector
  sendUserInfo();
} /* UsrpLogic::onDelayTimeout */


void UsrpLogic::handleTimerTick(Async::Timer *t)
{
  if (timerisset(&m_last_talker_timestamp))
  {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_talker_timestamp, &diff);
    if (diff.tv_sec > 3)
    {
      cout << name() << ": Last talker audio timeout" << endl;
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
    }
  }
} /* UsrpLogic::handleTimerTick */


bool UsrpLogic::setAudioCodec(void)
{
  delete m_enc;
  m_enc = Async::AudioEncoder::create("S16");
  if (m_enc == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize audio encoder" 
         << endl;
    m_enc = Async::AudioEncoder::create("DUMMY");
    assert(m_enc != 0);
    return false;
  }
  m_enc->writeEncodedSamples.connect(
      mem_fun(*this, &UsrpLogic::sendEncodedAudio));
  m_enc->flushEncodedSamples.connect(
      mem_fun(*this, &UsrpLogic::flushEncodedAudio));
  m_enc_endpoint->registerSink(m_enc, false);

  AudioSink *sink = 0;
  if (m_dec != 0)
  {
    sink = m_dec->sink();
    m_dec->unregisterSink();
    delete m_dec;
  }
  m_dec = Async::AudioDecoder::create("S16");
  if (m_dec == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize audio decoder" 
         << endl;
    m_dec = Async::AudioDecoder::create("DUMMY");
    assert(m_dec != 0);
    return false;
  }
  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &UsrpLogic::allEncodedSamplesFlushed));
  if (sink != 0)
  {
    m_dec->registerSink(sink, true);
  }

  return true;
} /* UsrpLogic::setAudioCodec */


void UsrpLogic::onLogicConInStreamStateChanged(bool is_active,
                                                    bool is_idle)
{
  checkIdle();
  if (is_idle)
  {
    sendStopMsg();
  }
} /* UsrpLogic::onLogicConInStreamStateChanged */


void UsrpLogic::onLogicConOutStreamStateChanged(bool is_active,
                                                     bool is_idle)
{
  checkIdle();
} /* UsrpLogic::onLogicConOutStreamStateChanged */


bool UsrpLogic::isIdle(void)
{
  return m_logic_con_out->isIdle() && m_logic_con_in->isIdle();
} /* UsrpLogic::isIdle */


void UsrpLogic::checkIdle(void)
{
  setIdle(isIdle());
} /* UsrpLogic::checkIdle */


// switching between DMR, YSF, NXDN, P25
void UsrpLogic::switchMode(uint8_t mode)
{
  UsrpTlvMetaMsg usrp;
  usrp.setMetaData(selected_mode[mode]);
  usrp.setType(USRP_TYPE_DTMF);
  usrp.setTlv(0x00);
  usrp.setTlvLen(0x00);

  if (udp_seq++ > 0x7fff) udp_seq = 0;
  usrp.setSeq(udp_seq);

  ostringstream os;
  if (!usrp.pack(os))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack UDP Usrp message\n";
    return;
  }

  sendUdpMessage(os);

  stringstream ss;
  ss << "switch_to_mode " << selected_mode[mode];
  processEvent(ss.str());
} /* UsrpLogic::switchMode */


void UsrpLogic::processEvent(const std::string& event)
{
  m_event_handler->processEvent(name() + "::" + event);
  checkIdle();
} /* UsrpLogic::processEvent */


void UsrpLogic::handlePlayFile(const std::string& path)
{
  setIdle(false);
  LinkManager::instance()->playFile(this, path);
} /* UsrpLogic::handlePlayFile */


void UsrpLogic::handlePlaySilence(int duration)
{
  setIdle(false);
  LinkManager::instance()->playSilence(this, duration);
} /* UsrpLogic::handlePlaySilence */


void UsrpLogic::handlePlayTone(int fq, int amp, int duration)
{
  setIdle(false);
  LinkManager::instance()->playTone(this, fq, amp, duration);
} /* UsrpLogic::handlePlayTone */


void UsrpLogic::handlePlayDtmf(const std::string& digit, int amp,
                                    int duration)
{
  setIdle(false);
  LinkManager::instance()->playDtmf(this, digit, amp, duration);
} /* UsrpLogic::handlePlayDtmf */


// receive interlogic messages here
void UsrpLogic::onPublishStateEvent(const string &event_name, const string &msg)
{
  //cout << "UsrpLogic::onPublishStateEvent - event_name: " << event_name
  //      << ", message: " << msg << endl;

  // if it is not allowed to handle information about users then all userinfo
  // traffic will be ignored
  if (!share_userinfo) return;

  Json::Value user_arr;
  Json::CharReaderBuilder rbuilder;
  std::unique_ptr<Json::CharReader> const reader(rbuilder.newCharReader());
  std::string jsonReaderError;
  
  if (!reader->parse(msg.c_str(), msg.c_str() + msg.size(), &user_arr,
                 &jsonReaderError))
  {
    if (debug >= LOGERROR)
    {
      cout << "*** Error: parsing StateEvent message (" 
           << jsonReaderError << ")" << endl;
    }
    return;
  }

  if (event_name == "DvUsers:info")
  {
    if (debug >= LOGINFO)
    {
      cout << "Download userdata from Reflector (DvUsers:info):" << endl;
    }
    for (Json::Value::ArrayIndex i = 0; i != user_arr.size(); i++)
    {
      User m_user;
      Json::Value& t_userdata = user_arr[i];
      m_user.id = t_userdata.get("id", "").asString();
      m_user.mode = t_userdata.get("mode","").asString();
      m_user.name = t_userdata.get("name","").asString();
      m_user.call = t_userdata.get("call","").asString();
      m_user.location = t_userdata.get("location","").asString();
      m_user.aprs_sym = static_cast<char>(t_userdata.get("sym", 46).asInt());
      m_user.aprs_tab = static_cast<char>(t_userdata.get("tab", 47).asInt());
      m_user.comment = t_userdata.get("comment","").asString();
      m_user.last_activity = t_userdata.get("last_activity",0).asUInt();

      userdata[m_user.id] = m_user;
      if (debug >= LOGINFO)
      {
        cout << "id:" << m_user.id << ",call=" << m_user.call << ",name="
             << m_user.name << ",location=" << m_user.location 
             << ", comment=" << m_user.comment << endl;
      }
    }
  }
  else if (event_name == "EchoLink:talker_state")
  {
    talker.callsign = user_arr.get("callsign","N0CALL").asString();
    talker.name = user_arr.get("name","No EL-Name").asString();
    talker.isTalking = user_arr.get("isTalking",false).asBool();
    if (debug >= LOGINFO)
    {
      cout << "Get userinfo from EchoLink module("
           << talker.callsign << "," << talker.name
           << (talker.isTalking ? ",1" : ",0") << ")" << endl;
    }
  }
} /* UsrpLogic::onPublishStateEvent */


void UsrpLogic::sendUserInfo(void)
{

  // read infos of Dv users configured in svxlink.conf
  Json::Value event(Json::arrayValue);

  for (std::map<std::string, User>::iterator iu = userdata.begin(); 
       iu!=userdata.end(); iu++)
  {
    Json::Value t_userinfo(Json::objectValue);
    t_userinfo["id"] = iu->second.id;
    t_userinfo["mode"] = iu->second.mode;
    t_userinfo["call"] = iu->second.call;
    t_userinfo["name"] = iu->second.name;
    t_userinfo["tab"] = iu->second.aprs_tab;
    t_userinfo["sym"] = iu->second.aprs_sym;
    t_userinfo["comment"] = iu->second.comment;
    t_userinfo["location"] = iu->second.location;
    t_userinfo["last_activity"] = 0;
    event.append(t_userinfo);
  }
  publishInfo("DvUsers:info", event);
} /* UsrpLogic::sendUserInfo */


void UsrpLogic::publishInfo(std::string type, Json::Value event)
{
  // if it is not allowed to handle information about users then all userinfo traffic 
  // will be ignored
  if (!share_userinfo) return;

   // sending own Dv user information to the reflectorlogic network
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = ""; //The JSON document is written on a single line
  Json::StreamWriter* writer = builder.newStreamWriter();
  std::stringstream os;
  writer->write(event, &os);
  delete writer;
  publishStateEvent(type, os.str());
} /* UsrpLogic::publishInfo */


/*
 * This file has not been truncated
 */
