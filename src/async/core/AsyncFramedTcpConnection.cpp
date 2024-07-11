/**
@file	 AsyncFramedTcpConnection.cpp
@brief   A TCP connection with framed instead of streamed content
@author  Tobias Blomberg / SM0SVX
@date	 2017-03-30

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

#include <cstring>
#include <cerrno>


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

#include "AsyncFramedTcpConnection.h"



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

FramedTcpConnection::FramedTcpConnection(size_t recv_buf_len)
  : TcpConnection(recv_buf_len), m_max_frame_size(DEFAULT_MAX_FRAME_SIZE),
    m_size_received(false)
{
#if 0
  TcpConnection::sendBufferFull.connect(
      sigc::mem_fun(*this, &FramedTcpConnection::onSendBufferFull));
#endif
} /* FramedTcpConnection::FramedTcpConnection */


FramedTcpConnection::FramedTcpConnection(
    int sock, const IpAddress& remote_addr, uint16_t remote_port,
    size_t recv_buf_len)
  : TcpConnection(sock, remote_addr, remote_port, recv_buf_len),
    m_max_frame_size(DEFAULT_MAX_FRAME_SIZE), m_size_received(false)
{
#if 0
  TcpConnection::sendBufferFull.connect(
      sigc::mem_fun(*this, &FramedTcpConnection::onSendBufferFull));
#endif
} /* FramedTcpConnection::FramedTcpConnection */


FramedTcpConnection::~FramedTcpConnection(void)
{
  for (auto& item : m_txq)
  {
    delete item;
  }
  m_txq.clear();
} /* FramedTcpConnection::~FramedTcpConnection */


TcpConnection& FramedTcpConnection::operator=(TcpConnection&& other_base)
{
  //std::cout << "### FramedTcpConnection::operator=(TcpConnection&&)"
  //          << std::endl;

  this->TcpConnection::operator=(std::move(other_base));
  auto& other = dynamic_cast<FramedTcpConnection&>(other_base);

  m_max_frame_size = other.m_max_frame_size;
  other.m_max_frame_size = DEFAULT_MAX_FRAME_SIZE;

  m_size_received = other.m_size_received;
  other.m_size_received = false;

  m_frame_size = other.m_frame_size;
  other.m_frame_size = 0;

  m_frame.swap(other.m_frame);
  other.m_frame.clear();

  m_txq.swap(other.m_txq);
  other.m_txq.clear();

  return *this;
} /* FramedTcpConnection::operator=(TcpConnection&&) */


int FramedTcpConnection::write(const void *buf, int count)
{
  //cout << "### FramedTcpConnection::write: count=" << count << "\n";
  if (count < 0)
  {
    return 0;
  }
  else if (static_cast<uint32_t>(count) > m_max_frame_size)
  {
    errno = EMSGSIZE;
    return -1;
  }

  QueueItem *qi = new QueueItem(buf, count);
  if (m_txq.empty())
  {
    int ret = TcpConnection::write(qi->m_buf, qi->m_size);
    //cout << "###   count=" << (qi->m_size-qi->m_pos) << " ret=" << ret << endl;
    if (ret < 0)
    {
      delete qi;
      qi = 0;
      return -1;
    }
    if (ret >= qi->m_size)
    {
      delete qi;
      qi = 0;
    }
    else
    {
      //cout << "### Not all bytes were sent: count=" << count
      //     << " ret=" << ret << endl;
      qi->m_pos += ret;
    }
  }

  if (qi != 0)
  {
    m_txq.push_back(qi);
  }

  return count;
} /* FramedTcpConnection::write */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void FramedTcpConnection::closeConnection(void)
{
  disconnectCleanup();
  TcpConnection::closeConnection();
} /* FramedTcpConnection::closeConnection */


void FramedTcpConnection::onDisconnected(DisconnectReason reason)
{
  disconnectCleanup();
  TcpConnection::onDisconnected(reason);
} /* FramedTcpConnection::onDisconnected */


int FramedTcpConnection::onDataReceived(void *buf, int count)
{
  int orig_count = count;
  uint8_t* ptr = reinterpret_cast<uint8_t*>(buf);

  while (count > 0)
  {
    if (!m_size_received)
    {
      if (static_cast<size_t>(count) < sizeof(m_frame_size))
      {
        break;
      }
      m_frame_size = static_cast<uint32_t>(*ptr++) << 24;
      m_frame_size |= static_cast<uint32_t>(*ptr++) << 16;
      m_frame_size |= static_cast<uint32_t>(*ptr++) << 8;
      m_frame_size |= static_cast<uint32_t>(*ptr++);
      if (m_frame_size > m_max_frame_size)
      {
        closeConnection();
        onDisconnected(DR_PROTOCOL_ERROR);
        return orig_count - count;
      }
      m_frame.clear();
      m_frame.reserve(m_frame_size);
      count -= sizeof(m_frame_size);
      m_size_received = true;
    }
    else
    {
      size_t cur_size = m_frame.size();
      size_t copy_cnt = min(m_frame_size - cur_size, static_cast<size_t>(count));
      m_frame.resize(cur_size + copy_cnt);
      ::memcpy(m_frame.data()+cur_size, ptr, copy_cnt);
      count -= copy_cnt;
      ptr += copy_cnt;
      if (m_frame.size() == m_frame_size)
      {
        m_size_received = false;
        frameReceived(this, m_frame);
      }
    }
  }

  return orig_count - count;
} /* FramedTcpConnection::onDataReceived */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

#if 0
void FramedTcpConnection::onSendBufferFull(bool is_full)
{
  //cout << "### FramedTcpConnection::onSendBufferFull: is_full="
  //     << is_full << "\n";
  if (!is_full)
  {
    while (!m_txq.empty())
    {
      QueueItem *qi = m_txq.front();
      int ret = TcpConnection::write(qi->m_buf+qi->m_pos, qi->m_size-qi->m_pos);
      //cout << "###   count=" << (qi->m_size-qi->m_pos) << " ret=" << ret << endl;
      if (ret <= 0)
      {
        return;
      }
      qi->m_pos += ret;
      if (qi->m_pos < qi->m_size)
      {
        break;
      }
      m_txq.pop_front();
      delete qi;
      qi = 0;
    }
  }
} /* FramedTcpConnection::onSendBufferFull */
#endif


void FramedTcpConnection::disconnectCleanup(void)
{
  for (TxQueue::iterator it = m_txq.begin(); it != m_txq.end(); ++it)
  {
    delete *it;
  }
  m_txq.clear();
} /* FramedTcpConnection::disconnectCleanup */


/*
 * This file has not been truncated
 */

