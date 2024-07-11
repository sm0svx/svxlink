/**
@file	 AsyncTcpClientBase.cpp
@brief   Contains a class for creating TCP client connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class that make it easy to create a new TCP connection
to a remote host. See usage instructions in the class definition.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

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
  : con(con), sock(-1)
{
  wr_watch.activity.connect(mem_fun(*this, &TcpClientBase::connectHandler));
  dns.resultsReady.connect(mem_fun(*this, &TcpClientBase::dnsResultsReady));
} /* TcpClientBase::TcpClientBase */


TcpClientBase::TcpClientBase(TcpConnection *con, const string& remote_host,
                             uint16_t remote_port)
  : con(con), sock(-1)
{
  IpAddress ip_addr(remote_host);
  if (!ip_addr.isEmpty())
  {
    con->setRemoteAddr(ip_addr);
  }
  else
  {
    dns.setLookupParams(remote_host);
  }
  con->setRemotePort(remote_port);
  wr_watch.activity.connect(mem_fun(*this, &TcpClientBase::connectHandler));
  dns.resultsReady.connect(mem_fun(*this, &TcpClientBase::dnsResultsReady));
} /* TcpClientBase::TcpClientBase */


TcpClientBase::TcpClientBase(TcpConnection *con, const IpAddress& remote_ip,
                             uint16_t remote_port)
  : con(con), sock(-1)
{
  con->setRemoteAddr(remote_ip);
  con->setRemotePort(remote_port);
  wr_watch.activity.connect(mem_fun(*this, &TcpClientBase::connectHandler));
  dns.resultsReady.connect(mem_fun(*this, &TcpClientBase::dnsResultsReady));
} /* TcpClientBase::TcpClientBase */


TcpClientBase::~TcpClientBase(void)
{
  closeConnection();
} /* TcpClientBase::~TcpClientBase */


TcpClientBase& TcpClientBase::operator=(TcpClientBase&& other)
{
  //std::cout << "### TcpClientBase::operator=(TcpClientBase&&)" << std::endl;
  *con = std::move(*other.con);

  dns = std::move(other.dns);

  sock = other.sock;
  other.sock = -1;

  wr_watch = std::move(other.wr_watch);

  bind_ip = other.bind_ip;
  other.bind_ip.clear();

  return *this;
} /* TcpClientBase::operator= */


void TcpClientBase::setBindIp(const IpAddress& bind_ip)
{
  this->bind_ip = bind_ip;
} /* TcpClientBase::setBindIp */


void TcpClientBase::connect(const string &remote_host, uint16_t remote_port)
{
  assert(isIdle() && con->isIdle());

  IpAddress ip_addr(remote_host);
  if (!ip_addr.isEmpty())
  {
    con->setRemoteAddr(ip_addr);
    dns.setLookupParams("");
  }
  else
  {
    dns.setLookupParams(remote_host);
  }
  con->setRemotePort(remote_port);
  connect();
} /* TcpClientBase::connect */


void TcpClientBase::connect(const IpAddress& remote_ip, uint16_t remote_port)
{
  assert(isIdle() && con->isIdle());

  con->setRemoteAddr(remote_ip);
  con->setRemotePort(remote_port);
  dns.setLookupParams("");
  connect();
} /* TcpClientBase::connect */


void TcpClientBase::connect(void)
{
  assert(isIdle() && con->isIdle());

  //m_successful_connect = false;

  if (!dns.label().empty())
  {
    dns.lookup();
  }
  else
  {
    connectToRemote();
  }
} /* TcpClientBase::connect */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void TcpClientBase::closeConnection(void)
{
  wr_watch.setEnabled(false);

  dns.abort();

  if (sock != -1)
  {
    ::close(sock);
    sock = -1;
  }
} /* TcpClientBase::closeConnection */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void TcpClientBase::dnsResultsReady(DnsLookup& dns_lookup)
{
  for (const auto& addr : dns.addresses())
  {
    if (!addr.isEmpty())
    {
      con->setRemoteAddr(addr);
      connectToRemote();
      return;
    }
  }

  closeConnection();
  con->onDisconnected(TcpConnection::DR_HOST_NOT_FOUND);
} /* TcpClientBase::dnsResultsReady */


void TcpClientBase::connectToRemote(void)
{
  assert(sock == -1);
  assert(!con->remoteHost().isEmpty());
  assert(con->remotePort() > 0);

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
    closeConnection();
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
      closeConnection();
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
      wr_watch.setFd(sock, FdWatch::FD_WATCH_WR);
      wr_watch.setEnabled(true);
    }
    else
    {
      int errno_tmp = errno;
      closeConnection();
      errno = errno_tmp;
      con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
      return;
    }
  }
  else
  {
    con->setSocket(sock);
    sock = -1;

    connectionEstablished();
  }

} /* TcpClientBase::connectToRemote */


void TcpClientBase::connectHandler(FdWatch *watch)
{
  wr_watch.setEnabled(false);
  
  int error;
  socklen_t error_size = sizeof(error);
  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &error_size) == -1)
  {
    int errno_tmp = errno;
    closeConnection();
    errno = errno_tmp;
    con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
    return;
  }
  if (error)
  {
    closeConnection();
    errno = error;
    con->onDisconnected(TcpConnection::DR_SYSTEM_ERROR);
    return;
  }
  
  con->setSocket(sock);
  sock = -1;

  connectionEstablished();

} /* TcpClientBase::connectHandler */



/*
 * This file has not been truncated
 */

