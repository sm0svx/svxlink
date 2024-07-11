/**
@file	 AsyncTcpServerBase.cpp
@brief   The base class for creating a TCP server
@author  Tobias Blomberg
@date	 2003-12-07

\verbatim
Async - A library for programming event driven applications
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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdio.h>

#include <iostream>
#include <algorithm>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
#include <AsyncApplication.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncTcpServer.h"



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

TcpServerBase::TcpServerBase(const string& port_str,
                             const Async::IpAddress &bind_ip)
  : sock(-1), rd_watch(0)
{
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    cleanup();
    return;
  }
  
  /* Force close on exec */
  if (fcntl(sock, F_SETFD, 1) == -1)
  {
    perror("fcntl(F_SETFD)");
    cleanup();
    return;
  }
  
    /* Reuse address if server crashes */
  const int on = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) == -1)
  {
    perror("setsockopt(sock, SO_REUSEADDR)");
    cleanup();
    return;
  }
  
    /* Send small packets at once. */
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on)) == -1)
  {
    perror("setsockopt(sock, TCP_NODELAY)");
    cleanup();
    return;
  }
  
  char *endptr = 0;
  struct servent *se;
  uint16_t port = strtol(port_str.c_str(), &endptr, 10);
  if (*endptr != '\0')
  {
    if ((se = getservbyname(port_str.c_str(), "tcp")) != NULL)
    {
      port = ntohs(se->s_port);
    }
    else
    {
      cerr << "Could not find service " << port_str << endl;
      cleanup();
      return;
    }
  }

  struct sockaddr_in addr = { 0 };
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  if (bind_ip.isEmpty())
  {
    addr.sin_addr.s_addr = INADDR_ANY;
  }
  else
  {
    addr.sin_addr = bind_ip.ip4Addr();
  }
  if (::bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
  {
    perror("bind");
    cleanup();
    return;
  }

  if (listen(sock, 5) != 0)
  {
    perror("listen");
    cleanup();
    return;
  }

  rd_watch = new FdWatch(sock, FdWatch::FD_WATCH_RD);
  rd_watch->activity.connect(mem_fun(*this, &TcpServerBase::onConnection));
    
} /* TcpServerBase::TcpServerBase */


TcpServerBase::~TcpServerBase(void)
{
  cleanup();
} /* TcpServerBase::~TcpServerBase */


int TcpServerBase::numberOfClients(void)
{
  return tcpConnectionList.size();
} /* TcpServerBase::numberOfClients */


TcpConnection *TcpServerBase::getClient(unsigned int index)
{
  if ((numberOfClients() > 0) && (index < tcpConnectionList.size()))
  {
    return tcpConnectionList[index];
  }
  
  return 0;

} /* TcpServerBase::getClient */


int TcpServerBase::writeAll(const void *buf, int count)
{
  if (tcpConnectionList.empty())
  {
    return 0;
  }
  
  TcpConnectionList::const_iterator it;
  for (it=tcpConnectionList.begin(); it!=tcpConnectionList.end() ; ++it)
  {
    (*it)->write(buf,count);
  }
  
  return count;

} /* TcpServerBase::writeAll */


int TcpServerBase::writeOnly(TcpConnection *con, const void *buf, int count)
{
  if (tcpConnectionList.empty())
  {
    return 0;
  }
  
  TcpConnectionList::const_iterator it;
  it = find(tcpConnectionList.begin(), tcpConnectionList.end(), con);
  assert(it != tcpConnectionList.end());
  (*it)->write(buf, count);
  
  return count;

} /* TcpServerBase::writeOnly */


int TcpServerBase::writeExcept(TcpConnection *con, const void *buf, int count)
{
  if (tcpConnectionList.empty())
  {
    return 0;
  }
  
  TcpConnectionList::const_iterator it;
  for (it=tcpConnectionList.begin(); it!=tcpConnectionList.end() ; ++it)
  {
    if(*it != con)
    {
      (*it)->write(buf, count);
    }
  }
  
  return count;

} /* TcpServerBase::writeExcept */


void TcpServerBase::setSslContext(SslContext& ctx)
{
  m_ssl_ctx = &ctx;
} /* TcpServerBase::setSslContext */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void TcpServerBase::addConnection(TcpConnection *con)
{
  if (m_ssl_ctx != nullptr)
  {
    con->setSslContext(*m_ssl_ctx, true);
  }
  tcpConnectionList.push_back(con);
} /* TcpServerBase::addConnection */


void TcpServerBase::removeConnection(TcpConnection *con)
{
  TcpConnectionList::iterator it;
  it = find(tcpConnectionList.begin(), tcpConnectionList.end(), con);
  assert(it != tcpConnectionList.end());
  tcpConnectionList.erase(it);
  Application::app().runTask([=]{ delete con; });
} /* TcpServerBase::removeConnection */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void TcpServerBase::cleanup(void)
{
  delete rd_watch;
  rd_watch = 0;
  
  if (sock != -1)
  {
    close(sock);
    sock = -1;
  }
  
    // If there are any connected clients, disconnect them and clear the list
  TcpConnectionList::const_iterator it;
  for (it=tcpConnectionList.begin(); it!=tcpConnectionList.end(); ++it)
  {
    delete *it;
  }
  tcpConnectionList.clear();
  
} /* TcpServerBase::cleanup */


void TcpServerBase::onConnection(FdWatch *watch)
{
  int client_sock;
  struct sockaddr_in client;
  socklen_t addrlen = sizeof(client);
  client_sock = accept(sock, (struct sockaddr *)&client, &addrlen);
  if (client_sock == -1)
  {
    perror("accept");
    return;
  }
  
    // Force close on exec
  if (fcntl(client_sock, F_SETFD, 1) == -1)
  {
    perror("fcntl(F_SETFD)");
    close(client_sock);
    return;
  }
  
    // Write must not block!
  if (fcntl(client_sock, F_SETFL, O_NONBLOCK) == -1)
  {
    perror("fcntl(client_sock, F_SETFL)");
    close(client_sock);
    return;
  }
  
    // Send small packets at once
  const int on = 1;
  if (setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY,
                 (char *)&on, sizeof(on)) == -1)
  {
    perror("setsockopt(client_sock, TCP_NODELAY)");
    close(client_sock);
    return;
  }
  
    // Create client object, add signal handling, add to client list
  createConnection(client_sock, IpAddress(client.sin_addr),
                   ntohs(client.sin_port));
} /* TcpServerBase::onConnection */


/*
 * This file has not been truncated
 */



