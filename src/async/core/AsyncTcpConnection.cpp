/**
@file	 AsyncTcpConnection.cpp
@brief   Contains a class for handling exiting TCP connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class to handle exiting TCP connections
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
#include <assert.h>

#include <cerrno>
#include <cstdio>
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
#include "AsyncTcpConnection.h"



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

const char *TcpConnection::disconnectReasonStr(DisconnectReason reason)
{
  switch (reason)
  {
    case DR_HOST_NOT_FOUND:
      return "Host not found";

    case DR_REMOTE_DISCONNECTED:
      return "Connection closed by remote peer";

    case DR_SYSTEM_ERROR:
      return strerror(errno);

    case DR_RECV_BUFFER_OVERFLOW:
      return "Receiver buffer overflow";

    case DR_ORDERED_DISCONNECT:
      return "Locally ordered disconnect";

    case DR_PROTOCOL_ERROR:
      return "Protocol error";

    case DR_SWITCH_PEER:
      return "Switch to higher priority peer";

    case DR_BAD_STATE:
      return "Connection in bad state";
  }
  
  return "Unknown disconnect reason";
  
} /* TcpConnection::disconnectReasonStr */


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
TcpConnection::TcpConnection(size_t recv_buf_len)
  : TcpConnection(-1, IpAddress(), 0, recv_buf_len)
{
} /* TcpConnection::TcpConnection */


TcpConnection::TcpConnection(int sock, const IpAddress& remote_addr,
      	      	      	     uint16_t remote_port, size_t recv_buf_len)
  : remote_addr(remote_addr), remote_port(remote_port),
    recv_buf_len(recv_buf_len), sock(sock),
    recv_buf(0), recv_buf_cnt(0)
{
  recv_buf = new char[recv_buf_len];
  rd_watch.activity.connect(mem_fun(*this, &TcpConnection::recvHandler));
  wr_watch.activity.connect(mem_fun(*this, &TcpConnection::writeHandler));
  setSocket(sock);
} /* TcpConnection::TcpConnection */


TcpConnection::~TcpConnection(void)
{
  closeConnection();
  delete [] recv_buf;
  recv_buf = 0;
  recv_buf_cnt = recv_buf_len = 0;
} /* TcpConnection::~TcpConnection */


TcpConnection& TcpConnection::operator=(TcpConnection&& other)
{
  //std::cout << "### TcpConnection::operator=(TcpConnection&&)" << std::endl;

  remote_addr = other.remote_addr;
  other.remote_addr.clear();

  remote_port = other.remote_port;
  other.remote_port = 0;

  sock = other.sock;
  other.sock = -1;

  rd_watch = std::move(other.rd_watch);

  wr_watch = std::move(other.wr_watch);

  delete [] recv_buf;
  recv_buf_len = other.recv_buf_len;
  recv_buf = other.recv_buf;
  recv_buf_cnt = other.recv_buf_cnt;

  other.recv_buf_len = DEFAULT_RECV_BUF_LEN;
  other.recv_buf = new char[other.recv_buf_len];
  other.recv_buf_cnt = 0;

  return *this;
} /* TcpConnection::operator= */


void TcpConnection::setRecvBufLen(size_t recv_buf_len)
{
  if (recv_buf_cnt > recv_buf_len)
  {
      // This will on next reception cause an overflow error disconnection
    recv_buf_cnt = recv_buf_len;
  }
  char *new_recv_buf = new char[recv_buf_len];
  memcpy(new_recv_buf, recv_buf, recv_buf_cnt);
  this->recv_buf_len = recv_buf_len;
  delete [] recv_buf;
  recv_buf = new_recv_buf;
} /* TcpConnection::setRecvBufLen */


int TcpConnection::write(const void *buf, int count)
{
  assert(sock >= 0);
  int cnt = ::send(sock, buf, count, MSG_NOSIGNAL);
  if (cnt < 0)
  {
    if (errno != EAGAIN)
    {
      return -1;
    }
    cnt = 0;
  }

  if (cnt < count)
  {
    sendBufferFull(true);
    wr_watch.setEnabled(true);
  }
  
  return cnt;
  
} /* TcpConnection::write */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    TcpConnection::setSocket
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
void TcpConnection::setSocket(int sock)
{
  this->sock = sock;
  rd_watch.setFd(sock, FdWatch::FD_WATCH_RD);
  rd_watch.setEnabled(sock >= 0);
  wr_watch.setEnabled(false);
  wr_watch.setFd(sock, FdWatch::FD_WATCH_WR);
} /* TcpConnection::setSocket */


/*
 *------------------------------------------------------------------------
 * Method:    TcpConnection::setRemoteAddr
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
void TcpConnection::setRemoteAddr(const IpAddress& remote_addr)
{
  this->remote_addr = remote_addr;
} /* TcpConnection::setRemoteAddr */


/*
 *------------------------------------------------------------------------
 * Method:    TcpConnection::setRemotePort
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    Tobias Blomberg
 * Created:   2003-12-07
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
void TcpConnection::setRemotePort(uint16_t remote_port)
{
  this->remote_port = remote_port;
} /* TcpConnection::setRemotePort */


void TcpConnection::closeConnection(void)
{
  recv_buf_cnt = 0;

  wr_watch.setEnabled(false);
  rd_watch.setEnabled(false);

  if (sock >= 0)
  {
    close(sock);
    sock = -1;
  }
} /* TcpConnection::closeConnection */





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
void TcpConnection::recvHandler(FdWatch *watch)
{
  //cout << "recv_buf_cnt=" << recv_buf_cnt << endl;
  //cout << "recv_buf_len=" << recv_buf_len << endl;
  
  if (recv_buf_cnt == recv_buf_len)
  {
    closeConnection();
    onDisconnected(DR_RECV_BUFFER_OVERFLOW);
    return;
  }
  
  int cnt = read(sock, recv_buf+recv_buf_cnt, recv_buf_len-recv_buf_cnt);
  if (cnt == -1)
  {
    int errno_tmp = errno;
    closeConnection();
    errno = errno_tmp;
    onDisconnected(DR_SYSTEM_ERROR);
    return;
  }
  else if (cnt == 0)
  {
    //cout << "Connection closed by remote host!\n";
    closeConnection();
    onDisconnected(DR_REMOTE_DISCONNECTED);
    return;
  }
  
  recv_buf_cnt += cnt;
  size_t processed = onDataReceived(recv_buf, recv_buf_cnt);
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
  
} /* TcpConnection::recvHandler */


void TcpConnection::writeHandler(FdWatch *watch)
{
  watch->setEnabled(false);
  sendBufferFull(false);
} /* TcpConnection::writeHandler */



/*
 * This file has not been truncated
 */

