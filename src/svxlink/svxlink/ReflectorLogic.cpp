/**
@file	 ReflectorLogic.cpp
@brief   A logic core that connect to the SvxReflector
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-12

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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncUdpSocket.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioValve.h>
#include <version/SVXLINK.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ReflectorLogic.h"
#include "../reflector/ReflectorMsg.h"
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

ReflectorLogic::ReflectorLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_con(0), m_msg_type(0), m_udp_sock(0),
    m_logic_con_in(0), m_logic_con_out(0),
    m_reconnect_timer(20000, Timer::TYPE_ONESHOT, false),
    m_next_udp_tx_seq(0), m_next_udp_rx_seq(0),
    m_heartbeat_timer(1000, Timer::TYPE_PERIODIC, false), m_dec(0),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_udp_heartbeat_tx_cnt(0), m_udp_heartbeat_rx_cnt(0),
    m_tcp_heartbeat_tx_cnt(0), m_tcp_heartbeat_rx_cnt(0),
    m_con_state(STATE_DISCONNECTED), m_enc(0), m_default_tg(0),
    m_tg_select_timeout(DEFAULT_TG_SELECT_TIMEOUT),
    m_tg_select_timer(1000, Async::Timer::TYPE_PERIODIC),
    m_tg_select_timeout_cnt(0), m_selected_tg(0), m_previous_tg(0),
    m_event_handler(0),
    m_report_tg_timer(500, Async::Timer::TYPE_ONESHOT, false),
    m_tg_local_activity(false), m_last_qsy(0), m_logic_con_in_valve(0),
    m_mute_first_tx_loc(true), m_mute_first_tx_rem(false),
    m_tmp_monitor_timer(1000, Async::Timer::TYPE_PERIODIC),
    m_tmp_monitor_timeout(DEFAULT_TMP_MONITOR_TIMEOUT)
{
  m_reconnect_timer.expired.connect(
      sigc::hide(mem_fun(*this, &ReflectorLogic::reconnect)));
  m_heartbeat_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::handleTimerTick));
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &ReflectorLogic::flushTimeout));
  timerclear(&m_last_talker_timestamp);

  m_tg_select_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::tgSelectTimerExpired)));
  m_report_tg_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::processTgSelectionEvent)));
  m_tmp_monitor_timer.expired.connect(sigc::hide(
        sigc::mem_fun(*this, &ReflectorLogic::checkTmpMonitorTimeout)));
} /* ReflectorLogic::ReflectorLogic */


ReflectorLogic::~ReflectorLogic(void)
{
  delete m_event_handler;
  m_event_handler = 0;
  delete m_udp_sock;
  m_udp_sock = 0;
  delete m_logic_con_in;
  m_logic_con_in = 0;
  delete m_enc;
  m_enc = 0;
  delete m_dec;
  m_dec = 0;
  delete m_con;
  m_con = 0;
} /* ReflectorLogic::~ReflectorLogic */


bool ReflectorLogic::initialize(void)
{
  if (!cfg().getValue(name(), "HOST", m_reflector_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration" << endl;
    return false;
  }

  m_reflector_port = 5300;
  cfg().getValue(name(), "PORT", m_reflector_port);

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: " << name() << "/CALLSIGN missing in configuration"
         << endl;
    return false;
  }

  if (!cfg().getValue(name(), "AUTH_KEY", m_auth_key))
  {
    cerr << "*** ERROR: " << name() << "/AUTH_KEY missing in configuration"
         << endl;
    return false;
  }
  if (m_auth_key == "Change this key now!")
  {
    cerr << "*** ERROR: You must change " << name() << "/AUTH_KEY from the "
            "default value" << endl;
    return false;
  }

  string event_handler_str;
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str))
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/EVENT_HANDLER not set\n";
    return false;
  }

  std::vector<std::string> monitor_tgs;
  cfg().getValue(name(), "MONITOR_TGS", monitor_tgs);
  for (std::vector<std::string>::iterator it=monitor_tgs.begin();
       it!=monitor_tgs.end(); ++it)
  {
    std::istringstream is(*it);
    MonitorTgEntry mte;
    is >> mte.tg;
    char modifier;
    while (is >> modifier)
    {
      if (modifier == '+')
      {
        mte.prio += 1;
      }
      else
      {
        cerr << "*** ERROR: Illegal format for config variable MONITOR_TGS "
             << "entry \"" << *it << "\"" << endl;
        return false;
      }
    }
    m_monitor_tgs.insert(mte);
  }

#if 0
  string audio_codec("GSM");
  if (AudioDecoder::isAvailable("OPUS") && AudioEncoder::isAvailable("OPUS"))
  {
    audio_codec = "OPUS";
  }
  else if (AudioDecoder::isAvailable("SPEEX") &&
           AudioEncoder::isAvailable("SPEEX"))
  {
    audio_codec = "SPEEX";
  }
  cfg().getValue(name(), "AUDIO_CODEC", audio_codec);
#endif

    // Create logic connection incoming audio passthrough
  m_logic_con_in = new Async::AudioStreamStateDetector;
  m_logic_con_in->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onLogicConInStreamStateChanged));
  AudioSource *prev_src = m_logic_con_in;

  cfg().getValue(name(), "MUTE_FIRST_TX_LOC", m_mute_first_tx_loc);
  cfg().getValue(name(), "MUTE_FIRST_TX_REM", m_mute_first_tx_rem);
  if (m_mute_first_tx_loc || m_mute_first_tx_rem)
  {
    m_logic_con_in_valve = new Async::AudioValve;
    m_logic_con_in_valve->setOpen(false);
    prev_src->registerSink(m_logic_con_in_valve);
    prev_src = m_logic_con_in_valve;
  }

  m_enc_endpoint = prev_src;
  prev_src = 0;

    // Create dummy audio codec used before setting the real encoder
  if (!setAudioCodec("DUMMY")) { return false; }
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

  m_logic_con_out = new Async::AudioStreamStateDetector;
  m_logic_con_out->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &ReflectorLogic::onLogicConOutStreamStateChanged));
  prev_src->registerSink(m_logic_con_out, true);
  prev_src = 0;

  cfg().getValue(name(), "DEFAULT_TG", m_default_tg);
  cfg().getValue(name(), "TG_SELECT_TIMEOUT", m_tg_select_timeout);

  m_event_handler = new EventHandler(event_handler_str, name());
  if (LinkManager::hasInstance())
  {
    m_event_handler->playFile.connect(sigc::bind<0>(
          mem_fun(LinkManager::instance(), &LinkManager::playFile), this));
    m_event_handler->playSilence.connect(sigc::bind<0>(
          mem_fun(LinkManager::instance(), &LinkManager::playSilence), this));
    m_event_handler->playTone.connect(sigc::bind<0>(
          mem_fun(LinkManager::instance(), &LinkManager::playTone), this));
    m_event_handler->playDtmf.connect(sigc::bind<0>(
          mem_fun(LinkManager::instance(), &LinkManager::playDtmf), this));
  }
  m_event_handler->setVariable("logic_name", name().c_str());

  m_event_handler->processEvent("namespace eval Logic {}");
  list<string> cfgvars = cfg().listSection(name());
  list<string>::const_iterator cfgit;
  for (cfgit=cfgvars.begin(); cfgit!=cfgvars.end(); ++cfgit)
  {
    string var = "Logic::CFG_" + *cfgit;
    string value;
    cfg().getValue(name(), *cfgit, value);
    m_event_handler->setVariable(var, value);
  }

  if (!m_event_handler->initialize())
  {
    return false;
  }

  cfg().getValue(name(), "TMP_MONITOR_TIMEOUT", m_tmp_monitor_timeout);

  std::string node_info_file;
  if (cfg().getValue(name(), "NODE_INFO_FILE", node_info_file))
  {
    std::ifstream node_info_is(node_info_file.c_str(), std::ios::in);
    if (node_info_is.good())
    {
      try
      {
        if (!(node_info_is >> m_node_info))
        {
          std::cerr << "*** ERROR: Failure while reading node information file "
                       "\"" << node_info_file << "\""
                    << std::endl;
          return false;
        }
      }
      catch (const Json::Exception& e)
      {
        std::cerr << "*** ERROR: Failure while reading node information "
                     "file \"" << node_info_file << "\": "
                  << e.what()
                  << std::endl;
        return false;
      }
    }
    else
    {
      std::cerr << "*** ERROR: Could not open node information file "
                   "\"" << node_info_file << "\""
                << std::endl;
      return false;
    }
  }
  m_node_info["sw"] = "SvxLink";
  m_node_info["swVer"] = SVXLINK_VERSION;

  if (!LogicBase::initialize())
  {
    return false;
  }

  connect();

  return true;
} /* ReflectorLogic::initialize */


void ReflectorLogic::remoteCmdReceived(LogicBase* src_logic,
                                       const std::string& cmd)
{
  //cout << "### src_logic=" << src_logic->name() << "  cmd=" << cmd << endl;
  if (cmd == "*")
  {
    processEvent("report_tg_status");
  }
  //else if (cmd[0] == '0') // Help
  //{

  //}
  else if (cmd[0] == '1') // Select TG
  {
    const std::string subcmd(cmd.substr(1));
    if (!subcmd.empty()) // Select specified TG
    {
      istringstream is(subcmd);
      uint32_t tg;
      if (is >> tg)
      {
        selectTg(tg, "tg_command_activation", true);
        m_tg_local_activity = true;
      }
      else
      {
        processEvent(std::string("command_failed ") + cmd);
      }
    }
    else // Select previous TG
    {
      selectTg(m_previous_tg, "tg_command_activation", true);
      m_tg_local_activity = true;
    }
  }
  else if (cmd[0] == '2')   // QSY
  {
    if ((m_selected_tg != 0) && isLoggedIn())
    {
      const std::string subcmd(cmd.substr(1));
      if (subcmd.empty())
      {
        cout << name() << ": Requesting QSY to random TG" << endl;
        sendMsg(MsgRequestQsy());
      }
      else
      {
        std::istringstream is(subcmd);
        uint32_t tg = 0;
        if (is >> tg)
        {
          cout << name() << ": Requesting QSY to TG #" << tg << endl;
          sendMsg(MsgRequestQsy(tg));
        }
        else
        {
          processEvent("tg_qsy_idle");
        }
      }
    }
    else
    {
      processEvent("tg_qsy_idle");
    }
  }
  else if (cmd == "3")   // Follow last QSY
  {
    if ((m_last_qsy > 0) && (m_last_qsy != m_selected_tg))
    {
      selectTg(m_last_qsy, "tg_command_activation", true);
      m_tg_local_activity = true;
    }
    else
    {
      processEvent(std::string("command_failed ") + cmd);
    }
  }
  else if (cmd[0] == '4')   // Temporarily monitor talk group
  {
    const std::string subcmd(cmd.substr(1));
    if ((m_tmp_monitor_timeout > 0) && !subcmd.empty())
    {
      istringstream is(subcmd);
      uint32_t tg = 0;
      if (is >> tg)
      {
        const MonitorTgsSet::iterator it = m_monitor_tgs.find(tg);
        if (it != m_monitor_tgs.end())
        {
          std::cout << name() << ": Refresh temporary monitor for TG #"
                    << tg << std::endl;
            // NOTE: (*it).timeout is mutable
          (*it).timeout = m_tmp_monitor_timeout;
        }
        else
        {
          std::cout << name() << ": Add temporary monitor for TG #"
                    << tg << std::endl;
          MonitorTgEntry mte(tg);
          mte.timeout = m_tmp_monitor_timeout;
          m_monitor_tgs.insert(mte);
          sendMsg(MsgTgMonitor(std::set<uint32_t>(
                  m_monitor_tgs.begin(), m_monitor_tgs.end())));
        }
        std::ostringstream os;
        os << "tmp_monitor_add " << tg;
        processEvent(os.str());
      }
      else
      {
        processEvent(std::string("command_failed ") + cmd);
      }
    }
    else
    {
      processEvent(std::string("command_failed ") + cmd);
    }
  }
  else
  {
    processEvent(std::string("unknown_command ") + cmd);
  }
} /* ReflectorLogic::remoteCmdReceived */


void ReflectorLogic::remoteReceivedTgUpdated(LogicBase *logic, uint32_t tg)
{
  //cout << "### ReflectorLogic::remoteReceivedTgUpdated: logic="
  //     << logic->name() << "  tg=" << tg << endl;
  if ((m_selected_tg == 0) && (tg > 0))
  {
    selectTg(tg, "tg_local_activation", !m_mute_first_tx_loc);
    m_tg_local_activity = true;
  }
} /* ReflectorLogic::remoteReceivedTgUpdated */


void ReflectorLogic::remoteReceivedPublishStateEvent(
    LogicBase *logic, const std::string& event_name, const std::string& data)
{
  //cout << "### ReflectorLogic::remoteReceivedPublishStateEvent:"
  //     << " logic=" << logic->name()
  //     << " event_name=" << event_name
  //     << " data=" << data
  //     << endl;
  //sendMsg(MsgStateEvent(logic->name(), event_name, msg));

  if (event_name == "Voter:sql_state")
  {
    //MsgUdpSignalStrengthValues msg;
    MsgSignalStrengthValues msg;
    std::istringstream is(data);
    Json::Value rx_arr;
    is >> rx_arr;
    for (Json::Value::ArrayIndex i = 0; i != rx_arr.size(); i++)
    {
      Json::Value& rx_data = rx_arr[i];
      std::string name = rx_data.get("name", "").asString();
      std::string id_str = rx_data.get("id", "?").asString();
      if (id_str.size() != 1)
      {
        return;
      }
      char id = id_str[0];
      int siglev = rx_data.get("siglev", 0).asInt();
      siglev = std::min(std::max(siglev, 0), 100);
      bool is_enabled = rx_data.get("enabled", false).asBool();
      bool sql_open = rx_data.get("sql_open", false).asBool();
      bool is_active = rx_data.get("active", false).asBool();
      //MsgUdpSignalStrengthValues::Rx rx(id, siglev);
      MsgSignalStrengthValues::Rx rx(id, siglev);
      rx.setEnabled(is_enabled);
      rx.setSqlOpen(sql_open);
      rx.setActive(is_active);
      msg.pushBack(rx);
    }
    //sendUdpMsg(msg);
    sendMsg(msg);
  }
  else if (event_name == "Rx:sql_state")
  {
    //MsgUdpSignalStrengthValues msg;
    MsgSignalStrengthValues msg;
    std::istringstream is(data);
    Json::Value rx_data;
    is >> rx_data;
    std::string name = rx_data.get("name", "").asString();
    std::string id_str = rx_data.get("id", "?").asString();
    if (id_str.size() != 1)
    {
      return;
    }
    char id = id_str[0];
    int siglev = rx_data.get("siglev", 0).asInt();
    siglev = std::min(std::max(siglev, 0), 100);
    bool sql_open = rx_data.get("sql_open", false).asBool();
    //MsgUdpSignalStrengthValues::Rx rx(id, siglev);
    MsgSignalStrengthValues::Rx rx(id, siglev);
    rx.setEnabled(true);
    rx.setSqlOpen(sql_open);
    rx.setActive(sql_open);
    msg.pushBack(rx);
    //sendUdpMsg(msg);
    sendMsg(msg);
  }
  else if (event_name == "Tx:state")
  {
    MsgTxStatus msg;
    std::istringstream is(data);
    Json::Value tx_data;
    is >> tx_data;
    std::string name = tx_data.get("name", "").asString();
    std::string id_str = tx_data.get("id", "?").asString();
    if (id_str.size() != 1)
    {
      return;
    }
    char id = id_str[0];
    if (id != '\0')
    {
      bool transmit = tx_data.get("transmit", false).asBool();
      MsgTxStatus::Tx tx(id);
      tx.setTransmit(transmit);
      msg.pushBack(tx);
      sendMsg(msg);
    }
  }
  else if (event_name == "MultiTx:state")
  {
    MsgTxStatus msg;
    std::istringstream is(data);
    Json::Value tx_arr;
    is >> tx_arr;
    for (Json::Value::ArrayIndex i = 0; i != tx_arr.size(); i++)
    {
      Json::Value& tx_data = tx_arr[i];
      std::string name = tx_data.get("name", "").asString();
      std::string id_str = tx_data.get("id", "").asString();
      if (id_str.size() != 1)
      {
        return;
      }
      char id = id_str[0];
      if (id != '\0')
      {
        bool transmit = tx_data.get("transmit", false).asBool();
        MsgTxStatus::Tx tx(id);
        tx.setTransmit(transmit);
        msg.pushBack(tx);
      }
    }
    sendMsg(msg);
  }
} /* ReflectorLogic::remoteReceivedPublishStateEvent */


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

void ReflectorLogic::onConnected(void)
{
  cout << name() << ": Connection established to " << m_con->remoteHost() << ":"
       << m_con->remotePort() << endl;
  sendMsg(MsgProtoVer());
  m_udp_heartbeat_tx_cnt = UDP_HEARTBEAT_TX_CNT_RESET;
  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;
  m_tcp_heartbeat_tx_cnt = TCP_HEARTBEAT_TX_CNT_RESET;
  m_tcp_heartbeat_rx_cnt = TCP_HEARTBEAT_RX_CNT_RESET;
  m_heartbeat_timer.setEnable(true);
  m_next_udp_tx_seq = 0;
  m_next_udp_rx_seq = 0;
  timerclear(&m_last_talker_timestamp);
  m_con_state = STATE_EXPECT_AUTH_CHALLENGE;
  m_con->setMaxFrameSize(ReflectorMsg::MAX_PREAUTH_FRAME_SIZE);
} /* ReflectorLogic::onConnected */


void ReflectorLogic::onDisconnected(TcpConnection *con,
                                    TcpConnection::DisconnectReason reason)
{
  cout << name() << ": Disconnected from " << m_con->remoteHost() << ":"
       << m_con->remotePort() << ": "
       << TcpConnection::disconnectReasonStr(reason) << endl;
  m_reconnect_timer.setTimeout(1000 + std::rand() % 5000);
  m_reconnect_timer.setEnable(true);
  delete m_udp_sock;
  m_udp_sock = 0;
  m_next_udp_tx_seq = 0;
  m_next_udp_rx_seq = 0;
  m_heartbeat_timer.setEnable(false);
  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
    m_enc->allEncodedSamplesFlushed();
  }
  if (timerisset(&m_last_talker_timestamp))
  {
    m_dec->flushEncodedSamples();
    timerclear(&m_last_talker_timestamp);
  }
  m_con_state = STATE_DISCONNECTED;
} /* ReflectorLogic::onDisconnected */


void ReflectorLogic::onFrameReceived(FramedTcpConnection *con,
                                     std::vector<uint8_t>& data)
{
  char *buf = reinterpret_cast<char*>(&data.front());
  int len = data.size();

  stringstream ss;
  ss.write(buf, len);

  ReflectorMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** ERROR[" << name()
         << "]: Unpacking failed for TCP message header\n";
    disconnect();
    return;
  }

  if ((header.type() > 100) && !isLoggedIn())
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected protocol message received"
         << endl;
    disconnect();
    return;
  }

  m_tcp_heartbeat_rx_cnt = TCP_HEARTBEAT_RX_CNT_RESET;

  switch (header.type())
  {
    case MsgHeartbeat::TYPE:
      break;
    case MsgError::TYPE:
      handleMsgError(ss);
      break;
    case MsgProtoVerDowngrade::TYPE:
      handleMsgProtoVerDowngrade(ss);
      break;
    case MsgAuthChallenge::TYPE:
      handleMsgAuthChallenge(ss);
      break;
    case MsgAuthOk::TYPE:
      handleMsgAuthOk();
      break;
    case MsgServerInfo::TYPE:
      handleMsgServerInfo(ss);
      break;
    case MsgNodeList::TYPE:
      handleMsgNodeList(ss);
      break;
    case MsgNodeJoined::TYPE:
      handleMsgNodeJoined(ss);
      break;
    case MsgNodeLeft::TYPE:
      handleMsgNodeLeft(ss);
      break;
    case MsgTalkerStart::TYPE:
      handleMsgTalkerStart(ss);
      break;
    case MsgTalkerStop::TYPE:
      handleMsgTalkerStop(ss);
      break;
    case MsgRequestQsy::TYPE:
      handleMsgRequestQsy(ss);
      break;
    default:
      // Better just ignoring unknown messages for easier addition of protocol
      // messages while being backwards compatible

      //cerr << "*** WARNING[" << name()
      //     << "]: Unknown protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* ReflectorLogic::onFrameReceived */


void ReflectorLogic::handleMsgError(std::istream& is)
{
  MsgError msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgAuthError" << endl;
    disconnect();
    return;
  }
  cout << name() << ": Error message received from server: " << msg.message()
       << endl;
  disconnect();
} /* ReflectorLogic::handleMsgError */


void ReflectorLogic::handleMsgProtoVerDowngrade(std::istream& is)
{
  MsgProtoVerDowngrade msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgProtoVerDowngrade" << endl;
    disconnect();
    return;
  }
  cout << name() << ": Server too old and we cannot downgrade to protocol version "
       << msg.majorVer() << "." << msg.minorVer() << " from "
       << MsgProtoVer::MAJOR << "." << MsgProtoVer::MINOR
       << endl;
  disconnect();
} /* ReflectorLogic::handleMsgProtoVerDowngrade */


void ReflectorLogic::handleMsgAuthChallenge(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_AUTH_CHALLENGE)
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected MsgAuthChallenge\n";
    disconnect();
    return;
  }

  MsgAuthChallenge msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgAuthChallenge\n";
    disconnect();
    return;
  }
  const uint8_t *challenge = msg.challenge();
  if (challenge == 0)
  {
    cerr << "*** ERROR[" << name() << "]: Illegal challenge received\n";
    disconnect();
    return;
  }
  sendMsg(MsgAuthResponse(m_callsign, m_auth_key, challenge));
  m_con_state = STATE_EXPECT_AUTH_OK;
} /* ReflectorLogic::handleMsgAuthChallenge */


void ReflectorLogic::handleMsgAuthOk(void)
{
  if (m_con_state != STATE_EXPECT_AUTH_OK)
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected MsgAuthOk\n";
    disconnect();
    return;
  }
  cout << name() << ": Authentication OK" << endl;
  m_con_state = STATE_EXPECT_SERVER_INFO;
  m_con->setMaxFrameSize(ReflectorMsg::MAX_POSTAUTH_FRAME_SIZE);
} /* ReflectorLogic::handleMsgAuthOk */


void ReflectorLogic::handleMsgServerInfo(std::istream& is)
{
  if (m_con_state != STATE_EXPECT_SERVER_INFO)
  {
    cerr << "*** ERROR[" << name() << "]: Unexpected MsgServerInfo\n";
    disconnect();
    return;
  }
  MsgServerInfo msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgServerInfo\n";
    disconnect();
    return;
  }
  m_client_id = msg.clientId();

  //cout << "### MsgServerInfo: clientId=" << msg.clientId()
  //     << " codecs=";
  //std::copy(msg.codecs().begin(), msg.codecs().end(),
  //     std::ostream_iterator<std::string>(cout, " "));
  //cout << " nodes=";
  //std::copy(msg.nodes().begin(), msg.nodes().end(),
  //     std::ostream_iterator<std::string>(cout, " "));
  //cout << endl;

  cout << name() << ": Connected nodes: ";
  const vector<string>& nodes = msg.nodes();
  if (!nodes.empty())
  {
    vector<string>::const_iterator it = nodes.begin();
    cout << *it++;
    for (; it != nodes.end(); ++it)
    {
      cout << ", " << *it;
    }
  }
  cout << endl;

  string selected_codec;
  for (vector<string>::const_iterator it = msg.codecs().begin();
       it != msg.codecs().end();
       ++it)
  {
    if (codecIsAvailable(*it))
    {
      selected_codec = *it;
      setAudioCodec(selected_codec);
      break;
    }
  }
  cout << name() << ": ";
  if (!selected_codec.empty())
  {
    cout << "Using audio codec \"" << selected_codec << "\"";
  }
  else
  {
    cout << "No supported codec :-(";
  }
  cout << endl;

  delete m_udp_sock;
  m_udp_sock = new UdpSocket;
  m_udp_sock->dataReceived.connect(
      mem_fun(*this, &ReflectorLogic::udpDatagramReceived));

  m_con_state = STATE_CONNECTED;

  std::ostringstream node_info_os;
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = ""; //The JSON document is written on a single line
  Json::StreamWriter* writer = builder.newStreamWriter();
  writer->write(m_node_info, &node_info_os);
  delete writer;
  MsgNodeInfo node_info_msg(node_info_os.str());
  sendMsg(node_info_msg);

#if 0
    // Set up RX and TX sites node information
  MsgNodeInfo::RxSite rx_site;
  MsgNodeInfo::TxSite tx_site;
  rx_site.setRxName("Rx1");
  tx_site.setTxName("Tx1");
  rx_site.setQthName(cfg().getValue("LocationInfo", "QTH_NAME"));
  tx_site.setQthName(cfg().getValue("LocationInfo", "QTH_NAME"));
  int32_t antenna_height = 0;
  if (cfg().getValue("LocationInfo", "ANTENNA_HEIGHT", antenna_height))
  {
    rx_site.setAntennaHeight(antenna_height);
    tx_site.setAntennaHeight(antenna_height);
  }
  float antenna_dir = -1.0f;
  cfg().getValue("LocationInfo", "ANTENNA_DIR", antenna_dir);
  rx_site.setAntennaDirection(antenna_dir);
  tx_site.setAntennaDirection(antenna_dir);
  double rf_frequency = 0;
  cfg().getValue("LocationInfo", "FREQUENCY", rf_frequency);
  if (rf_frequency < 0.0f)
  {
    rf_frequency = 0.0f;
  }
  rx_site.setRfFrequency(static_cast<uint64_t>(1000000.0f * rf_frequency));
  tx_site.setRfFrequency(static_cast<uint64_t>(1000000.0f * rf_frequency));
  vector<float> ctcss_frequencies;
  //ctcss_frequencies.push_back(136.5);
  //rx_site.setCtcssFrequencies(ctcss_frequencies);
  //tx_site.setCtcssFrequencies(ctcss_frequencies);
  float tx_power = 0.0f;
  cfg().getValue("LocationInfo", "TX_POWER", tx_power);
  tx_site.setTxPower(tx_power);
  MsgNodeInfo::TxSites tx_sites;
  tx_sites.push_back(tx_site);
  MsgNodeInfo::RxSites rx_sites;
  rx_sites.push_back(rx_site);

    // Send node information to the server
  MsgNodeInfo node_info; node_info
    .setSwInfo("SvxLink v" SVXLINK_VERSION)
    .setTxSites(tx_sites)
    .setRxSites(rx_sites);
  sendMsg(node_info);
#endif

  if (m_selected_tg > 0)
  {
    cout << name() << ": Selecting TG #" << m_selected_tg << endl;
    sendMsg(MsgSelectTG(m_selected_tg));
  }

  if (!m_monitor_tgs.empty())
  {
    sendMsg(MsgTgMonitor(
          std::set<uint32_t>(m_monitor_tgs.begin(), m_monitor_tgs.end())));
  }
  sendUdpMsg(MsgUdpHeartbeat());

} /* ReflectorLogic::handleMsgServerInfo */


void ReflectorLogic::handleMsgNodeList(std::istream& is)
{
  MsgNodeList msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeList\n";
    disconnect();
    return;
  }
  cout << name() << ": Connected nodes: ";
  const vector<string>& nodes = msg.nodes();
  if (!nodes.empty())
  {
    vector<string>::const_iterator it = nodes.begin();
    cout << *it++;
    for (; it != nodes.end(); ++it)
    {
      cout << ", " << *it;
    }
  }
  cout << endl;
} /* ReflectorLogic::handleMsgNodeList */


void ReflectorLogic::handleMsgNodeJoined(std::istream& is)
{
  MsgNodeJoined msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeJoined\n";
    disconnect();
    return;
  }
  cout << name() << ": Node joined: " << msg.callsign() << endl;
} /* ReflectorLogic::handleMsgNodeJoined */


void ReflectorLogic::handleMsgNodeLeft(std::istream& is)
{
  MsgNodeLeft msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgNodeLeft\n";
    disconnect();
    return;
  }
  cout << name() << ": Node left: " << msg.callsign() << endl;
} /* ReflectorLogic::handleMsgNodeLeft */


void ReflectorLogic::handleMsgTalkerStart(std::istream& is)
{
  MsgTalkerStart msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgTalkerStart\n";
    disconnect();
    return;
  }
  cout << name() << ": Talker start on TG #" << msg.tg() << ": "
       << msg.callsign() << endl;

    // Select the incoming TG if idle
  if (m_tg_select_timeout_cnt == 0)
  {
    selectTg(msg.tg(), "tg_remote_activation", !m_mute_first_tx_rem);
  }
  else
  {
    uint32_t selected_tg_prio = 0;
    MonitorTgsSet::const_iterator selected_tg_it =
      m_monitor_tgs.find(MonitorTgEntry(m_selected_tg));
    if (selected_tg_it != m_monitor_tgs.end())
    {
      selected_tg_prio = selected_tg_it->prio;
    }
    MonitorTgsSet::const_iterator talker_tg_it =
      m_monitor_tgs.find(MonitorTgEntry(msg.tg()));
    if ((talker_tg_it != m_monitor_tgs.end()) &&
        (talker_tg_it->prio > selected_tg_prio) &&
        !m_tg_local_activity)
    {
      std::cout << name() << ": Activity on prioritized TG #"
                << msg.tg() << ". Switching!" << std::endl;
      selectTg(msg.tg(), "tg_remote_prio_activation", !m_mute_first_tx_rem);
    }
  }

  std::ostringstream ss;
  ss << "talker_start " << msg.tg() << " " << msg.callsign();
  processEvent(ss.str());
} /* ReflectorLogic::handleMsgTalkerStart */


void ReflectorLogic::handleMsgTalkerStop(std::istream& is)
{
  MsgTalkerStop msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgTalkerStop\n";
    disconnect();
    return;
  }
  cout << name() << ": Talker stop on TG #" << msg.tg() << ": "
       << msg.callsign() << endl;

  std::ostringstream ss;
  ss << "talker_stop " << msg.tg() << " " << msg.callsign();
  processEvent(ss.str());
} /* ReflectorLogic::handleMsgTalkerStop */


void ReflectorLogic::handleMsgRequestQsy(std::istream& is)
{
  MsgRequestQsy msg;
  if (!msg.unpack(is))
  {
    cerr << "*** ERROR[" << name() << "]: Could not unpack MsgRequestQsy\n";
    disconnect();
    return;
  }
  cout << name() << ": Server QSY request for TG #" << msg.tg() << endl;
  if (m_tg_local_activity)
  {
    selectTg(msg.tg(), "tg_qsy", true);
  }
  else
  {
    m_last_qsy = msg.tg();
    cout << name()
         << ": Server QSY request ignored. No local activity." << endl;
    std::ostringstream os;
    os << "tg_qsy_ignored " << msg.tg();
    processEvent(os.str());
  }
} /* ReflectorLogic::handleMsgRequestQsy */


void ReflectorLogic::sendMsg(const ReflectorMsg& msg)
{
  if (!isConnected())
  {
    return;
  }

  m_tcp_heartbeat_tx_cnt = TCP_HEARTBEAT_TX_CNT_RESET;

  ostringstream ss;
  ReflectorMsg header(msg.type());
  if (!header.pack(ss) || !msg.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack reflector TCP message\n";
    disconnect();
    return;
  }
  if (m_con->write(ss.str().data(), ss.str().size()) == -1)
  {
    disconnect();
  }
} /* ReflectorLogic::sendMsg */


void ReflectorLogic::sendEncodedAudio(const void *buf, int count)
{
  if (!isLoggedIn())
  {
    return;
  }

  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }
  sendUdpMsg(MsgUdpAudio(buf, count));
} /* ReflectorLogic::sendEncodedAudio */


void ReflectorLogic::flushEncodedAudio(void)
{
  if (!isLoggedIn())
  {
    flushTimeout();
    return;
  }
  sendUdpMsg(MsgUdpFlushSamples());
  m_flush_timeout_timer.setEnable(true);
} /* ReflectorLogic::flushEncodedAudio */


void ReflectorLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  if (!isLoggedIn())
  {
    return;
  }

  if (addr != m_con->remoteHost())
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received from wrong source address "
         << addr << ". Should be " << m_con->remoteHost() << "." << endl;
    return;
  }
  if (port != m_con->remotePort())
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received with wrong source port number "
         << port << ". Should be " << m_con->remotePort() << "." << endl;
    return;
  }

  stringstream ss;
  ss.write(reinterpret_cast<const char *>(buf), count);

  ReflectorUdpMsg header;
  if (!header.unpack(ss))
  {
    cout << "*** WARNING[" << name()
         << "]: Unpacking failed for UDP message header" << endl;
    return;
  }

  if (header.clientId() != m_client_id)
  {
    cout << "*** WARNING[" << name()
         << "]: UDP packet received with wrong client id "
         << header.clientId() << ". Should be " << m_client_id << "." << endl;
    return;
  }

    // Check sequence number
  uint16_t udp_rx_seq_diff = header.sequenceNum() - m_next_udp_rx_seq;
  if (udp_rx_seq_diff > 0x7fff) // Frame out of sequence (ignore)
  {
    cout << name()
         << ": Dropping out of sequence UDP frame with seq="
         << header.sequenceNum() << endl;
    return;
  }
  else if (udp_rx_seq_diff > 0) // Frame lost
  {
    cout << name() << ": UDP frame(s) lost. Expected seq="
         << m_next_udp_rx_seq
         << " but received " << header.sequenceNum()
         << ". Resetting next expected sequence number to "
         << (header.sequenceNum() + 1) << endl;
  }
  m_next_udp_rx_seq = header.sequenceNum() + 1;

  m_udp_heartbeat_rx_cnt = UDP_HEARTBEAT_RX_CNT_RESET;

  switch (header.type())
  {
    case MsgUdpHeartbeat::TYPE:
      break;

    case MsgUdpAudio::TYPE:
    {
      MsgUdpAudio msg;
      if (!msg.unpack(ss))
      {
        cerr << "*** WARNING[" << name() << "]: Could not unpack MsgUdpAudio\n";
        return;
      }
      if (!msg.audioData().empty())
      {
        gettimeofday(&m_last_talker_timestamp, NULL);
        m_dec->writeEncodedSamples(
            &msg.audioData().front(), msg.audioData().size());
      }
      break;
    }

    case MsgUdpFlushSamples::TYPE:
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
      break;

    case MsgUdpAllSamplesFlushed::TYPE:
      m_enc->allEncodedSamplesFlushed();
      break;

    default:
      // Better ignoring unknown protocol messages for easier addition of new
      // messages while still being backwards compatible

      //cerr << "*** WARNING[" << name()
      //     << "]: Unknown UDP protocol message received: msg_type="
      //     << header.type() << endl;
      break;
  }
} /* ReflectorLogic::udpDatagramReceived */


void ReflectorLogic::sendUdpMsg(const ReflectorUdpMsg& msg)
{
  if (!isLoggedIn())
  {
    return;
  }

  m_udp_heartbeat_tx_cnt = UDP_HEARTBEAT_TX_CNT_RESET;

  if (m_udp_sock == 0)
  {
    return;
  }

  ReflectorUdpMsg header(msg.type(), m_client_id, m_next_udp_tx_seq++);
  ostringstream ss;
  if (!header.pack(ss) || !msg.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack reflector TCP message\n";
    return;
  }
  m_udp_sock->write(m_con->remoteHost(), m_con->remotePort(),
                    ss.str().data(), ss.str().size());
} /* ReflectorLogic::sendUdpMsg */


void ReflectorLogic::connect(void)
{
  if (!isConnected())
  {
    cout << name() << ": Connecting to " << m_reflector_host << ":"
         << m_reflector_port << endl;
    m_reconnect_timer.setEnable(false);
    m_con = new TcpClient<FramedTcpConnection>(m_reflector_host,
                                               m_reflector_port);
    m_con->connected.connect(
        mem_fun(*this, &ReflectorLogic::onConnected));
    m_con->disconnected.connect(
        mem_fun(*this, &ReflectorLogic::onDisconnected));
    m_con->frameReceived.connect(
        mem_fun(*this, &ReflectorLogic::onFrameReceived));
    m_con->connect();
  }
} /* ReflectorLogic::connect */


void ReflectorLogic::disconnect(void)
{
  if (m_con != 0)
  {
    if (m_con->isConnected())
    {
      m_con->disconnect();
      onDisconnected(m_con, TcpConnection::DR_ORDERED_DISCONNECT);
    }
    delete m_con;
    m_con = 0;
    m_con_state = STATE_DISCONNECTED;
  }
} /* ReflectorLogic::disconnect */


void ReflectorLogic::reconnect(void)
{
  disconnect();
  connect();
} /* ReflectorLogic::reconnect */


bool ReflectorLogic::isConnected(void) const
{
  return (m_con != 0) && m_con->isConnected();
} /* ReflectorLogic::isConnected */


void ReflectorLogic::allEncodedSamplesFlushed(void)
{
  sendUdpMsg(MsgUdpAllSamplesFlushed());
} /* ReflectorLogic::allEncodedSamplesFlushed */


void ReflectorLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_enc->allEncodedSamplesFlushed();
} /* ReflectorLogic::flushTimeout */


void ReflectorLogic::handleTimerTick(Async::Timer *t)
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

  if (--m_udp_heartbeat_tx_cnt == 0)
  {
    sendUdpMsg(MsgUdpHeartbeat());
  }

  if (--m_tcp_heartbeat_tx_cnt == 0)
  {
    sendMsg(MsgHeartbeat());
  }

  if (--m_udp_heartbeat_rx_cnt == 0)
  {
    cout << name() << ": UDP Heartbeat timeout" << endl;
    disconnect();
  }

  if (--m_tcp_heartbeat_rx_cnt == 0)
  {
    cout << name() << ": Heartbeat timeout" << endl;
    disconnect();
  }
} /* ReflectorLogic::handleTimerTick */


bool ReflectorLogic::setAudioCodec(const std::string& codec_name)
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
      mem_fun(*this, &ReflectorLogic::sendEncodedAudio));
  m_enc->flushEncodedSamples.connect(
      mem_fun(*this, &ReflectorLogic::flushEncodedAudio));
  m_enc_endpoint->registerSink(m_enc, false);

  string opt_prefix(m_enc->name());
  opt_prefix += "_ENC_";
  list<string> names = cfg().listSection(name());
  for (list<string>::const_iterator nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_enc->setOption(opt_name, opt_value);
    }
  }
  m_enc->printCodecParams();

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
      mem_fun(*this, &ReflectorLogic::allEncodedSamplesFlushed));
  if (sink != 0)
  {
    m_dec->registerSink(sink, true);
  }

  opt_prefix = string(m_dec->name()) + "_DEC_";
  names = cfg().listSection(name());
  for (list<string>::const_iterator nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg().getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      m_dec->setOption(opt_name, opt_value);
    }
  }
  m_dec->printCodecParams();

  return true;
} /* ReflectorLogic::setAudioCodec */


bool ReflectorLogic::codecIsAvailable(const std::string &codec_name)
{
  return AudioEncoder::isAvailable(codec_name) &&
         AudioDecoder::isAvailable(codec_name);
} /* ReflectorLogic::codecIsAvailable */


void ReflectorLogic::onLogicConInStreamStateChanged(bool is_active,
                                                    bool is_idle)
{
  //cout << "### ReflectorLogic::onLogicConInStreamStateChanged: is_active="
  //     << is_active << "  is_idle=" << is_idle << endl;
  if (is_idle)
  {
    if ((m_logic_con_in_valve != 0) && (m_selected_tg > 0))
    {
      m_logic_con_in_valve->setOpen(true);
    }
  }
  else
  {
    if (m_tg_select_timeout_cnt == 0) // No TG currently selected
    {
      if (m_default_tg > 0)
      {
        selectTg(m_default_tg, "tg_default_activation", !m_mute_first_tx_loc);
      }
    }
    m_tg_local_activity = true;
    m_tg_select_timeout_cnt = m_tg_select_timeout;
  }

  if (!m_tg_selection_event.empty())
  {
    //processTgSelectionEvent();
    m_report_tg_timer.reset();
    m_report_tg_timer.setEnable(true);
  }
} /* ReflectorLogic::onLogicConInStreamStateChanged */


void ReflectorLogic::onLogicConOutStreamStateChanged(bool is_active,
                                                     bool is_idle)
{
  //cout << "### ReflectorLogic::onLogicConOutStreamStateChanged: is_active="
  //     << is_active << "  is_idle=" << is_idle << endl;
  if (!is_idle && (m_tg_select_timeout_cnt > 0))
  {
    m_tg_select_timeout_cnt = m_tg_select_timeout;
  }

  if (!m_tg_selection_event.empty())
  {
    //processTgSelectionEvent();
    m_report_tg_timer.reset();
    m_report_tg_timer.setEnable(true);
  }
} /* ReflectorLogic::onLogicConOutStreamStateChanged */


void ReflectorLogic::tgSelectTimerExpired(void)
{
  //cout << "### ReflectorLogic::tgSelectTimerExpired: m_tg_select_timeout_cnt="
  //     << m_tg_select_timeout_cnt << endl;
  if (m_tg_select_timeout_cnt > 0)
  {
    if (m_logic_con_out->isIdle() && m_logic_con_in->isIdle() &&
        (--m_tg_select_timeout_cnt == 0))
    {
      selectTg(0, "tg_selection_timeout", false);
    }
  }
} /* ReflectorLogic::tgSelectTimerExpired */


void ReflectorLogic::selectTg(uint32_t tg, const std::string& event, bool unmute)
{
  cout << name() << ": Selecting TG #" << tg << endl;

  ostringstream os;
  os << event << " " << tg << " " << m_selected_tg;
  m_tg_selection_event = os.str();
  m_report_tg_timer.reset();
  m_report_tg_timer.setEnable(true);

  if (tg != m_selected_tg)
  {
    sendMsg(MsgSelectTG(tg));
    if (m_selected_tg != 0)
    {
      m_previous_tg = m_selected_tg;
    }
    m_selected_tg = tg;
    m_tg_local_activity = false;
    m_event_handler->setVariable(name() + "::selected_tg", m_selected_tg);
    m_event_handler->setVariable(name() + "::previous_tg", m_previous_tg);
  }
  m_tg_select_timeout_cnt = (tg > 0) ? m_tg_select_timeout : 0;

  if (m_logic_con_in_valve != 0)
  {
    m_logic_con_in_valve->setOpen(unmute);
  }
} /* ReflectorLogic::selectTg */


void ReflectorLogic::processEvent(const std::string& event)
{
  m_event_handler->processEvent(name() + "::" + event);
} /* ReflectorLogic::processEvent */


void ReflectorLogic::processTgSelectionEvent(void)
{
  if (!m_logic_con_out->isIdle() || !m_logic_con_in->isIdle() ||
      m_tg_selection_event.empty())
  {
    return;
  }
  processEvent(m_tg_selection_event);
  m_tg_selection_event.clear();
} /* ReflectorLogic::processTgSelectionEvent */


void ReflectorLogic::checkTmpMonitorTimeout(void)
{
  bool changed = false;
  MonitorTgsSet::iterator it = m_monitor_tgs.begin();
  while (it != m_monitor_tgs.end())
  {
    MonitorTgsSet::iterator next=it;
    ++next;
    const MonitorTgEntry& mte = *it;
    if (mte.timeout > 0)
    {
        // NOTE: mte.timeout is mutable
      if (--mte.timeout <= 0)
      {
        std::cout << name() << ": Temporary monitor timeout for TG #"
                  << mte.tg << std::endl;
        changed = true;
        m_monitor_tgs.erase(it);
        std::ostringstream os;
        os << "tmp_monitor_remove " << mte.tg;
        processEvent(os.str());
      }
    }
    it = next;
  }
  if (changed)
  {
    sendMsg(MsgTgMonitor(std::set<uint32_t>(
            m_monitor_tgs.begin(), m_monitor_tgs.end())));
  }
} /* ReflectorLogic::checkTmpMonitorTimeout */


/*
 * This file has not been truncated
 */
