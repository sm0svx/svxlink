/**
@file	 QsoFrn.cpp
@brief   Free Radio Network (FRN) QSO module
@author  sh123
@date	 2014-12-30

This file contains a class that implementes the things needed for one
EchoLink Qso.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <sigc++/bind.h>
#include <sstream>
#include <regex.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/
#include <AsyncConfig.h>
#include <AsyncAudioPacer.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioDecimator.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDebugger.h>
#include <AsyncTcpClient.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/
#include "Utils.h"
#include "ModuleFrn.h"
#include "QsoFrn.h"
#include "multirate_filter_coeff.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/
using namespace std;
using namespace Async;
using namespace sigc;
using namespace FrnUtils;


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

QsoFrn::QsoFrn(ModuleFrn *module)
  : init_ok(false)
  , tcp_client(new TcpClient<>(TCP_BUFFER_SIZE))
  , rx_timeout_timer(new Timer(RX_TIMEOUT_TIME, Timer::TYPE_PERIODIC))
  , con_timeout_timer(new Timer(CON_TIMEOUT_TIME, Timer::TYPE_PERIODIC))
  , keepalive_timer(new Timer(KEEPALIVE_TIMEOUT_TIME, Timer::TYPE_PERIODIC))
  , reconnect_timer(new Timer(KEEPALIVE_TIMEOUT_TIME, Timer::TYPE_ONESHOT))
  , state(STATE_DISCONNECTED)
  , connect_retry_cnt(0)
  , send_buffer_cnt(0)
  , gsmh(gsm_create())
  , lines_to_read(-1)
  , is_receiving_voice(false)
  , is_rf_disabled(false)
  , reconnect_timeout_ms(RECONNECT_TIMEOUT_TIME)
  , opt_frn_debug(false)
{
  assert(module != 0);

  Config &cfg = module->cfg();
  const string &cfg_name = module->cfgName();

  if (cfg.getValue(cfg_name, "FRN_DEBUG", opt_frn_debug))
    cout << "frn debugging is enabled" << endl;

  if (cfg.getValue(cfg_name, "DISABLE_RF", is_rf_disabled))
    cout << "rf is disabled" << endl;

  if (!cfg.getValue(cfg_name, "SERVER", opt_server))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/SERVER not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "PORT", opt_port))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/PORT not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "SERVER_BACKUP", opt_server_backup))
  {
    cerr << "*** WARNING: Config variable " << cfg_name
         << "/SERVER_BACKUP not set\n";
    opt_server_backup = opt_server;
  }
  if (!cfg.getValue(cfg_name, "PORT_BACKUP", opt_port_backup))
  {
    cerr << "*** WARNING: Config variable " << cfg_name
         << "/PORT_BACKUP not set\n";
    opt_port_backup = opt_port;
  }
  if (!cfg.getValue(cfg_name, "EMAIL_ADDRESS", opt_email_address))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/EMAIL_ADDRESS not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "DYN_PASSWORD", opt_dyn_password))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/DYN_PASSWORD not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "CALLSIGN_AND_USER", opt_callsign_and_user))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/CALLSIGN_AND_USER not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "CLIENT_TYPE", opt_client_type))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/CLIENT_TYPE not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "BAND_AND_CHANNEL", opt_band_and_channel))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/BAND_AND_CHANNEL not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "DESCRIPTION", opt_description))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/DESCRIPTION not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "COUNTRY", opt_country))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/COUNTRY not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "CITY_CITY_PART", opt_city_city_part))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/CITY_CITY_PART not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "NET", opt_net))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/NET not set\n";
    return;
  }
  if (!cfg.getValue(cfg_name, "VERSION", opt_version))
  {
    cerr << "*** ERROR: Config variable " << cfg_name
         << "/VERSION not set\n";
    return;
  }

  int gsm_one = 1;
  assert(gsm_option(gsmh, GSM_OPT_WAV49, &gsm_one) != -1);

  tcp_client->connected.connect(
      mem_fun(*this, &QsoFrn::onConnected));
  tcp_client->disconnected.connect(
      mem_fun(*this, &QsoFrn::onDisconnected));
  tcp_client->dataReceived.connect(
      mem_fun(*this, &QsoFrn::onDataReceived));
  //tcp_client->sendBufferFull.connect(
  //    mem_fun(*this, &QsoFrn::onSendBufferFull));

  this->rxVoiceStarted.connect(
      mem_fun(*this, &QsoFrn::onRxVoiceStarted));
  this->frnListReceived.connect(
      mem_fun(*this, &QsoFrn::onFrnListReceived));
  this->frnClientListReceived.connect(
      mem_fun(*this, &QsoFrn::onFrnClientListReceived));

  con_timeout_timer->setEnable(false);
  con_timeout_timer->expired.connect(
      mem_fun(*this, &QsoFrn::onConnectTimeout));

  rx_timeout_timer->setEnable(false);
  rx_timeout_timer->expired.connect(
      mem_fun(*this, &QsoFrn::onRxTimeout));

  keepalive_timer->setEnable(true);
  keepalive_timer->expired.connect(
      mem_fun(*this, &QsoFrn::onKeepaliveTimeout));

  reconnect_timer->setEnable(false);
  reconnect_timer->expired.connect(
      mem_fun(*this, &QsoFrn::onDelayedReconnect));

  init_ok = true;
}


QsoFrn::~QsoFrn(void)
{
  AudioSink::clearHandler();
  AudioSource::clearHandler();

  delete con_timeout_timer;
  con_timeout_timer = 0;

  delete rx_timeout_timer;
  con_timeout_timer = 0;

  delete tcp_client;
  tcp_client = 0;

  delete keepalive_timer;
  keepalive_timer = 0;

  gsm_destroy(gsmh);
  gsmh = 0;
}


bool QsoFrn::initOk(void)
{
  return init_ok;
}


void QsoFrn::connect(bool is_backup)
{
  setState(STATE_CONNECTING);

  server = is_backup ? opt_server_backup : opt_server;
  port = is_backup ? opt_port_backup : opt_port;

  cout << "connecting to " << server << ":" << port << endl;
  tcp_client->connect(server, atoi(port.c_str()));
}


void QsoFrn::disconnect(void)
{
  setState(STATE_DISCONNECTED);

  con_timeout_timer->setEnable(false);

  if (tcp_client->isConnected())
  {
    tcp_client->disconnect();
  }
}


std::string QsoFrn::stateToString(State state)
{
  switch(state)
  {
    case STATE_DISCONNECTED:
      return "DISCONNECTED";
    case STATE_CONNECTING:
      return "CONNECTING";
    case STATE_CONNECTED:
      return "CONNECTED";
    case STATE_LOGGING_IN_1:
      return "LOGGING_IN_1";
    case STATE_LOGGING_IN_2:
      return "LOGGIN_IN_2";
    case STATE_IDLE:
      return "IDLE";
    case STATE_ERROR:
      return "ERROR";
    case STATE_TX_AUDIO_WAITING:
      return "TX_AUDIO_WAITING";
    case STATE_TX_AUDIO_APPROVED:
      return "TX_AUDIO_APPROVED";
    case STATE_TX_AUDIO:
      return "TX_AUDIO";
    case STATE_RX_AUDIO:
      return "RX_AUDIO";
    case STATE_RX_CLIENT_LIST_HEADER:
      return "RX_CLIENT_LIST_HEADER";
    case STATE_RX_CLIENT_LIST:
      return "RX_CLIENT_LIST";
    case STATE_RX_LIST:
      return "RX_LIST";
    default:
      return "UNKNOWN";
  }
}


int QsoFrn::writeSamples(const float *samples, int count)
{
  //cout << __FUNCTION__ << " " << count << endl;

  if (state == STATE_IDLE)
  {
    sendRequest(RQ_TX0);
    setState(STATE_TX_AUDIO_WAITING);
  }

  int samples_read = 0;
  con_timeout_timer->reset();

  while (samples_read < count)
  {
    int read_cnt = min(BUFFER_SIZE - send_buffer_cnt, count-samples_read);
    for (int i = 0; i < read_cnt; i++)
    {
      float sample = samples[samples_read++];
      if (sample > 1)
        send_buffer[send_buffer_cnt++] = 32767;
      else if (sample < -1)
        send_buffer[send_buffer_cnt++] = -32767;
      else
        send_buffer[send_buffer_cnt++] = static_cast<int16_t>(32767.0 * sample);
    }
    if (send_buffer_cnt == BUFFER_SIZE)
    {
      if (state == STATE_TX_AUDIO)
      {
        sendVoiceData(send_buffer, send_buffer_cnt);
        send_buffer_cnt = 0;
      }
      else
      {
        samples_read = count;
        break;
      }
    }
  }
  return samples_read;
}


void QsoFrn::flushSamples(void)
{
  //cout << __FUNCTION__ << " " << stateToString(state) << endl;

  if (state == STATE_TX_AUDIO)
  {
    if (send_buffer_cnt > 0)
    {
      memset(send_buffer + send_buffer_cnt, 0,
          sizeof(send_buffer) - sizeof(*send_buffer) * send_buffer_cnt);
      send_buffer_cnt = BUFFER_SIZE;

      sendVoiceData(send_buffer, send_buffer_cnt);
      send_buffer_cnt = 0;
    }
    sendRequest(RQ_RX0);
  }
  sourceAllSamplesFlushed();
}


void QsoFrn::resumeOutput(void)
{
  //cout << __FUNCTION__ << endl;
}


void QsoFrn::squelchOpen(bool is_open)
{
  if (is_open && state == STATE_IDLE)
  {
//    sendRequest(RQ_TX0);
//    setState(STATE_TX_AUDIO_WAITING);
  }
}


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/
void QsoFrn::allSamplesFlushed(void)
{
  //cout << __FUNCTION__ << endl;
}


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/
void QsoFrn::setState(State newState)
{
  if (newState != state)
  {
    if (opt_frn_debug)
      cout << "state: " << stateToString(newState) << endl;
    state = newState;
    stateChange(newState);
    if (state == STATE_ERROR)
      error();
  }
}


void QsoFrn::login(void)
{
  assert(state == STATE_CONNECTED);

  setState(STATE_LOGGING_IN_1);

  std::stringstream s;
  s << "CT:";
  s << "<VX>" << opt_version           << "</VX>";
  s << "<EA>" << opt_email_address     << "</EA>";
  s << "<PW>" << opt_dyn_password      << "</PW>";
  s << "<ON>" << opt_callsign_and_user << "</ON>";
  s << "<CL>" << opt_client_type       << "</CL>";
  s << "<BC>" << opt_band_and_channel  << "</BC>";
  s << "<DS>" << opt_description       << "</DS>";
  s << "<NN>" << opt_country           << "</NN>";
  s << "<CT>" << opt_city_city_part    << "</CT>";
  s << "<NT>" << opt_net               << "</NT>";
  s << endl;

  std::string req = s.str();
  tcp_client->write(req.c_str(), req.length());
}

void QsoFrn::sendVoiceData(short *data, int len)
{
  assert(len == BUFFER_SIZE);

  size_t nbytes = 0;
  unsigned char gsm_data[FRN_AUDIO_PACKET_SIZE];

  for (int nframe = 0; nframe < FRAME_COUNT; nframe++)
  {
    short * src = data + nframe * PCM_FRAME_SIZE;
    unsigned char * dst = gsm_data + nframe * GSM_FRAME_SIZE;

    // GSM_OPT_WAV49, produce alternating frames 32, 33, 32, 33, ..
    gsm_encode(gsmh, src, dst);
    gsm_encode(gsmh, src + PCM_FRAME_SIZE / 2, dst + 32);

    nbytes += GSM_FRAME_SIZE;
  }
  sendRequest(RQ_TX1);
  size_t written = tcp_client->write(gsm_data, nbytes);
  if (written != nbytes)
  {
    cerr << "not all voice data was written to FRN: "
         << written << "\\" << nbytes << endl;
  }
}


void QsoFrn::reconnect(void)
{
  bool is_using_backup_server = (server == opt_server_backup && port == opt_port_backup);

  reconnect_timeout_ms = static_cast<int>(reconnect_timeout_ms * RECONNECT_BACKOFF);
  if (reconnect_timeout_ms > RECONNECT_MAX_TIMEOUT) {
    reconnect_timeout_ms = RECONNECT_MAX_TIMEOUT;
  }

  if (connect_retry_cnt++ < MAX_CONNECT_RETRY_CNT)
  {
    cout << "reconnecting #" << connect_retry_cnt << endl;
    connect(!is_using_backup_server);
  }
  else
  {
    cerr << "failed to reconnect " << MAX_CONNECT_RETRY_CNT << " times" << endl;
    connect_retry_cnt = 0;
    reconnect_timeout_ms = RECONNECT_TIMEOUT_TIME;
    setState(STATE_ERROR);
  }
}


void QsoFrn::sendRequest(Request rq)
{
  std::stringstream s;

  switch(rq)
  {
    case RQ_RX0:
      s << "RX0";
      break;

    case RQ_TX0:
      s << "TX0";
      break;

    case RQ_TX1:
      s << "TX1";
      break;

    case RQ_P:
      s << "P";
      break;

    default:
      cerr << "unknown request " << rq << endl;
      return;
  }
  if (opt_frn_debug)
    cout << "req:   " << s.str() << endl;
  if (tcp_client->isConnected())
  {
    s << "\r\n";
    std::string rq_s = s.str();
    size_t written = tcp_client->write(rq_s.c_str(), rq_s.length());
    if (written != rq_s.length())
    {
      cerr << "request " << rq_s << " was not written to FRN: "
           << written << "\\" << rq_s.length() << endl;
    }
  }
}


int QsoFrn::handleAudioData(unsigned char *data, int len)
{
  unsigned char *gsm_data = data + CLIENT_INDEX_SIZE;
  short *pcm_buffer = receive_buffer;
  float pcm_samples[PCM_FRAME_SIZE];

  if (len < FRN_AUDIO_PACKET_SIZE + CLIENT_INDEX_SIZE)
    return 0;

  if (!is_receiving_voice)
  {
    unsigned short client_index = data[1] | data[0] << 8;
    is_receiving_voice = true;
    if (client_index > 0 && client_index <= client_list.size())
      rxVoiceStarted(client_list[client_index - 1]);
  }

  if (!is_rf_disabled)
  {
    for (int frameno = 0; frameno < FRAME_COUNT; frameno++)
    {
      unsigned char *src = gsm_data + frameno * GSM_FRAME_SIZE;
      short *dst = pcm_buffer;
      bool is_gsm_decode_success = true;

      // GSM_OPT_WAV49, consume alternating frames of size 33, 32, 33, 32, ..
      if (gsm_decode(gsmh, src, dst) == -1)
        is_gsm_decode_success = false;

      if (gsm_decode(gsmh, src + 33, dst + PCM_FRAME_SIZE / 2) == -1)
        is_gsm_decode_success = false;

      if (!is_gsm_decode_success)
        cerr << "gsm decoder failed to decode frame " << frameno << endl;

      for (int i = 0; i < PCM_FRAME_SIZE; i++)
         pcm_samples[i] = static_cast<float>(pcm_buffer[i]) / 32768.0;

      int all_written = 0;
      while (all_written < PCM_FRAME_SIZE)
      {
        int written = sinkWriteSamples(pcm_samples + all_written,
            PCM_FRAME_SIZE - all_written);
        if (written == 0)
        {
          cerr << "cannot write frame to sink, dropping sample "
               << (PCM_FRAME_SIZE - all_written) << endl;
          break;
        }
        all_written += written;
      }
      pcm_buffer += PCM_FRAME_SIZE;
    }
  }
  setState(STATE_IDLE);
  rx_timeout_timer->setEnable(true);
  rx_timeout_timer->reset();
  sendRequest(RQ_P);
  return FRN_AUDIO_PACKET_SIZE + CLIENT_INDEX_SIZE;
}


int QsoFrn::handleCommand(unsigned char *data, int len)
{
  int bytes_read = 0;
  Response cmd = (Response)data[0];
  if (opt_frn_debug)
    cout << "cmd:   " << cmd << endl;

  keepalive_timer->reset();

  bytes_read += 1;

  switch (cmd)
  {
    case DT_IDLE:
      sendRequest(RQ_P);
      setState(STATE_IDLE);
      break;

    case DT_DO_TX:
      setState(STATE_TX_AUDIO_APPROVED);
      break;

    case DT_VOICE_BUFFER:
      setState(STATE_RX_AUDIO);
      rx_timeout_timer->setEnable(true);
      rx_timeout_timer->reset();
      break;

    case DT_TEXT_MESSAGE:
    case DT_NET_NAMES:
    case DT_ADMIN_LIST:
    case DT_ACCESS_LIST:
    case DT_BLOCK_LIST:
    case DT_MUTE_LIST:
    case DT_ACCESS_MODE:
      setState(STATE_RX_LIST);
      break;

    case DT_CLIENT_LIST:
      setState(STATE_RX_CLIENT_LIST_HEADER);
      break;

    default:
      cout << "unknown command " << cmd << endl;
      break;
  }
  return bytes_read;
}


int QsoFrn::handleListHeader(unsigned char *data, int len)
{
  int bytes_read = 0;

  if (len >= CLIENT_INDEX_SIZE)
  {
    bytes_read += CLIENT_INDEX_SIZE;
    setState(STATE_RX_CLIENT_LIST);
    lines_to_read = -1;
  }
  return bytes_read;
}


int QsoFrn::handleList(unsigned char *data, int len)
{
  int bytes_read = 0;
  std::string line;
  std::istringstream lines(std::string((char*)data, len));
  bool has_win_newline = hasWinNewline(lines);

  if (hasLine(lines) && safeGetline(lines, line))
  {
    if (lines_to_read == -1)
    {
      lines_to_read = atoi(line.c_str());
    }
    else
    {
      cur_item_list.push_back(line);
      lines_to_read--;
    }
    bytes_read += line.length() + (has_win_newline ? 2 : 1);
  }
  if (lines_to_read == 0)
  {
    if (state == STATE_RX_CLIENT_LIST)
      frnClientListReceived(cur_item_list);
    frnListReceived(cur_item_list);
    cur_item_list.clear();
    lines_to_read = -1;
    setState(STATE_IDLE);
  }
  //cout << "got " << len << " read " << bytes_read << endl;
  return bytes_read;
}


int QsoFrn::handleLogin(unsigned char *data, int len, bool stage_one)
{
  int bytes_read = 0;
  std::string line;
  std::istringstream lines(std::string((char*)data, len));
  bool has_win_newline = hasWinNewline(lines);

  if (hasLine(lines) && safeGetline(lines, line))
  {
    if (stage_one)
    {
      if (line.length() == std::string("2014003").length() ||
          line.length() == std::string("0").length())
      {
        setState(STATE_LOGGING_IN_2);
        cout << "login stage 1 completed: " << line << endl;
      }
      else
      {
        setState(STATE_ERROR);
        cerr << "login stage 1 failed: " << line << endl;
      }
    }
    else
    {
      if (line.find("<AL>BLOCK</AL>") == std::string::npos &&
          line.find("<AL>WRONG</AL>") == std::string::npos)
      {
        setState(STATE_IDLE);
        sendRequest(RQ_RX0);
        cout << "login stage 2 completed: " << line << endl;
      }
      else
      {
        setState(STATE_ERROR);
        cerr << "login stage 2 failed: " << line << endl;
      }
    }
    bytes_read += line.length() + (has_win_newline ? 2 : 1);
  }
  return bytes_read;
}


void QsoFrn::onConnected(void)
{
  //cout << __FUNCTION__ << endl;
  setState(STATE_CONNECTED);

  connect_retry_cnt = 0;
  reconnect_timeout_ms = RECONNECT_TIMEOUT_TIME;
  con_timeout_timer->setEnable(true);
  login();
}


void QsoFrn::onDisconnected(TcpConnection *conn,
  TcpConnection::DisconnectReason reason)
{
  //cout << __FUNCTION__ << " ";
  bool needs_reconnect = false;

  setState(STATE_DISCONNECTED);

  con_timeout_timer->setEnable(false);

  switch (reason)
  {
    case TcpConnection::DR_HOST_NOT_FOUND:
      cout << "DR_HOST_NOT_FOUND" << endl;
      needs_reconnect = true;
      break;

    case TcpConnection::DR_REMOTE_DISCONNECTED:
      cout << "DR_REMOTE_DISCONNECTED" << ", "
           << conn->disconnectReasonStr(reason) << endl;
      needs_reconnect = true;
      break;

    case TcpConnection::DR_SYSTEM_ERROR:
      cout << "DR_SYSTEM_ERROR" << ", "
           << conn->disconnectReasonStr(reason) << endl;
      needs_reconnect = true;
      break;

    //case TcpConnection::DR_RECV_BUFFER_OVERFLOW:
    //  cout << "DR_RECV_BUFFER_OVERFLOW" << endl;
    //  setState(STATE_ERROR);
    //  break;

    case TcpConnection::DR_ORDERED_DISCONNECT:
      cout << "DR_ORDERED_DISCONNECT" << endl;
      break;

    default:
      cout << "DR_UNKNOWN" << endl;
      setState(STATE_ERROR);
      break;
  }

  if (needs_reconnect)
  {
    cout << "reconnecting in " << reconnect_timeout_ms << " ms" << endl;
    reconnect_timer->setEnable(true);
    reconnect_timer->setTimeout(reconnect_timeout_ms);
    reconnect_timer->reset();
  }
}


int QsoFrn::onDataReceived(TcpConnection *con, void *data, int len)
{
  //cout << __FUNCTION__ << " len: " << len << endl;
  unsigned char *p_data = (unsigned char*)data;

  con_timeout_timer->reset();
  int remaining_bytes = len;

  while (remaining_bytes > 0)
  {
    int bytes_read = 0;

    switch (state)
    {
      case STATE_LOGGING_IN_1:
        bytes_read += handleLogin(p_data, remaining_bytes, true);
        break;

      case STATE_LOGGING_IN_2:
        bytes_read += handleLogin(p_data, remaining_bytes, false);
        break;

      case STATE_TX_AUDIO_APPROVED:
        if (remaining_bytes >= CLIENT_INDEX_SIZE)
          bytes_read += CLIENT_INDEX_SIZE;
        setState(STATE_TX_AUDIO);
        break;

      case STATE_TX_AUDIO:
      case STATE_TX_AUDIO_WAITING:
      case STATE_IDLE:
        bytes_read += handleCommand(p_data, remaining_bytes);
        break;

      case STATE_RX_AUDIO:
        bytes_read += handleAudioData(p_data, remaining_bytes);
        break;

      case STATE_RX_CLIENT_LIST_HEADER:
        bytes_read += handleListHeader(p_data, remaining_bytes);
        break;

      case STATE_RX_LIST:
      case STATE_RX_CLIENT_LIST:
        bytes_read += handleList(p_data, remaining_bytes);
        break;

      default:
        break;
    }
    //cout << bytes_read << " " << remaining_bytes << " " << len << endl;
    if (bytes_read == 0)
      break;
    remaining_bytes -= bytes_read;
    p_data += bytes_read;
  }
  return len - remaining_bytes;
}


void QsoFrn::onSendBufferFull(bool is_full)
{
  cerr << "send buffer is full " << is_full << endl;
}


void QsoFrn::onConnectTimeout(Timer *timer)
{
  //cout << __FUNCTION__ << endl;
  if (state == STATE_IDLE)
  {
    disconnect();
    reconnect();
  }
}


void QsoFrn::onRxTimeout(Timer *timer)
{
  //cout << __FUNCTION__ << endl;
  sinkFlushSamples();
  rx_timeout_timer->setEnable(false);
  is_receiving_voice = false;
  //setState(STATE_IDLE);
  sendRequest(RQ_P);
}


void QsoFrn::onKeepaliveTimeout(Timer *timer)
{
  if (state == STATE_IDLE)
    sendRequest(RQ_P);
}


void QsoFrn::onRxVoiceStarted(const string &client_descritpion) const
{
  if (is_rf_disabled)
    cout << "[listen only] ";
  cout << "voice started: " << client_descritpion << endl;
}


void QsoFrn::onFrnListReceived(const FrnList &list) const
{
  cout << "FRN list received:" << endl;
  for (FrnList::const_iterator it = list.begin(); it != list.end(); ++it)
  {
    cout << "-- " << *it << endl;
  }
}


void QsoFrn::onFrnClientListReceived(const FrnList &list)
{
  cout << "FRN active client list updated" << endl;
  client_list = list;
}


void QsoFrn::onDelayedReconnect(Async::Timer *timer)
{
  reconnect();
}

/*
 * This file has not been truncated
 */

