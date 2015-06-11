/**
@file	 svxserver.cpp
@brief   Main file for the svxserver
@author  Adi Bier / DL1HRC
@date	 2014-06-16

This is the main file for the svxserver remote transceiver for the
SvxLink server. It is used to link in remote transceivers to the SvxLink
server core (e.g. via a TCP/IP network).

\verbatim
svxserver - A svxlink server application
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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
#include <signal.h>
#include <sys/time.h>
#include <cstring>
#include <cstdlib>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncTcpServer.h>
#include <AsyncTcpConnection.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "server.h"
#include "NetTrxMsg.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace sigc;
using namespace NetTrxMsg;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


SvxServer::SvxServer(Async::Config &cfg)
{
  string port = "5210";
  if (!cfg.getValue("GLOBAL", "LISTEN_PORT", port))
  {
    cerr << "*** ERROR: Configuration variable LISTEN_PORT is missing."
         << "Setting default to 5210" << endl;
  }

  cfg.getValue("GLOBAL", "AUTH_KEY", auth_key, true);
  
  hbto = 10000;
  string value;
  if (cfg.getValue("GLOBAL", "HEARTBEAT_TIMEOUT", value))
  {
    hbto = 1000 * atoi(value.c_str());
    if (hbto < 5000 || hbto > 50000)
    {
      cout << "--- senseless value for HEARTBEAT_TIMEOUT "
           << "setting to 10000" << endl;
      hbto = 10000;
    }
    cout << "--- setting HEARTBEAT_TIMEOUT=" << hbto << "ms" << endl;
  }

  cout << "--- AUTH_KEY=" << auth_key << endl;
  cout << "--- starting SERVER on port " << port << endl;

   // create the server instance
  server = new Async::TcpServer(port);
  server->clientConnected.connect(mem_fun(*this, &SvxServer::clientConnected));
  server->clientDisconnected.connect(
      mem_fun(*this, &SvxServer::clientDisconnected));

  heartbeat_timer = new Async::Timer(hbto);
  heartbeat_timer->setEnable(false);
  heartbeat_timer->expired.connect(mem_fun(*this, &SvxServer::heartbeat));

} /* SvxServer::SvxServer */


SvxServer::~SvxServer(void)
{
  clients.clear();
  delete heartbeat_timer;
  delete server;
}


void SvxServer::clientConnected(Async::TcpConnection *con)
{
  cout << "--- Client connected: " << con->remoteHost() << ":"
       << con->remotePort() << endl;

  con->dataReceived.connect(mem_fun(*this, &SvxServer::tcpDataReceived));

  pair<const string, uint16_t> key(con->remoteHost().toString(),
                                   con->remotePort());
  Cons clpair;
  clpair.con = con;
  clpair.state = STATE_VER_WAIT;
  clpair.recv_exp = sizeof(Msg);
  clpair.recv_cnt = 0;
  gettimeofday(&clpair.last_msg, NULL);

  clients[key] = clpair;

  MsgProtoVer *ver_msg = new MsgProtoVer;
  sendMsg(con, ver_msg);

  MsgAuthChallenge *auth_msg = new MsgAuthChallenge;
  memcpy(auth_challenge, auth_msg->challenge(),
        MsgAuthChallenge::CHALLENGE_LEN);
        
  if (auth_key.empty())
  {
    MsgAuthOk *auth_msg = new MsgAuthOk;
    sendMsg(con, auth_msg);
    clpair.state = STATE_VER_WAIT;
  }
  else
  {
    sendMsg(con, auth_msg);
    clpair.state = STATE_AUTH_WAIT;
  }

  clients[key] = clpair;
  heartbeat_timer->setEnable(true);

} /* SvxServer::clientConnected */


void SvxServer::clientDisconnected(Async::TcpConnection *con,
      	      	      	Async::TcpConnection::DisconnectReason reason)
{
  cout << "--- Client disconnected: " << con->remoteHost() << ":"
       << con->remotePort() << endl;

  assert(clients.size() > 0);

  Clients::iterator it;
  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if ((*it).second.con == con)
    {
      (*it).second.state = STATE_DISC;
      break;
    }
  }

  assert(it != clients.end());

  cout << "--- removing client " << con->remoteHost() << ":"
       << con->remotePort()  << " from client list" << endl;

  clients.erase(it);

  if (clients.size() < 1)
  {
    heartbeat_timer->setEnable(false);
    heartbeat_timer->reset();
  }
} /* SvxServer::clientDisconnected */


void SvxServer::heartbeat(Async::Timer *t)
{
  struct timeval diff_tv;
  struct timeval now;
  gettimeofday(&now, NULL);
  int diff_ms;
  Clients t_clients;

  assert(clients.size() > 0);
  MsgHeartbeat *msg = new MsgHeartbeat;
  Clients::iterator it;
  for (it = clients.begin(); it != clients.end(); it++)
  {
    // sending heartbeat to clients
    sendMsg(((*it).second).con, msg);

    // get clients last activity
    timersub(&now, &((*it).second).last_msg, &diff_tv);
    diff_ms=diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000;
    if (diff_ms > 2 * hbto)
    {
      cerr << "**** ERROR: Heartbeat timeout, lost connection from "
           << (*it).second.con->remoteHost() << ":"
           << (*it).second.con->remotePort() << endl;
      t_clients.insert(*it);
    }
  }
  
  // check if something to delete
  for (it = t_clients.begin(); it != t_clients.end(); it++)
  {
    ((*it).second).con->disconnect();
    clientDisconnected(((*it).second).con, 
                  TcpConnection::DR_ORDERED_DISCONNECT);
  }
  t->reset();
} /* SvxServer::heartbeat */


int SvxServer::tcpDataReceived(Async::TcpConnection *con, void *data, int size)
{
  //cout << "tcpDataReceived: size=" << size << endl;

  Clients::iterator it;
  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if (((*it).second).con == con)
    {
      break;
    }
  }

  assert (it != clients.end());

  int orig_size = size;
  char *buf = static_cast<char*>(data);
  while (size > 0)
  {
    unsigned read_cnt = min(static_cast<unsigned>(size), (*it).second.recv_exp
                                                      - (*it).second.recv_cnt);
    if ((*it).second.recv_cnt + read_cnt > sizeof((*it).second.recv_buf))
    {
      cerr << "*** ERROR: TCP receive buffer overflow " <<
              "in svxserver. Disconnecting...\n";
      con->disconnect();
      clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
      return orig_size;
    }
    memcpy((*it).second.recv_buf+(*it).second.recv_cnt, buf, read_cnt);
    size -= read_cnt;
    (*it).second.recv_cnt += read_cnt;
    buf += read_cnt;

    if ((*it).second.recv_cnt == (*it).second.recv_exp)
    {
      if ((*it).second.recv_exp == sizeof(Msg))
      {
        Msg *msg = reinterpret_cast<Msg*>((*it).second.recv_buf);
        if (msg->size() == sizeof(Msg))
        {
	      handleMsg(con, msg);
	      (*it).second.recv_cnt = 0;
	      (*it).second.recv_exp = sizeof(Msg);
	    }
	    else if (msg->size() > sizeof(Msg))
	    {
          (*it).second.recv_exp = msg->size();
	    }
	    else
	    {
	      cerr << "*** ERROR: Illegal message header received in svxserver "
               << ". Header length too small (" << msg->size() << ")\n";
	      con->disconnect();
	      clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
	      return orig_size;
	    }
      }
      else
      {
    	Msg *msg = reinterpret_cast<Msg*>((*it).second.recv_buf);
    	handleMsg(con, msg);
	    (*it).second.recv_cnt = 0;
        (*it).second.recv_exp = sizeof(Msg);
      }
    }
  }

  return orig_size;

} /* SvxServer::tcpDataReceived */


void SvxServer::handleMsg(Async::TcpConnection *con, Msg *msg)
{
//cout << "message <---------- " << con->remoteHost() << ":" << con->remotePort()
//     << ", type=" << msg->type() << " received\n";

  int state = STATE_READY;
  Clients::iterator it;

  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if ( ((*it).second).con == con)
    {
      state = ((*it).second).state;
      gettimeofday(&((*it).second).last_msg, NULL);
      break;
    }
  }

  assert(it != clients.end());

  switch (state)
  {
    case STATE_DISC:
      return;

    case STATE_VER_WAIT:
      return;

    case STATE_AUTH_WAIT:
      if (msg->type() == MsgAuthResponse::TYPE &&
          msg->size() == sizeof(MsgAuthResponse))
      {
        MsgAuthResponse *resp_msg = reinterpret_cast<MsgAuthResponse *>(msg);
        if (!resp_msg->verify(auth_key, auth_challenge))
        {
          cerr << "*** ERROR: Authentication error in svxserver.\n";
          con->disconnect();
          ((*it).second).state = STATE_DISC;
          clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
          return;
        }
        else
        {
          MsgAuthOk *ok_msg = new MsgAuthOk;
          sendMsg(con, ok_msg);
          ((*it).second).state = STATE_READY;
        }
      }
      else
      {
        cerr << "*** ERROR: Protocol error in svxserver.\n";
        con->disconnect();
        clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
      }
      return;

    case STATE_READY:
      //cout << "state=STATE_READY\n";
      break;
  }

  Msg *cmsg = 0;

  // check type of message
  switch (msg->type())
  {
    case MsgHeartbeat::TYPE:
    {
      return;
    }

      // get callsign from remote station
    case MsgRemoteCall::TYPE:
    {
      MsgRemoteCall *n = reinterpret_cast<MsgRemoteCall *>(msg);
      (*it).second.callsign = n->getCall();
      return;
    }

    case MsgSquelch::TYPE:
    {
      MsgSquelch *n = reinterpret_cast<MsgSquelch *>(msg);
      //cout << "___MsgSquelch: isOpen=" << n->isOpen() << endl;
      cmsg = n;
      break;
    }

    case MsgReset::TYPE:
    {
      //cout << "___MsgReset" << endl;
      break;
    }

     // messages not handled by svxserver
    case MsgAddToneDetector::TYPE:
    case MsgSendDtmf::TYPE:
    case MsgEnableCtcss::TYPE:
      return;

    case MsgRxAudioCodecSelect::TYPE:
    {
      MsgRxAudioCodecSelect *n = reinterpret_cast<MsgRxAudioCodecSelect *>(msg);
      ((*it).second).rxcodec = n->name();
      return;
    }

    case MsgTxAudioCodecSelect::TYPE:
    {
      MsgTxAudioCodecSelect *n = reinterpret_cast<MsgTxAudioCodecSelect *>(msg);
      ((*it).second).rxcodec = n->name();
      return;
    }

    case MsgAudio::TYPE:
    {
      if ((*it).second.sql_open == false)
      {
        //cout << "___set MsgSquelch(false)" << endl;
        MsgSquelch  *ms = new MsgSquelch(true, 1.0, 1);
        sendExcept(con, ms);
        (*it).second.sql_open = true;
      }

      if ((*it).second.tx_mode != Tx::TX_AUTO)
      {
        //cout << "___MsgAudio::TYPE" << endl;
        (*it).second.tx_mode = Tx::TX_AUTO;
        MsgSetTxCtrlMode *n = new MsgSetTxCtrlMode(Tx::TX_AUTO);
        sendExcept(con, n);
        sendMsg(con, n);
      }

      cmsg = reinterpret_cast<MsgAudio *>(msg);
      sendExcept(con, cmsg);
      return;
    }

    case MsgFlush::TYPE:
    {
      /*cout << "___rx=MsgFlush" << endl;
      cout << "___set MsgSquelch(false)" << endl;*/
      MsgSquelch  *ms = new MsgSquelch(false, 0.0, 1);
      sendExcept(con, ms);
      sendMsg(con, ms);
      (*it).second.sql_open = false;

      MsgAllSamplesFlushed *o = new MsgAllSamplesFlushed;
      sendMsg(con, o);
      cmsg = o;
      break;
    }

    case MsgSetMuteState::TYPE:
    {
      MsgSetMuteState *s = reinterpret_cast<MsgSetMuteState *>(msg);
      //cout << "___MsgSetMuteState: muteState=" << s->muteState() << endl;
      cmsg = s;
      break;
    }

    case MsgSetTxCtrlMode::TYPE:
    {
      MsgSetTxCtrlMode *s = reinterpret_cast<MsgSetTxCtrlMode *>(msg);
      /*cout << "___MsgSetTxCtrlMode: type="<< s->type() <<
           ", mode=" << s->mode() << endl;*/

      if (s->mode() == 1)
      {
        //cout << "___sende MsgTransmitterStateChange TX=true" << endl;
        MsgTransmitterStateChange *n = new MsgTransmitterStateChange(true);
        sendMsg(con, n);
        sendExcept(con, n);

        MsgSquelch  *ms = new MsgSquelch(true, 1.0, 1);
        //cout << "___set MsgSquelch(true)" << endl;
        sendExcept(con, ms);
        (*it).second.sql_open = true;
      }
      if (s->mode() == 2)
      {
        //cout << "___MsgSetTxCtrlMode(Tx::TX_AUTO)" << endl;
        (*it).second.tx_mode = Tx::TX_AUTO;
        MsgSetTxCtrlMode *n = new MsgSetTxCtrlMode(Tx::TX_AUTO);
        sendExcept(con, n);

        //cout << "___sende MsgTransmitterStateChange TX=false" << endl;
        MsgTransmitterStateChange *m = new MsgTransmitterStateChange(false);
        sendExcept(con, m);
        sendMsg(con, m);
      }

      cmsg = s;
      break;
    }

    default:
      cerr << "*** ERROR: Unknown TCP message received by svxserver "
           << msg->type() << ", size=" << msg->size() << endl;
      return;
  }

  sendExcept(con, cmsg);

} /* SvxServer::handleMsg */


void SvxServer::sendExcept(Async::TcpConnection *con, Msg *msg)
{

  Clients::iterator it;

  // sending data to connected clients
  for (it=clients.begin(); it != clients.end(); it++)
  {
    if (((*it).second).con != con)
    {
      sendMsg(((*it).second).con, msg);
    }
  }
} /* SvxServer::sendExcept */


void SvxServer::sendMsg(Async::TcpConnection *con, Msg *msg)
{

  assert(con->isConnected());
  if (clients.size() < 1)
  {
    return;
  }

  int written = con->write(msg, msg->size());
  if (written != static_cast<int>(msg->size()))
  {
    if (written == -1)
    {
      cerr << "*** ERROR: TCP transmit error.\n";
    }
    else
    {
      cerr << "*** ERROR: TCP transmit buffer overflow.\n";
      con->disconnect();
      clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
    }
  }
} /* SvxServer::sendMsg */


/*
 * This file has not been truncated
 */
