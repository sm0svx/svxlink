/**
@file	 QsoFrn.cpp
@brief   Data for Frn Qso.
@author  Tobias Blomberg / SM0SVX
@date	 2004-06-02

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
  , tcp_client(new TcpClient())
  , keep_alive_timer(new Timer(KEEP_ALIVE_TIME, Timer::TYPE_PERIODIC))
  , con_timeout_timer(new Timer(CON_TIMEOUT_TIME, Timer::TYPE_PERIODIC))
  , state(STATE_DISCONNECTED)
  , connect_retry_cnt(0)
{
  assert(module != 0);

  Config &cfg = module->cfg();
  const string &cfg_name = module->cfgName();

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
 
  tcp_client->connected.connect(
      mem_fun(*this, &QsoFrn::onConnected));
  tcp_client->disconnected.connect(
      mem_fun(*this, &QsoFrn::onDisconnected));
  tcp_client->dataReceived.connect(
      mem_fun(*this, &QsoFrn::onDataReceived));
  tcp_client->sendBufferFull.connect(
      mem_fun(*this, &QsoFrn::onSendBufferFull));

  keep_alive_timer->setEnable(false);
  keep_alive_timer->expired.connect(
      mem_fun(*this, &QsoFrn::onKeepaliveTimeout));

  con_timeout_timer->setEnable(false);
  con_timeout_timer->expired.connect(
      mem_fun(*this, &QsoFrn::onConnectTimeout));

  init_ok = true;
}


QsoFrn::~QsoFrn(void)
{
  AudioSink::clearHandler();
  AudioSource::clearHandler();

  delete keep_alive_timer;
  keep_alive_timer = 0;

  delete con_timeout_timer;
  con_timeout_timer = 0;

  delete tcp_client;
  tcp_client = 0;
}


bool QsoFrn::initOk(void)
{
  return init_ok;
}


void QsoFrn::connect(void)
{
  cout << __FUNCTION__ << ", " << opt_server << ":" << opt_port << endl;
  setState(STATE_CONNECTING);
  tcp_client->connect(opt_server, atoi(opt_port.c_str()));
}


void QsoFrn::disconnect(void)
{
  setState(STATE_DISCONNECTED);

  if (tcp_client->isConnected())
  { 
    keep_alive_timer->setEnable(false);
    con_timeout_timer->setEnable(false);
    tcp_client->disconnect();
  }
}


std::string QsoFrn::stateToString(State state)
{
  std::string result;

  switch(state)
  {
    case STATE_DISCONNECTED:
      result = "DISCONNECTED";
      break;
    case STATE_CONNECTING:
      result = "CONNECTING";
      break;
    case STATE_CONNECTED:
      result = "CONNECTED";
      break;
    case STATE_LOGGING_IN:
      result = "LOGGING_IN";
      break;
    case STATE_LOGGING_IN_2:
      result = "LOGGIN_IN_2";
      break;
    case STATE_LOGGED_IN:
      result = "LOGGED_IN";
      break;
    case STATE_ERROR:
      result = "ERROR";
      break;
    default:
      result = "UNKNOWN";
      break;
  }
  return result;
}


int QsoFrn::writeSamples(const float *samples, int count)
{
  cout << "." << flush;
  return count;
}


void QsoFrn::flushSamples(void)
{
  cout << __FUNCTION__ << endl;
  sourceAllSamplesFlushed();
}


void QsoFrn::resumeOutput(void)
{
  cout << __FUNCTION__ << endl;
}


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/
void QsoFrn::allSamplesFlushed(void)
{
  cout << __FUNCTION__ << endl;
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
    cout << __FUNCTION__ << " " << stateToString(newState) << endl;
    state = newState;
    stateChange(newState);
  }
}

void QsoFrn::login(void)
{
  setState(STATE_LOGGING_IN);

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


void QsoFrn::reconnect(void)
{
  if (connect_retry_cnt++ < MAX_CONNECT_RETRY_CNT)
  {
    cout << "reconnecting " << connect_retry_cnt << endl;
    connect();
  } 
  else 
  {
    setState(STATE_ERROR);
    cerr << "failed to connect " << MAX_CONNECT_RETRY_CNT << " times" << endl;
  }
}


void QsoFrn::handleCommand(Command cmd, void *data, int len)
{
  cout << cmd << flush;
  std::string data_s((char*)data, len);

  switch (cmd)
  {
    case DT_IDLE:
      break;

    case DT_DO_TX:
      break;

    case DT_VOICE_BUFFER:
      break;

    case DT_CLIENT_LIST:
    case DT_TEXT_MESSAGE:
    case DT_NET_NAMES:
    case DT_ADMIN_LIST:
    case DT_ACCESS_LIST:
    case DT_BLOCK_LIST:
    case DT_MUTE_LIST:
    case DT_ACCESS_MODE:
      cout << endl << data_s << endl;
      break;

    default:
      cerr << "unknown command" << endl << data_s << endl;
      break;
  }
}


void QsoFrn::onConnected(void)
{
  //cout << __FUNCTION__ << endl;
  setState(STATE_CONNECTED);

  connect_retry_cnt = 0;
  con_timeout_timer->setEnable(true);
  login();
}


void QsoFrn::onDisconnected(TcpConnection *conn, 
  TcpConnection::DisconnectReason reason)
{
  //cout << __FUNCTION__ << " ";
  setState(STATE_DISCONNECTED);

  keep_alive_timer->setEnable(false);
  con_timeout_timer->setEnable(false);

  switch (reason)
  {
    case TcpConnection::DR_HOST_NOT_FOUND:
      cout << "DR_HOST_NOT_FOUND";
      setState(STATE_ERROR);
      break;

    case TcpConnection::DR_REMOTE_DISCONNECTED:
      cout << "DR_REMOTE_DISCONNECTED" << ", " 
           << conn->disconnectReasonStr(reason);
      reconnect();
      break;

    case TcpConnection::DR_SYSTEM_ERROR:
      cout << "DR_SYSTEM_ERROR" << ", " 
           << conn->disconnectReasonStr(reason);
      reconnect();
      break;

    case TcpConnection::DR_RECV_BUFFER_OVERFLOW:
      cout << "DR_RECV_BUFFER_OVERFLOW";
      setState(STATE_ERROR);
      break;

    case TcpConnection::DR_ORDERED_DISCONNECT:
      cout << "DR_ORDERED_DISCONNECT";
      break;

    default:
      cout << "DR_UNKNOWN";
      setState(STATE_ERROR);
      break;
  }
  cout << endl;
}


int QsoFrn::onDataReceived(TcpConnection *con, void *data, int len)
{
  //cout << __FUNCTION__ << " len: " << len << endl;
  con_timeout_timer->reset();

  std::string data_s((char*)data, len);

  switch(state)
  {
    case STATE_LOGGING_IN:
      // TODO add version validation
      setState(STATE_LOGGING_IN_2);
      cout << data_s << endl;
      break;

    case STATE_LOGGING_IN_2:
      // TODO add server response validation
      setState(STATE_LOGGED_IN);
      keep_alive_timer->setEnable(true);
      cout << data_s << endl;
      break;

    case STATE_LOGGED_IN:
      handleCommand((Command)data_s[0], data, len);
      break;

    default:
      break;
  }
  return len;
}


void QsoFrn::onSendBufferFull(bool is_full)
{
  cout << __FUNCTION__ << " " << is_full << endl;
}


void QsoFrn::onKeepaliveTimeout(Timer *timer)
{
  cout << "_" << flush;
  std::string req("P\n");
  if (tcp_client->isConnected())
  {
    tcp_client->write(req.c_str(), req.length());
  }
}


void QsoFrn::onConnectTimeout(Timer *timer)
{
  //cout << __FUNCTION__ << endl;
  disconnect();
}


/*
 * This file has not been truncated
 */

