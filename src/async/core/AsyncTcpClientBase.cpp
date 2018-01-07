/**
@file	 AsyncTcpClientBase.cpp
@brief   Contains a class for creating TCP client connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class that make it easy to create a new TCP connection
to a remote host. See usage instructions in the class definition.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2017 Tobias Blomberg

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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstdio>
#include <cassert>
#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncFdWatch.h"
#include "AsyncDnsLookup.h"
#include "AsyncTcpClient.h"
#include "AsyncApplication.h"


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
 * Prototypes / Local functions
 *
 ****************************************************************************/

namespace {
  void deleteDnsObject(DnsLookup *dns) { delete dns; }
};


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


TcpClientBase::TcpClientBase(TcpConnection *con)
  : con(con), dns(0), sock(-1), wr_watch(0)
{
  wr_watch = new FdWatch;
  wr_watch->activity.connect(mem_fun(*this, &TcpClientBase::connectHandler));
} /* TcpClientBase::TcpClientBase */


TcpClientBase::TcpClientBase(TcpConnection *con, const string& remote_host,
                             uint16_t remote_port)
  : con(con), dns(0), remote_host(remote_host), sock(-1), wr_watch(0)
{
  IpAddress ip_addr(remote_host);
  if (!ip_addr.isEmpty())
  {
    con->setRemoteAddr(ip_addr);
    this->remote_host = ip_addr.toString();
  }
  con->setRemotePort(remote_port);
  wr_watch = new FdWatch;
  wr_watch->activity.connect(mem_fun(*this, &TcpClientBase::connectHandler));
} /* TcpClientBase::TcpClientBase */


TcpClientBase::TcpClientBase(TcpConnection *con, const IpAddress& remote_ip,
                             uint16_t remote_port)
  : con(con), dns(0), remote_host(remote_ip.toString()), sock(-1), wr_watch(0)
{
  con->setRemoteAddr(remote_ip);
  con->setRemotePort(remote_port);
  wr_watch = new FdWatch;
  wr_watch->activity.connect(mem_fun(*this, &TcpClientBase::connectHandler));
} /* TcpClientBase::TcpClientBase */


TcpClientBase::~TcpClientBase(void)
{
  disconnect();
  delete wr_watch;
  wr_watch = 0;
} /* TcpClientBase::~TcpClientBase */


void TcpClientBase::bind(const IpAddress& bind_ip)
{
  this->bind_ip = bind_ip;
} /* TcpClientBase::bind */


void TcpClientBase::connect(const string &remote_host, uint16_t remote_port)
{
  this->remote_host = remote_host;
  IpAddress ip_addr(remote_host);
  if (!ip_addr.isEmpty())
  {
    con->setRemoteAddr(ip_addr);
    this->remote_host = ip_addr.toString();
  }
  con->setRemotePort(remote_port);
  connect();
} /* TcpClientBase::connect */


void TcpClientBase::connect(const IpAddress& remote_ip, uint16_t remote_port)
{
  con->setRemoteAddr(remote_ip);
  remote_host = remote_ip.toString();
  con->setRemotePort(remote_port);
  connect();
} /* TcpClientBase::connect */


void TcpClientBase::connect(void)
{
    // Do nothing if DNS lookup is pending, connection is pending or if the
    // connection is already established
  if ((dns != 0) || (sock != -1) || (con->socket() != -1))
  {
    return;
  }

  if (con->remoteHost().isEmpty() ||
      (remote_host != con->remoteHost().toString()))
  {
    assert(!remote_host.empty());
    dns = new DnsLookup(remote_host);
    dns->resultsReady.connect(mem_fun(*this, &TcpClientBase::dnsResultsReady));
  }
  else
  {
    connectToRemote();
  }
} /* TcpClientBase::connect */


void TcpClientBase::disconnect(void)
{
  wr_watch->setEnabled(false);

  delete dns;
  dns = 0;
  
  if (sock != -1)
  {
    ::close(sock);
    sock = -1;
  }
} /* TcpClientBase::disconnect */



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

void TcpClientBase::dnsResultsReady(DnsLookup& dns_lookup)
{
  vector<IpAddress> result = dns->addresses();
  
    // Avoid memory leak by not deleting the dns object in the connected slot
  Application::app().runTask(sigc::bind(&deleteDnsObject, dns));
  dns = 0;
  
  if (result.empty() || result[0].isEmpty())
  {
    disconnect();
    con->onDisconnected(TcpConnection::DR_HOST_NOT_FOUND);
    return;
  }
  
  con->setRemoteAddr(result[0]);
  
  connectToRemote();
} /* TcpClientBase::dnsResultsReady */


void TcpClientBase::connectToRemote(void)
{
  assert(sock == -1);
  
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(con->remotePort());
  addr.sin_addr = con->remoteHost().ip4Addr();

    /* Create a TCP/IP socket to use */
  sock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
  {
    con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
    return;
  }

    /* Setup non-blocking operation */
  if (fcntl(sock, F_SETFL, O_NONBLOCK))
  {
    int errno_tmp = errno;
    disconnect();
    errno = errno_tmp;
    con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
    return;
  }

  if (!bind_ip.isEmpty())
  {
    struct sockaddr_in addr = { 0 };
    addr.sin_family = AF_INET;
    addr.sin_port = htons(0);
    addr.sin_addr = bind_ip.ip4Addr();
    if (::bind(sock, (struct sockaddr *)&addr, sizeof(addr)) != 0)
    {
      int errno_tmp = errno;
      disconnect();
      errno = errno_tmp;
      con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
      return;
    }
  }
    
    /* Connect to the server */
  int result = ::connect(sock, reinterpret_cast<struct sockaddr *>(&addr),
                         sizeof(addr));
  if (result == -1)
  {
    if (errno == EINPROGRESS)
    {
      wr_watch->setFd(sock, FdWatch::FD_WATCH_WR);
      wr_watch->setEnabled(true);
    }
    else
    {
      int errno_tmp = errno;
      disconnect();
      errno = errno_tmp;
      con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
      return;
    }
  }
  else
  {
    con->setSocket(sock);
    sock = -1;
    
    connected();
  }

} /* TcpClientBase::connectToRemote */


void TcpClientBase::connectHandler(FdWatch *watch)
{
  wr_watch->setEnabled(false);
  
  int error;
  socklen_t error_size = sizeof(error);
  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &error_size) == -1)
  {
    int errno_tmp = errno;
    disconnect();
    errno = errno_tmp;
    con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
    return;
  }
  if (error)
  {
    disconnect();
    errno = error;
    con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
    return;
  }
  
  con->setSocket(sock);
  sock = -1;
  
  connected();
  
} /* TcpClientBase::connectHandler */



/*
 * This file has not been truncated
 */

