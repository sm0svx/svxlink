/**
@file	 NetTrxTcpClient.cpp
@brief   Network connection manager for remote transceivers
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-15

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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
    con = new NetTrxTcpClient(remote_host, remote_port, RECV_BUF_SIZE);
    clients[key] = con;
  }
  
  return con;
  
} /* NetTrxTcpClient::instance */


void NetTrxTcpClient::sendMsg(Msg *msg)
{
  if (isConnected())
  {
    int written = write(msg, msg->size());
    if (written != static_cast<int>(msg->size()))
    {
      if (written == -1)
      {
      	cerr << "*** ERROR: " << strerror(errno) << endl;
      }
      else
      {
      	cerr << "*** ERROR: TCP transmit buffer overflow.\n";
      }
      disconnect();
      disconnected(this, TcpConnection::DR_ORDERED_DISCONNECT);
    }
  }
  
  delete msg;
  
} /* NetTrxTcpClient::sendMsg */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

NetTrxTcpClient::NetTrxTcpClient(const std::string& remote_host,
      	      	      	      	 uint16_t remote_port, size_t recv_buf_len)
  : TcpClient(remote_host, remote_port, recv_buf_len), recv_cnt(0),
    recv_exp(0), reconnect_timer(0)
{
  connected.connect(slot(*this, &NetTrxTcpClient::tcpConnected));
  disconnected.connect(slot(*this, &NetTrxTcpClient::tcpDisconnected));
  dataReceived.connect(slot(*this, &NetTrxTcpClient::tcpDataReceived));

  reconnect_timer = new Timer(10000);
  reconnect_timer->setEnable(false);
  reconnect_timer->expired.connect(slot(*this, &NetTrxTcpClient::reconnect));
  
} /* NetTrxTcpClient::NetTrxTcpClient */


NetTrxTcpClient::~NetTrxTcpClient(void)
{
  delete reconnect_timer;  
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
} /* NetTx::tcpConnected */


void NetTrxTcpClient::tcpDisconnected(TcpConnection *con,
      	      	      	    TcpConnection::DisconnectReason reason)
{
  recv_exp = 0;
  reconnect_timer->setEnable(true);
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
    int read_cnt = min(size, recv_exp-recv_cnt);
    if (recv_cnt+read_cnt > static_cast<int>(sizeof(recv_buf)))
    {
      cerr << "*** ERROR: TCP receive buffer overflow. Disconnecting...\n";
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
	if (recv_exp == static_cast<int>(msg->size()))
	{
	  msgReceived(msg);
	  recv_cnt = 0;
	  recv_exp = sizeof(Msg);
	}
	else
	{
      	  recv_exp = msg->size();
	}
      }
      else
      {
      	Msg *msg = reinterpret_cast<Msg*>(recv_buf);
      	msgReceived(msg);
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



/*
 * This file has not been truncated
 */
