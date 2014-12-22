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
  , state(STATE_DISCONNECTED)
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

  init_ok = true;
}


QsoFrn::~QsoFrn(void)
{
  AudioSink::clearHandler();
  AudioSource::clearHandler();

  delete tcp_client;
  tcp_client = 0;
}


bool QsoFrn::initOk(void)
{
  return init_ok;
}


void QsoFrn::connect(void)
{
  tcp_client->connect(opt_server, atoi(opt_port.c_str()));
  setState(STATE_CONNECTING);
}


void QsoFrn::disconnect(void)
{
  if (tcp_client->isConnected())
  {
    tcp_client->disconnect();
  }
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
    state = newState;
    stateChange(newState);
  }
}


void QsoFrn::onConnected(void)
{
  cout << __FUNCTION__ << endl;
  setState(STATE_CONNECTED);
}


void QsoFrn::onDisconnected(TcpConnection *conn, 
  TcpConnection::DisconnectReason reason)
{
  cout << __FUNCTION__ << " ";
  setState(STATE_DISCONNECTED);
  switch (reason)
  {
    case TcpConnection::DR_HOST_NOT_FOUND:
      cout << "DR_HOST_NOT_FOUND";
      break;
    case TcpConnection::DR_REMOTE_DISCONNECTED:
      cout << "DR_REMOTE_DISCONNECTED" << ", " << conn->disconnectReasonStr(reason);
      break;
    case TcpConnection::DR_SYSTEM_ERROR:
      cout << "DR_SYSTEM_ERROR" << ", " << conn->disconnectReasonStr(reason);
      break;
    case TcpConnection::DR_RECV_BUFFER_OVERFLOW:
      cout << "DR_RECV_BUFFER_OVERFLOW";
      break;
    case TcpConnection::DR_ORDERED_DISCONNECT:
      cout << "DR_ORDERED_DISCONNECT";
      break;
    default:
      cout << "DR_UNKNOWN";
      break;
  }
  cout << endl;
}


int QsoFrn::onDataReceived(TcpConnection *con, void *data, int len)
{
  return len;
}

void QsoFrn::onSendBufferFull(bool is_full)
{
}

/*
 * This file has not been truncated
 */

