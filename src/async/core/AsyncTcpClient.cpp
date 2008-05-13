/**
@file	 AsyncTcpClient.cpp
@brief   Contains a class for creating TCP client connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class that make it easy to create a new TCP connection
to a remote host. See usage instructions in the class definition.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003  Tobias Blomberg

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
TcpClient::TcpClient(const string& remote_host, uint16_t remote_port,
    size_t recv_buf_len)
  : TcpConnection(recv_buf_len), dns(0), remote_host(remote_host),
    remote_port(remote_port), sock(-1), wr_watch(0)
{
  
} /* TcpClient::TcpClient */


TcpClient::~TcpClient(void)
{
  disconnect();
} /* TcpClient::~TcpClient */


void TcpClient::connect(void)
{
  if ((dns != 0) || (sock != -1) || (socket() != -1))
  {
    return;
  }
  
  assert(dns == 0);
  
  dns = new DnsLookup(remote_host);
  dns->resultsReady.connect(slot(*this, &TcpClient::dnsResultsReady));
} /* TcpClient::connect */


void TcpClient::disconnect(void)
{
  TcpConnection::disconnect();

  delete wr_watch;
  wr_watch = 0;

  delete dns;
  dns = 0;
  
  if (sock != -1)
  {
    ::close(sock);
    sock = -1;
  }
  
} /* TcpClient::disconnect */



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
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void TcpClient::dnsResultsReady(DnsLookup& dns_lookup)
{
  vector<IpAddress> result = dns->addresses();
  
  delete dns;
  dns = 0;
  
  if (result.empty() || (result[0].ip4Addr().s_addr == INADDR_NONE))
  {
    disconnect();
    disconnected(this, DR_HOST_NOT_FOUND);
    return;
  }
  
  connectToRemote(result[0]);
  
} /* TcpClient::dnsResultsReady */


void TcpClient::connectToRemote(const IpAddress& ip_addr)
{
  setRemoteAddr(ip_addr);
  setRemotePort(remote_port);
  
  assert(sock == -1);
  
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(remote_port);
  addr.sin_addr = ip_addr.ip4Addr();

    /* Create a TCP/IP socket to use */
  sock = ::socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
  {
    disconnected(this, DR_SYSTEM_ERROR);
    return;
  }

    /* Setup non-blocking operation */
  if (fcntl(sock, F_SETFL, O_NONBLOCK))
  {
    int errno_tmp = errno;
    disconnect();
    errno = errno_tmp;
    disconnected(this, DR_SYSTEM_ERROR);
    return;
  }
    
    /* Connect to the server */
  int result = ::connect(sock, reinterpret_cast<struct sockaddr *>(&addr),
      	      	       sizeof(addr));
  if (result == -1)
  {
    if (errno == EINPROGRESS)
    {
      wr_watch = new FdWatch(sock, FdWatch::FD_WATCH_WR);
      wr_watch->activity.connect(slot(*this, &TcpClient::connectHandler));
    }
    else
    {
      int errno_tmp = errno;
      disconnect();
      errno = errno_tmp;
      disconnected(this, DR_SYSTEM_ERROR);
      return;
    }
  }
  else
  {
    setSocket(sock);
    sock = -1;
    
    connected();
  }

} /* TcpClient::connectToRemote */


void TcpClient::connectHandler(FdWatch *watch)
{
  delete wr_watch;
  wr_watch = 0;
  
  int error;
  socklen_t error_size = sizeof(error);
  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &error_size) == -1)
  {
    int errno_tmp = errno;
    disconnect();
    errno = errno_tmp;
    disconnected(this, DR_SYSTEM_ERROR);
    return;
  }
  if (error)
  {
    disconnect();
    errno = error;
    disconnected(this, DR_SYSTEM_ERROR);
    return;
  }
  
  setSocket(sock);
  sock = -1;
  
  connected();
  
} /* TcpClient::connectHandler */



/*
 * This file has not been truncated
 */

