/**
@file	 svxserver.cpp
@brief   Main file for the svxserver
@author  Adi Bier / DL1HRC
@date	 2017-01-17

This is the main file for the svxserver remote transceiver for the
SvxLink server. It is used to link in remote transceivers to the SvxLink
server core (e.g. via a TCP/IP network).

\verbatim
svxserver - A svxlink server application
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
    cerr << "--- WARNING: Configuration variable LISTEN_PORT is missing."
         << "Setting default to 5210" << endl;
  }

  string value;
  cfg.getValue("GLOBAL", "SQL_TIMEOUT", value);
  sql_timeout = 1000 * atoi(value.c_str());
  if (sql_timeout < 1000)
  {
     // if SQL_TIMEOUT not set, define 60 seconds
    cerr << "--- WARNING: Configuration variable SQL_TIMEOUT is senseless or"
         << " missing. Setting default to 60 seconds" << endl;
    sql_timeout = 60000;
  }

  cfg.getValue("GLOBAL", "SQL_RESET_TIMEOUT", value);
  sql_resettimeout = 1000 * atoi(value.c_str());
  if (sql_resettimeout < 1000)
  {
     // if SQL_RESET_TIMEOUT not set, define 60 seconds
    cerr << "--- WARNING: Configuration variable SQL_RESET_TIMEOUT is senseless"
         << " or missing. Setting default to 60 seconds" << endl;
    sql_resettimeout = 60000;
  }

  cfg.getValue("GLOBAL", "AUTH_KEY", auth_key, true);

  hbto = 10000;
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

  cout << "--- setting sql-timeout to " << sql_timeout << " ms" << endl;
  cout << "--- starting SERVER on port " << port << endl;

   // create the server instance
  server = new Async::TcpServer<>(port);
  server->clientConnected.connect(mem_fun(*this, &SvxServer::clientConnected));
  server->clientDisconnected.connect(
      mem_fun(*this, &SvxServer::clientDisconnected));

  auth_msg = new MsgAuthChallenge;
  memcpy(auth_challenge, auth_msg->challenge(),
        MsgAuthChallenge::CHALLENGE_LEN);

  heartbeat_timer = new Async::Timer(hbto);
  heartbeat_timer->expired.connect(mem_fun(*this, &SvxServer::hbtimeout));
  heartbeat_timer->setEnable(false);

  sql_timer = new Async::Timer(sql_timeout);
  sql_timer->expired.connect(mem_fun(*this, &SvxServer::sqltimeout));
  sql_timer->setEnable(false);

  sql_resettimer = new Async::Timer(sql_resettimeout);
  sql_resettimer->expired.connect(mem_fun(*this, &SvxServer::sqlresettimeout));
  sql_resettimer->setEnable(false);

  audio_timer = new Async::Timer(1000);
  audio_timer->expired.connect(mem_fun(*this, &SvxServer::audiotimeout));
  audio_timer->setEnable(false);

  master = 0;
} /* SvxServer::SvxServer */


SvxServer::~SvxServer(void)
{
  clients.clear();
  delete server;
  delete master;
  delete heartbeat_timer;
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
  clpair.sql_open = false;  // set SQL close as default
  clpair.blocked = false;   // node is not blocked as default
  clpair.recv_exp = sizeof(Msg);
  clpair.recv_cnt = 0;
  gettimeofday(&clpair.last_msg, NULL);
  gettimeofday(&clpair.sent_msg, NULL);

  clients[key] = clpair;

  MsgProtoVer *ver_msg = new MsgProtoVer;
  sendMsg(con, ver_msg);

  if (auth_key.empty())
  {
    MsgAuthOk *auth_ok = new MsgAuthOk;
    sendMsg(con, auth_ok);
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

  if (clients.size() < 1)
  {
    heartbeat_timer->setEnable(false);
    heartbeat_timer->reset();
  }
  resetMaster(con);

  Clients::iterator it;
  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if ((*it).second.con == con)
    {
      (*it).second.state = STATE_DISC;
      (*it).second.sql_open = false;

        // If a station lost network connection it can't be
        // master anymore, send a SQL close command to all
        // connected stations
      if (isMaster(con))
      {
        MsgSquelch *ms = new MsgSquelch(false, 0.0, 1);
        sendMsg(con, ms);
      }
      break;
    }
  }

  if (it != clients.end())
  {
    cout << "-X- removing client " << con->remoteHost() << ":"
         << con->remotePort()  << " from client list" << endl;
    clients.erase(it);
  }
} /* SvxServer::clientDisconnected */


int SvxServer::tcpDataReceived(Async::TcpConnection *con, void *data, int size)
{

//  cout << "tcpDataReceived: " << con->remoteHost() << ":"
//       << con->remotePort() << endl;

  Clients::iterator it;
  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if (((*it).second).con == con)
    {
      break;
    }
  }

  if (it == clients.end())
  {
    cout << "--- tcp data received from station out of my list "
         << con->remoteHost() << ":" << con->remotePort() << endl;
    return size;
  }

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
          cerr << "*** ERROR: Illegal message header received in svxserver. "
               << "Header length too small (" << msg->size() << ")\n";
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

//  cout << "message <---------- " << con->remoteHost() << ":" 
//       << con->remotePort() << ", type=" << msg->type() << " received\n";

  int state = STATE_READY;
  Clients::iterator it;
  Clients t_clients;

  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if ( ((*it).second).con == con)
    {
      state = (*it).second.state;
      gettimeofday(&((*it).second).last_msg, NULL);
      break;
    }
  }

  if (it == clients.end())
  {
    cout << "-- message received from ip out of my list "
         << con->remoteHost() << ":" << con->remotePort() << endl;
    return;
  }

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

          // sending SQL close to connected node just to be sure that it 
          // isn't still open from former connects
          MsgTransmitterStateChange *txcl = new MsgTransmitterStateChange(false);
          sendMsg(con, txcl);
        }
      }
      else
      {
        cerr << "*** ERROR: Protocol error in svxserver.\n";
        ((*it).second).state = STATE_DISC;
        con->disconnect();
        clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
      }
      return;

    case STATE_READY:
      break;
  }

  Msg *cmsg = 0;

    // check type of message
  switch (msg->type())
  {
      // is heartbeat, send a heartbeat back to client
    case MsgHeartbeat::TYPE:
    {
      MsgHeartbeat *m = new MsgHeartbeat;
      sendMsg(con, m);
      return;
    }

#if 0
      // get callsign from remote station, not implemented yet
    case MsgRemoteCall::TYPE:
    {
      MsgRemoteCall *n = reinterpret_cast<MsgRemoteCall *>(msg);
      (*it).second.callsign = n->getCall();
      return;
    }
#endif

    case MsgSquelch::TYPE:
    {
      MsgSquelch *n = reinterpret_cast<MsgSquelch *>(msg);
      cmsg = n;
      break;
    }

    case MsgReset::TYPE:
    {
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
      // if SvxServer is receiving an audiostream and the SQL is still not
      // open, it will send a SQL=open command to all connected stations
      // may occur in case of a network error when the connection has been
      // reestablished
      if (!hasMaster())
      {
        audio_timer->setEnable(true);
        setMaster(con);
        MsgSquelch *ms = new MsgSquelch(true, 1.0, 1);
        sendExcept(con, ms);
        (*it).second.sql_open = true;

        // sends the audiostream to all connected clients without the
        // source client
        if ((*it).second.tx_mode != Tx::TX_AUTO)
        {
          (*it).second.tx_mode = Tx::TX_AUTO;
          MsgSetTxCtrlMode *n = new MsgSetTxCtrlMode(Tx::TX_AUTO);
          sendExcept(con, n);
          sendMsg(con, n);
        }
      }

      if (isMaster(con))
      {
        // sends the audiostream to all connected clients without the
        // source client if the source client is Master
        cmsg = reinterpret_cast<MsgAudio *>(msg);
        sendExcept(con, cmsg);
        audio_timer->reset();
      }
      return;
    }

    case MsgFlush::TYPE:
    {
      // reset the master to provide other stations to get the audio focus
      if (isMaster(con))
      {
        resetMaster(con);
        MsgSquelch  *ms = new MsgSquelch(false, 0.0, 1);
        sendExcept(con, ms);
        sendMsg(con, ms);

        (*it).second.sql_open = false;

        MsgAllSamplesFlushed *o = new MsgAllSamplesFlushed;
        sendMsg(con, o);
        cmsg = o;
        break;
      }
      else
      {
        /*
        / ignore all flush messages from nodes that's not master,
        / this can happen if a rx audiostream is triggerd when the
        / TX is keying up
        */
        return;
      }
    }

    case MsgSetMuteState::TYPE:
    {
      MsgSetMuteState *s = reinterpret_cast<MsgSetMuteState *>(msg);
        // wirklich notwendig?
      cmsg = s;
      break;
    }

    case MsgSetTxCtrlMode::TYPE:
    {
      MsgSetTxCtrlMode *s = reinterpret_cast<MsgSetTxCtrlMode *>(msg);

      // mode=1 means TX=true/on
      if (s->mode() == 1)
      {
        // the station with 1st SQL opening becomes a master
        setMaster(con);

        MsgTransmitterStateChange *n = new MsgTransmitterStateChange(true);
        sendMsg(con, n);
        sendExcept(con, n);

        MsgSquelch  *ms = new MsgSquelch(true, 1.0, 1);
        sendExcept(con, ms);
        (*it).second.sql_open = true;
        audio_timer->reset();
        audio_timer->setEnable(true);
      }

      // mode=2 means TX=AUTO
      if (s->mode() == 2)
      {
        resetMaster(con);
        (*it).second.sql_open = false;

        (*it).second.tx_mode = Tx::TX_AUTO;
        MsgSetTxCtrlMode *n = new MsgSetTxCtrlMode(Tx::TX_AUTO);
        sendExcept(con, n);

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


void SvxServer::audiotimeout(Timer *t)
{
  // is an audio stream not received anymore: reset
  if (hasMaster())
  {
    cout << "...Audio timeout " << master->remoteHost() << endl;
    resetAll();
  }
} /* SvxServer::audiotimeout */


void SvxServer::sqltimeout(Timer *t)
{
  Clients::iterator it;
  Clients t_clients;

  // find the connection handler that has a problem with
  // the SQL -> revoke the AUTH grant
  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if ( (*it).second.con == master)
    {
      (*it).second.state = STATE_DISC;
      (*it).second.blocked = true;
      cout << "*** WARNING: SQL on " << master->remoteHost() 
           << " has been open too long, blocking station." << endl;
      gettimeofday(&((*it).second).last_msg, NULL);
      break;
    }
  }

  resetAll();
} /* SvxServer::sqltimeout */


void SvxServer::sqlresettimeout(Timer *t)
{
  
} /* SvxServer::sqlresettimeout */


void SvxServer::resetAll(void)
{
  MsgSquelch *ms = new MsgSquelch(false, 0.0, 1);
  sendExcept(master, ms);
  sendMsg(master, ms);

  Clients::iterator it;
  Clients t_clients;

  for (it=clients.begin(); it!=clients.end(); it++)
  {
    if ( (*it).second.con == master)
    {
      (*it).second.sql_open = false;
      gettimeofday(&(*it).second.last_msg, NULL);
      break;
    }
  }

  MsgAllSamplesFlushed *o = new MsgAllSamplesFlushed;
  sendMsg(master, o);
  sendExcept(master, o);
  resetMaster(master);
} /* SvxServer::resetAll */


void SvxServer::hbtimeout(Timer *t)
{
  struct timeval t_time;
  struct timeval t_diff;
  int diff_ms;

  Clients::iterator it;
  Clients t_clients;
  MsgHeartbeat *m = new MsgHeartbeat;

  for (it=clients.begin(); it!=clients.end(); it++)
  {
    gettimeofday(&t_time, NULL);
    timersub(&t_time, &(*it).second.last_msg, &t_diff );
    diff_ms = int(t_diff.tv_sec * 1000 +  t_diff.tv_usec/1000);

      // if the difference more then 2*timeout, put the client
      // into the "disconnect pool"
    if (diff_ms > 2 * hbto)
    {
      cerr << "**** ERROR: Heartbeat timeout, lost connection to "
           << (*it).second.con->remoteHost() << ":"
           << (*it).second.con->remotePort() << endl;
      t_clients.insert(*it); // storing clients to be removed later
      (*it).second.state = STATE_DISC;
    }
    else 
    {
      sendMsg((*it).second.con, m);
    }
  }

  // removing client connection from connection pool
  for (it = t_clients.begin(); it != t_clients.end(); it++)
  {
    cout << "-X- disconnect client " << (*it).second.con->remoteHost() << ":"
         << (*it).second.con->remotePort() << endl;
    (*it).second.con->disconnect();
    clientDisconnected((*it).second.con, TcpConnection::DR_ORDERED_DISCONNECT);
  }

  t->reset();

} /* SvxServer::hbtimeout */


void SvxServer::sendExcept(Async::TcpConnection *con, Msg *msg)
{

    // copy map to prevent a segfault by erasing a connection
    // that exists anymore
  Clients t_clients = clients;
  Clients::iterator it;

    // sending data to connected clients without the source client
  for (it = t_clients.begin(); it != t_clients.end(); it++)
  {
    if ((*it).second.con != con)
    {
      sendMsg((*it).second.con, msg);
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
    cout << "*** ERROR: (" << con->remoteHost() << ":"
         << con->remotePort() << ") TCP transmit "
         << (written == -1 ? "error." : "buffer overflow.")
         << endl;
    con->disconnect();
    clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
  }
} /* SvxServer::sendMsg */


bool SvxServer::hasMaster()
{
  return (master != 0);
} /* SvxServer::hasMaster */


bool SvxServer::isMaster(Async::TcpConnection *con)
{
  return (con == master ? true : false);
} /* SvxServer::isMaster */


void SvxServer::setMaster(Async::TcpConnection *con)
{
  if (master == 0) {
    cout << "IS Master " << con->remoteHost() << endl;
    master = con;
    sql_timer->reset();
    sql_timer->setEnable(true);
  }
} /* SvxServer::setMaster */


void SvxServer::resetMaster(Async::TcpConnection *con)
{
  if (master == con)
  {
    cout << "NO Master " << con->remoteHost() << endl;
    master = 0;
    sql_timer->setEnable(false);
  }
} /* SvxServer::resetMaster */


/*
 * This file has not been truncated
 */
