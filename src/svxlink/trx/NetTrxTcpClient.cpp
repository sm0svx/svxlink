/**
@file	 NetTrxTcpClient.cpp
@brief   Network connection manager for remote transceivers
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-15

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

#include <cerrno>
#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NetTrxTcpClient.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace NetTrxMsg;



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

std::map<std::pair<const std::string, uint16_t>, NetTrxTcpClient*>
      	NetTrxTcpClient::clients;



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

NetTrxTcpClient *NetTrxTcpClient::instance(const std::string& remote_host,
      	      	      	      	      	   uint16_t remote_port)
{
  pair<const string, uint16_t> key(remote_host, remote_port);
  NetTrxTcpClient *con = 0;
  
  if (clients.find(key) != clients.end())
  {
    con = clients[key];
  }
  else
  {
      // Initialize the GCrypt library if not already initialized
    if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
    {
      gcry_check_version(NULL);
      gcry_error_t err;
      err = gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
      if (err != GPG_ERR_NO_ERROR)
      {
        cerr << "*** ERROR: Failed to initialize the Libgcrypt library: "
             << gcry_strsource(err) << "/" << gcry_strerror(err) << endl;
        return 0;
      }
        // Tell Libgcrypt that initialization has completed
      err = gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
      if (err != GPG_ERR_NO_ERROR)
      {
        cerr << "*** ERROR: Failed to initialize the Libgcrypt library: "
             << gcry_strsource(err) << "/" << gcry_strerror(err) << endl;
        return 0;
      }
    }

    con = new NetTrxTcpClient(remote_host, remote_port, RECV_BUF_SIZE);
    clients[key] = con;
  }
  
  con->user_cnt += 1;

  return con;
  
} /* NetTrxTcpClient::instance */


void NetTrxTcpClient::deleteInstance(void)
{
  user_cnt -= 1;
  assert(user_cnt >= 0);
  if (user_cnt == 0)
  {
    Clients::iterator it;
    for (it=clients.begin(); it!=clients.end(); ++it)
    {
      if ((*it).second == this)
      {
      	break;
      }
    }
    assert(it != clients.end());
    clients.erase(it);
    delete this;
  }
} /* NetTrxTcpClient::deleteInstance */


void NetTrxTcpClient::sendMsg(Msg *msg)
{
  if (state == STATE_READY)
  {
    sendMsgP(msg);
  }
  else
  { 
    delete msg;
  }
} /* NetTrxTcpClient::sendMsg */


void NetTrxTcpClient::connect(void)
{
  if (isIdle())
  {
    TcpClient<>::connect();
  }
} /* NetTrxTcpClient::connect */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

NetTrxTcpClient::NetTrxTcpClient(const std::string& remote_host,
      	      	      	      	 uint16_t remote_port, size_t recv_buf_len)
  : TcpClient<>(remote_host, remote_port, recv_buf_len), recv_cnt(0),
    recv_exp(0), reconnect_timer(0), last_msg_timestamp(), heartbeat_timer(0),
    user_cnt(0), state(STATE_DISC), disc_reason(DR_SYSTEM_ERROR)
{
  connected.connect(mem_fun(*this, &NetTrxTcpClient::tcpConnected));
  disconnected.connect(mem_fun(*this, &NetTrxTcpClient::tcpDisconnected));
  dataReceived.connect(mem_fun(*this, &NetTrxTcpClient::tcpDataReceived));

  reconnect_timer = new Timer(20000);
  reconnect_timer->setEnable(false);
  reconnect_timer->expired.connect(mem_fun(*this, &NetTrxTcpClient::reconnect));
  
  heartbeat_timer = new Timer(10000);
  heartbeat_timer->setEnable(false);
  heartbeat_timer->expired.connect(mem_fun(*this, &NetTrxTcpClient::heartbeat));
  
} /* NetTrxTcpClient::NetTrxTcpClient */


NetTrxTcpClient::~NetTrxTcpClient(void)
{
  delete reconnect_timer;
  delete heartbeat_timer;
} /* NetTrxTcpClient::~NetTrxTcpClient */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void NetTrxTcpClient::tcpConnected(void)
{
  recv_cnt = 0;
  recv_exp = sizeof(Msg);
  gettimeofday(&last_msg_timestamp, NULL);
  heartbeat_timer->setEnable(true);
  state = STATE_VER_WAIT;
} /* NetTx::tcpConnected */


void NetTrxTcpClient::tcpDisconnected(TcpConnection *con,
      	      	      	    TcpConnection::DisconnectReason reason)
{
  disc_reason = reason;
  recv_exp = 0;
  state = STATE_DISC;
  reconnect_timer->setEnable(true);
  heartbeat_timer->setEnable(false);
  isReady(false);
} /* NetTrxTcpClient::tcpDisconnected */


int NetTrxTcpClient::tcpDataReceived(TcpConnection *con, void *data, int size)
{
  //cout << "NetTrxTcpClient::tcpDataReceived: size=" << size << endl;
  
  //Msg *msg = reinterpret_cast<Msg*>(data);
  //cout << "Received a TCP message with type " << msg->type()
  //     << " and size " << msg->size() << endl;
  
  int orig_size = size;
  
  char *buf = static_cast<char*>(data);
  while (size > 0)
  {
    unsigned read_cnt = min(static_cast<unsigned>(size), recv_exp-recv_cnt);
    if (recv_cnt+read_cnt > sizeof(recv_buf))
    {
      cerr << "*** ERROR: TCP receive buffer overflow. Disconnecting from "
           << remoteHost().toString() << ":" << remotePort() << "...\n";
      con->disconnect();
      disconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
      return orig_size;
    }
    memcpy(recv_buf+recv_cnt, buf, read_cnt);
    size -= read_cnt;
    recv_cnt += read_cnt;
    buf += read_cnt;
    
    if (recv_cnt == recv_exp)
    {
      if (recv_exp == sizeof(Msg))
      {
      	Msg *msg = reinterpret_cast<Msg*>(recv_buf);
	if (msg->size() == sizeof(Msg))
	{
	  handleMsg(msg);
	  recv_cnt = 0;
	  recv_exp = sizeof(Msg);
	}
	else if (msg->size() > sizeof(Msg))
	{
      	  recv_exp = msg->size();
	}
	else
	{
	  cerr << "*** ERROR: Illegal message header received. Header length "
	       << "too small (" << msg->size() << "). Disconnecting from "
               << remoteHost().toString() << ":" << remotePort() << "...\n";
	  con->disconnect();
	  disconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
	  return orig_size;
	}
      }
      else
      {
      	Msg *msg = reinterpret_cast<Msg*>(recv_buf);
	handleMsg(msg);
	recv_cnt = 0;
	recv_exp = sizeof(Msg);
      }
    }
  }
  
  return orig_size;
  
} /* NetTrxTcpClient::tcpDataReceived */


void NetTrxTcpClient::reconnect(Timer *t)
{
  reconnect_timer->setEnable(false);
  connect();
} /* NetTrxTcpClient::reconnect */


void NetTrxTcpClient::handleMsg(Msg *msg)
{
  //cout << "type=" << msg->type() << " state=" << state << endl;
  
  switch (state)
  {
    case STATE_DISC:
      return;
      
    case STATE_VER_WAIT:
      if (msg->type() == MsgProtoVer::TYPE)
      {
        MsgProtoVer *ver_msg = reinterpret_cast<MsgProtoVer *>(msg);
        if ((msg->size() != sizeof(MsgProtoVer)) ||
            (ver_msg->majorVer() != MsgProtoVer::MAJOR) ||
            (ver_msg->minorVer() != MsgProtoVer::MINOR))
        {
          cerr << "*** ERROR: Incompatible protocol version. Disconnecting from "
               << remoteHost().toString() << ":" << remotePort() << "...\n";
          localDisconnect();
          return;
        }
        cout << remoteHost().toString() << ":" << remotePort()
             << ": RemoteTrx protocol version " << ver_msg->majorVer() << "."
             << ver_msg->minorVer() << endl;
        state = STATE_AUTH_WAIT;
      }
      else
      {
        cerr << "*** ERROR: No protocol version received. Disconnecting from "
               << remoteHost().toString() << ":" << remotePort() << "...\n";
        localDisconnect();
      }
      return;
    
    case STATE_AUTH_WAIT:
      if (msg->type() == MsgAuthChallenge::TYPE)
      {
        if (msg->size() != sizeof(MsgAuthChallenge))
        {
          cerr << "*** ERROR: Protocol error. Wrong length of "
                  "MsgAuthChallenge message. Disconnecting from "
               << remoteHost().toString() << ":" << remotePort() << "...\n";
          localDisconnect();
          return;
        }
        MsgAuthChallenge *chal_msg = reinterpret_cast<MsgAuthChallenge*>(msg);
        MsgAuthResponse *resp_msg =
            new MsgAuthResponse(auth_key, chal_msg->challenge());
        sendMsgP(resp_msg);
      }
      else if (msg->type() == MsgAuthOk::TYPE)
      {
        if (msg->size() != sizeof(MsgAuthOk))
        {
          cerr << "*** ERROR: Protocol error. Wrong length of "
                  "MsgAuthOk message. Disconnecting from "
               << remoteHost().toString() << ":" << remotePort() << "...\n";
          localDisconnect();
          return;
        }
        state = STATE_READY;
        isReady(true);
      }
      return;
    
    case STATE_READY:
      break;
  }
  
  gettimeofday(&last_msg_timestamp, NULL);
  
  switch (msg->type())
  {
    case MsgHeartbeat::TYPE:
    {
      break;
    }
    
    case MsgProtoVer::TYPE:
    case MsgAuthChallenge::TYPE:
    case MsgAuthOk::TYPE:
      cerr << "*** ERROR: Message type " << msg->type()
           << " received in the wrong state. Disconnecting from "
               << remoteHost().toString() << ":" << remotePort() << "...\n";
      localDisconnect();
      break;
    
    default:
      msgReceived(msg);
      break;
  }
  
  
} /* NetTrxTcpClient::handleMsg */


void NetTrxTcpClient::heartbeat(Timer *t)
{
  MsgHeartbeat *msg = new MsgHeartbeat;
  sendMsgP(msg);
  
  struct timeval diff_tv;
  struct timeval now;
  gettimeofday(&now, NULL);
  timersub(&now, &last_msg_timestamp, &diff_tv);
  int diff_ms = diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000;
  
  if (diff_ms > 15000)
  {
    cerr << "*** ERROR: Heartbeat timeout. Disconnecting from "
         << remoteHost().toString() << ":" << remotePort() << "...\n";
    localDisconnect();
  }
  
  t->reset();
  
} /* NetTrxTcpClient::heartbeat */


void NetTrxTcpClient::localDisconnect(void)
{
  disconnect();
  disconnected(this, TcpConnection::DR_ORDERED_DISCONNECT);
} /* NetTrxTcpClient::localDisconnect */


void NetTrxTcpClient::sendMsgP(Msg *msg)
{
  assert(isConnected());

  int written = write(msg, msg->size());
  if (written != static_cast<int>(msg->size()))
  {
    if (written == -1)
    {
      cerr << "*** ERROR: TCP write error: " << strerror(errno) << "\n";
    }
    else
    {
      cerr << "*** ERROR: TCP transmit buffer overflow. Disconnecting from "
         << remoteHost().toString() << ":" << remotePort() << "...\n";
    }
    disconnect();
    disconnected(this, TcpConnection::DR_ORDERED_DISCONNECT);
  }
  
  delete msg;
  
} /* NetTrxTcpClient::sendMsgP */



/*
 * This file has not been truncated
 */
