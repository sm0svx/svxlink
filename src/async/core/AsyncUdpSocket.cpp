/**
 * @file    AsyncUdpSocket.cpp
 * @brief   Contains a class for using UDP sockets
 * @author  Tobias Blomberg
 * @date    2003-04-26
 *
 * This file contains a class for communication over a UDP sockets.
 *
 * \verbatim
 * Async - A library for programming event driven applications
 * Copyright (C) 2003  Tobias Blomberg
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * \endverbatim
 */




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>

#include <cerrno>
#include <cstdio>
#include <cstring>


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

#include "AsyncIpAddress.h"
#include "AsyncUdpSocket.h"



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

class UdpPacket
{
  public:
    const IpAddress ip;
    int       	    port;
    char    	    buf[65535];
    int       	    len;
    
    UdpPacket(const IpAddress& ip, int port, const void *buf, int len)
      : ip(ip), port(port), len(len)
    {
      memcpy(this->buf, buf, len);
    }
  
};


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
UdpSocket::UdpSocket(uint16_t local_port, const IpAddress &bind_ip)
  : sock(-1), rd_watch(0), wr_watch(0), send_buf(0)
{
    // Create UDP socket
  sock = socket(AF_INET, SOCK_DGRAM, 0);
  if(sock == -1)
  {
    perror("socket");
    cleanup();
    return;
  }
  
    // Setup the socket for non-blocking operation
  if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1)
  {
    perror("fcntl");
    cleanup();
    return;
  }
  
    // Bind the socket to a local port if one was specified
  if (local_port > 0)
  {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(local_port);
    if (bind_ip.isEmpty())
    {
      addr.sin_addr.s_addr = INADDR_ANY;
    }
    else
    {
      addr.sin_addr = bind_ip.ip4Addr();
    }
    if(::bind(sock, reinterpret_cast<struct sockaddr *>(&addr),
              sizeof(addr)) == -1)
    {
      perror("bind");
      cleanup();
      return;
    }
  }

    // Setup a watch for incoming data
  rd_watch = new FdWatch(sock, FdWatch::FD_WATCH_RD);
  assert(rd_watch != 0);
  rd_watch->activity.connect(mem_fun(*this, &UdpSocket::handleInput));

    // Setup a watch for outgoing data (signals activity when a buffer full
    // condition occurs)
  wr_watch = new FdWatch(sock, FdWatch::FD_WATCH_WR);
  assert(wr_watch != 0);
  wr_watch->activity.connect(mem_fun(*this, &UdpSocket::sendRest));
  wr_watch->setEnabled(false);
  
} /* UdpSocket::UdpSocket */


UdpSocket::~UdpSocket(void)
{
  cleanup();
} /* UdpSocket::~UdpSocket */


Async::IpAddress UdpSocket::localAddr(void) const
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int ret = getsockname(sock, reinterpret_cast<struct sockaddr *>(&addr),
                        &len);
  if ((ret != 0) || (len != sizeof(addr)))
  {
    perror("getsockname");
    return Async::IpAddress();
  }
  //std::cout << "### UdpSocket::localAddr: sin_addr="
  //          << Async::IpAddress(addr.sin_addr) << std::endl;
  return Async::IpAddress(addr.sin_addr);
} /* UdpSocket::localAddr */


uint16_t UdpSocket::localPort(void) const
{
  struct sockaddr_in addr;
  socklen_t len = sizeof(addr);
  int ret = getsockname(sock, reinterpret_cast<struct sockaddr *>(&addr),
                        &len);
  if ((ret != 0) || (len != sizeof(addr)))
  {
    perror("getsockname");
    return 0;
  }
  //std::cout << "### UdpSocket::localPort: sin_port="
  //          << ntohs(addr.sin_port) << std::endl;
  return ntohs(addr.sin_port);
} /* UdpSocket::localPort */


bool UdpSocket::write(const IpAddress& remote_ip, int remote_port,
    const void *buf, int count)
{
  if (send_buf != 0)
  {
    return false;
  }
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(remote_port);
  addr.sin_addr = remote_ip.ip4Addr();
  int ret = sendto(sock, buf, count, 0,
      reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret == -1)
  {
    if (errno == EAGAIN)
    {
      send_buf = new UdpPacket(remote_ip, remote_port, buf, count);
      wr_watch->setEnabled(true);
      sendBufferFull(true);
      return true;
    }
    else
    {
      perror("sendto in UdpSocket::write");
      return false;
    }
  }
  assert(ret == count);
  
  return true;
  
} /* UdpSocket::write */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void UdpSocket::onDataReceived(const IpAddress& ip, uint16_t port, void* buf,
    int count)
{
  dataReceived(ip, port, buf, count);
} /* UdpSocket::onDataReceived */


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
void UdpSocket::cleanup(void)
{
  delete rd_watch;
  rd_watch = 0;
  
  delete wr_watch;
  wr_watch = 0;
  
  delete send_buf;
  send_buf = 0;
  
  if (sock != -1)
  {
    if (close(sock) == -1)
    {
      perror("close");
    }
    sock = -1;
  }
} /* UdpSocket::cleanup */


void UdpSocket::handleInput(FdWatch *watch)
{
  char buf[65536];
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  
  int len = recvfrom(sock, buf, sizeof(buf), 0,
      reinterpret_cast<struct sockaddr *>(&addr), &addr_len);
  if (len == -1)
  {
    perror("recvfrom in UdpSocket::handleInput");
    return;
  }

  onDataReceived(IpAddress(addr.sin_addr), ntohs(addr.sin_port), buf, len);
} /* UdpSocket::handleInput */


void UdpSocket::sendRest(FdWatch *watch)
{
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(send_buf->port);
  addr.sin_addr = send_buf->ip.ip4Addr();
  /*
  cout << "sock=" << sock << "  port=" << send_buf->port
      << "  ip=" << send_buf->ip.toString() << "  len=" << send_buf->len
      << endl;
  */
  int ret = sendto(sock, send_buf->buf, send_buf->len, 0,
      reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret == -1)
  {
    if (errno == EAGAIN)
    {
      return;
    }
    else
    {
      perror("sendto in UdpSocket::sendRest");
    }
  }
  else
  {
    assert(ret == send_buf->len);
    sendBufferFull(false);
  }
  
  delete send_buf;
  send_buf = 0;
  wr_watch->setEnabled(false);
  
} /* UdpSocket::sendRest */






/*
 * This file has not been truncated
 */

