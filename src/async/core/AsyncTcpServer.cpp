/**
@file	 AsyncTcpServer.cpp
@brief   A class for creating a TCP server
@author  Tobias Blomberg
@date	 2003-12-07

This class is used to create a TCP server that listens to a TCP-port.

\verbatim
Async - A library for programming event driven applications
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

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <iostream>
#include <stdio.h>
#include <algorithm>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


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


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::TcpServer
 * Purpose:   Consturctor
 * Input:     
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
TcpServer::TcpServer(const string& port_str, const Async::IpAddress &bind_ip)
  : sock(-1), rd_watch(0)
{
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
  {
    perror("socket");
    cleanup();
    return;
  }
  
  /* Force close on exec */
  fcntl(sock, F_SETFD, 1);
  
    /* Reuse address if server crashes */
  const int on = 1;
  setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on));
  
    /* Send small packets at once. */
  setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
  
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

  struct sockaddr_in addr;
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
  if (bind(sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) != 0)
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
  rd_watch->activity.connect(mem_fun(*this, &TcpServer::onConnection));
    
} /* TcpServer::TcpServer */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::~TcpServer
 * Purpose:   Destructor
 * Input:     
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
TcpServer::~TcpServer(void)
{
  cleanup();
} /* TcpServer::~TcpServer */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::numberOfClients
 * Purpose:   Get the number of clients that is connected to the server
 * Input:     None
 * Output:    Returns the number of connected clients
 * Author:    Ulf Larsson
 * Created:   2004-14-14
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
int TcpServer::numberOfClients(void)
{
  return tcpConnectionList.size();
} /* TcpServer::numberOfClients */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::getClient
 * Purpose:   Return TcpConnection pointer to the given client
 * Input:     Index of the requested client
 * Output:    Returns a TcpConnection pointer
 * Author:    Ulf Larsson
 * Created:   2004-14-14
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
TcpConnection *TcpServer::getClient(unsigned int index)
{
  if ((numberOfClients() > 0) && (index < tcpConnectionList.size()))
  {
    return tcpConnectionList[index];
  }
  
  return 0;

} /* TcpServer::getClient */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::writeAll
 * Purpose:   Write data to all connected clients 
 * Input:     buf   Data buffer to send to clients
 *            count Number of bytes in buf buffer
 * Output:    Number of bytes sent
 * Author:    Ulf Larsson
 * Created:   2004-14-14
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
int TcpServer::writeAll(const void *buf, int count)
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

} /* TcpServer::writeAll */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::writeOnly
 * Purpose:   Send data only to the given client
 * Input:     con   client
 *            but   Data buffer to send to clients
 *            count Number of bytes in the buffer
 * Output:    Number of bytes sent
 * Author:    Ulf Larsson
 * Created:   2004-14-14
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
int TcpServer::writeOnly(TcpConnection *con, const void *buf, int count)
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

} /* TcpServer::writeOnly */


/*
 *------------------------------------------------------------------------
 * Method:    TcpServer::writeExcept
 * Purpose:   Send data to all connected clients except the given client
 * Input:     con   client
 *            but   Data buffer to send to clients
 *            count Number of bytes in buf buffer
 * Output:    Number of sent bytes
 * Author:    Ulf Larsson
 * Created:   2004-14-14
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
int TcpServer::writeExcept(TcpConnection *con, const void *buf, int count)
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

} /* TcpServer::writeExcept */




/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    TcpServer::cleanup
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void TcpServer::cleanup(void)
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
  
} /* TcpServer::cleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    TcpServer::incomingConnection
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void TcpServer::onConnection(FdWatch *watch)
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
  fcntl(client_sock, F_SETFD, 1);
  
    // Write must not block!
  fcntl(client_sock, F_SETFL, O_NONBLOCK);
  
    // Send small packets at once
  const int on = 1;
  setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
  
    // Create client object, add signal handling, add to client list
  TcpConnection *con = new TcpConnection(client_sock,
      	  IpAddress(client.sin_addr), ntohs(client.sin_port));
  con->disconnected.connect(mem_fun(*this, &TcpServer::onDisconnected));
  tcpConnectionList.push_back(con);
  
    // Emit signal on client connection
  clientConnected(con);
  
} /* TcpServer::onConnection */


/*
 *----------------------------------------------------------------------------
 * Method:    TcpServer::onDisconnected
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Ulf Larsson
 * Created:   2004-09-14
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void TcpServer::onDisconnected(TcpConnection *con,
      	      	       	       TcpConnection::DisconnectReason reason)
{
    // Emit signal on client disconnection
  clientDisconnected(con, reason);
  
    // Remove client from client list
  TcpConnectionList::iterator it;
  it = find(tcpConnectionList.begin(), tcpConnectionList.end(), con);
  assert(it != tcpConnectionList.end());
  tcpConnectionList.erase(it);
  delete con;
  
} /* TcpServer::onDisconnected */




/*
 * This file has not been truncated
 */



