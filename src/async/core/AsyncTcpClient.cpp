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
TcpClient::TcpClient(const string& remote_host, short remote_port,
    size_t recv_buf_len)
  : remote_host(remote_host), remote_port(remote_port),
    recv_buf_len(recv_buf_len), dns(0), sock(-1), rd_watch(0), wr_watch(0),
    recv_buf(0), recv_buf_cnt(0)
{
  recv_buf = new char[recv_buf_len];
} /* TcpClient::TcpClient */


TcpClient::~TcpClient(void)
{
  disconnect();
  delete [] recv_buf;
} /* TcpClient::~TcpClient */


void TcpClient::connect(void)
{
  if ((dns != 0) || (sock != -1))
  {
    return;
  }
  
  dns = new DnsLookup(remote_host);
  dns->resultsReady.connect(slot(this, &TcpClient::dnsResultsReady));
} /* TcpClient::connect */


void TcpClient::disconnect(void)
{
  recv_buf_cnt = 0;
  
  delete dns;
  dns = 0;
  
  delete wr_watch;
  wr_watch = 0;
  
  delete rd_watch;
  rd_watch = 0;
  
  if (sock != -1)
  {
    close(sock);
    sock = -1;  
  }
} /* TcpClient::disconnect */


int TcpClient::write(const void *buf, int count)
{
  assert(sock != -1);
  int cnt = ::write(sock, buf, count);
  if (cnt == -1)
  {
    disconnect();
    disconnected(DR_SYSTEM_ERROR);
  }
  else if (cnt < count)
  {
    sendBufferFull(true);
    wr_watch->setEnabled(true);
  }
  
  return cnt;
  
} /* TcpClient::write */



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
    disconnected(DR_HOST_NOT_FOUND);
    return;
  }
  
  connectToRemote(result[0]);
  
} /* TcpClient::dnsResultsReady */


void TcpClient::connectToRemote(const IpAddress& ip_addr)
{
  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(remote_port);
  addr.sin_addr = ip_addr.ip4Addr();

    /* Create a TCP/IP socket to use */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if (sock == -1)
  {
    disconnected(DR_SYSTEM_ERROR);
    return;
  }

    /* Setup non-blocking operation */
  if (fcntl(sock, F_SETFL, O_NONBLOCK))
  {
    disconnect();
    disconnected(DR_SYSTEM_ERROR);
    return;
  }
  
  rd_watch = new FdWatch(sock, FdWatch::FD_WATCH_RD);
  rd_watch->activity.connect(slot(this, &TcpClient::recvHandler));
  
  
    /* Connect to the server */
  int result = ::connect(sock, reinterpret_cast<struct sockaddr *>(&addr),
      	      	       sizeof(addr));
  if (result == -1)
  {
    if (errno == EINPROGRESS)
    {
      wr_watch = new FdWatch(sock, FdWatch::FD_WATCH_WR);
      wr_watch->activity.connect(slot(this, &TcpClient::connectHandler));
    }
    else
    {
      disconnect();
      disconnected(DR_SYSTEM_ERROR);
      return;
    }
  }
  else
  {
    wr_watch->setEnabled(false);
    connected();
  }

} /* TcpClient::connectToRemote */


void TcpClient::recvHandler(FdWatch *watch)
{
  //cout << "recv_buf_cnt=" << recv_buf_cnt << endl;
  //cout << "recv_buf_len=" << recv_buf_len << endl;
  
  if (recv_buf_cnt == recv_buf_len)
  {
    disconnect();
    disconnected(DR_RECV_BUFFER_OVERFLOW);
    return;
  }
  
  int cnt = read(sock, recv_buf+recv_buf_cnt, recv_buf_len-recv_buf_cnt);
  if (cnt == -1)
  {
    //cout << "System error!\n";
    disconnect();
    disconnected(DR_SYSTEM_ERROR);
    return;
  }
  else if (cnt == 0)
  {
    //cout << "Connection closed by remote host!\n";
    disconnect();
    disconnected(DR_REMOTE_DISCONNECTED);
    return;
  }
  
  recv_buf_cnt += cnt;
  size_t processed = dataReceived(recv_buf, recv_buf_cnt);
  //cout << "processed=" << processed << endl;
  if (processed >= recv_buf_cnt)
  {
    recv_buf_cnt = 0;
  }
  else
  {
    memmove(recv_buf, recv_buf + processed, recv_buf_cnt - processed);
    recv_buf_cnt = recv_buf_cnt - processed;
  }
  
} /* TcpClient::recvHandler */


void TcpClient::connectHandler(FdWatch *watch)
{
  delete wr_watch;
  wr_watch = 0;
  
  int error;
  socklen_t error_size = sizeof(error);
  if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &error_size) == -1)
  {
    disconnect();
    disconnected(DR_SYSTEM_ERROR);
    return;
  }
  if (error)
  {
    perror("delayed connect");
    disconnect();
    disconnected(DR_SYSTEM_ERROR);
    return;
  }
  
  wr_watch = new FdWatch(sock, FdWatch::FD_WATCH_WR);
  wr_watch->activity.connect(slot(this, &TcpClient::writeHandler));
  wr_watch->setEnabled(false);
  
  connected();
  
} /* TcpClient::connectHandler */


void TcpClient::writeHandler(FdWatch *watch)
{
  watch->setEnabled(false);
  sendBufferFull(false);
} /* TcpClient::writeHandler */



/*
 * This file has not been truncated
 */

