/**
@file	 AsyncHttpServerConnection.cpp
@brief   A simple HTTP Server connection class
@author  Tobias Blomberg / SM0SVX
@date	 2019-08-26

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
#include <sstream>


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

#include "AsyncHttpServerConnection.h"



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

HttpServerConnection::HttpServerConnection(size_t recv_buf_len)
  : TcpConnection(recv_buf_len), m_state(STATE_DISCONNECTED)
{
  TcpConnection::sendBufferFull.connect(
      sigc::mem_fun(*this, &HttpServerConnection::onSendBufferFull));
} /* HttpServerConnection::HttpServerConnection */


HttpServerConnection::HttpServerConnection(
    int sock, const IpAddress& remote_addr, uint16_t remote_port,
    size_t recv_buf_len)
  : TcpConnection(sock, remote_addr, remote_port, recv_buf_len),
    m_state(STATE_EXPECT_START_LINE)
{
} /* HttpServerConnection::HttpServerConnection */


HttpServerConnection::~HttpServerConnection(void)
{
} /* HttpServerConnection::~HttpServerConnection */


void HttpServerConnection::disconnect(void)
{
  //cout << "### HttpServerConnection::disconnect\n";
  disconnectCleanup();
  TcpConnection::disconnect();
} /* HttpServerConnection::disconnect */


//int HttpServerConnection::write(const void *buf, int count)
//{
//  cout << "### HttpServerConnection::write: count=" << count << "\n";
//  if (count < 0)
//  {
//    return 0;
//  }
//
//  //QueueItem *qi = new QueueItem(buf, count);
//  //if (m_txq.empty())
//  //{
//  //  int ret = TcpConnection::write(qi->m_buf, qi->m_size);
//  //  //cout << "###   count=" << (qi->m_size-qi->m_pos) << " ret=" << ret << endl;
//  //  if (ret < 0)
//  //  {
//  //    delete qi;
//  //    qi = 0;
//  //    return -1;
//  //  }
//  //  if (ret >= qi->m_size)
//  //  {
//  //    delete qi;
//  //    qi = 0;
//  //  }
//  //  else
//  //  {
//  //    //cout << "### Not all bytes were sent: count=" << count
//  //    //     << " ret=" << ret << endl;
//  //    qi->m_pos += ret;
//  //  }
//  //}
//
//  //if (qi != 0)
//  //{
//  //  m_txq.push_back(qi);
//  //}
//
//  return count;
//} /* HttpServerConnection::write */


bool HttpServerConnection::write(const Response& res)
{
  std::ostringstream os;
  os << "HTTP/1.1 " << res.code() << " " << codeToString(res.code()) << "\r\n";
  for (std::map<std::string, std::string>::const_iterator it=res.headers().begin();
       it!=res.headers().end(); ++it)
  {
    os << (*it).first << ": " << (*it).second << "\r\n";
  }
  os << "\r\n";
  if (res.sendContent())
  {
    os << res.content();
  }
  //std::cout << "### HttpServerConnection::write:" << std::endl;
  //std::cout << os.str() << std::endl;
  int len = TcpConnection::write(os.str().c_str(), os.str().size());
  return len == static_cast<int>(os.str().size());
} /* HttpServerConnection::write */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void HttpServerConnection::onDisconnected(DisconnectReason reason)
{
  //cout << "### HttpServerConnection::onDisconnected: "
  //     << TcpConnection::disconnectReasonStr(reason) << "\n";
  disconnectCleanup();
  disconnected(this, reason);
} /* HttpServerConnection::onDisconnected */


int HttpServerConnection::onDataReceived(void *buf, int count)
{
  std::string data(reinterpret_cast<char*>(buf), count);
  size_t data_pos = 0;

  //std::cout << "### HttpServerConnection::onDataReceived: "
  //         << data << std::endl;

  while (data.size() > data_pos)
  {
    if (m_state == STATE_EXPECT_START_LINE)
    {
      size_t eol = data.find("\r\n", data_pos);
      size_t len = eol;
      if (len != std::string::npos)
      {
        len -= data_pos;
      }
      m_row.append(data.substr(data_pos, len));
      data_pos = eol;
      if (eol != std::string::npos)
      {
        data_pos += 2;
        handleStartLine();
        m_row.clear();
      }
    }
    else if (m_state == STATE_EXPECT_HEADER)
    {
      size_t eol = data.find("\r\n", data_pos);
      size_t len = eol;
      if (len != std::string::npos)
      {
        len -= data_pos;
      }
      m_row.append(data.substr(data_pos, len));
      data_pos = eol;
      if (eol != std::string::npos)
      {
        data_pos += 2;
        handleHeader();
        m_row.clear();
      }
    }
    else
    {
      data_pos = std::string::npos;
      //m_req.content.append(data);
    }
  }

  if (m_state == STATE_REQ_COMPLETE)
  {
    requestReceived(this, m_req);
    m_req.clear();
    m_state = STATE_EXPECT_START_LINE;
  }

  return count;
} /* HttpServerConnection::onDataReceived */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void HttpServerConnection::handleStartLine(void)
{
  std::istringstream is(m_row);
  std::string protocol;
  if (!(is >> m_req.method >> m_req.target >> protocol >> std::ws))
  {
    std::cerr << "*** ERROR: Could not parse HTTP header" << std::endl;
    disconnect();
    return;
  }

  if (protocol.substr(0, 5) != "HTTP/")
  {
    std::cerr << "*** ERROR: Illegal protocol specification string \""
              << protocol << "\"" << std::endl;
    disconnect();
    return;
  }

  is.clear();
  is.str(protocol.substr(5));
  char dot;
  if (!(is >> m_req.ver_major >> dot >> m_req.ver_minor >> std::ws) ||
      (dot != '.'))
  {
    std::cerr << "*** ERROR: Illegal protocol version specification \""
              << protocol << "\"" << std::endl;
    disconnect();
    return;
  }

  //std::cout << "### HttpServerConnection::handleStartLine: method="
  //          << m_req.method << " target=" << m_req.target
  //          << " version=" << m_req.ver_major << "."
  //          << m_req.ver_minor
  //          << std::endl;
  m_state = STATE_EXPECT_HEADER;
} /* HttpServerConnection::handleStartLine */


void HttpServerConnection::handleHeader(void)
{
  //std::cout << "### HttpServerConnection::handleHeader: m_row="
  //          << m_row << std::endl;

  if (m_row.empty())
  {
    m_state = STATE_REQ_COMPLETE;
    return;
  }

  size_t colon = m_row.find(":");
  if (colon == std::string::npos)
  {
    std::cerr << "*** ERROR: Malformed HTTP header received" << std::endl;
    disconnect();
    return;
  }
  std::string key(m_row.substr(0, colon));
  size_t value_begin = m_row.find_first_not_of(" \t", colon+1);
  size_t value_end = m_row.find_last_not_of(" \t");
  if (value_begin == std::string::npos)
  {
    std::cerr << "*** ERROR: Malformed HTTP header value received" << std::endl;
    disconnect();
    return;
  }
  std::string value(m_row.substr(value_begin, value_end-value_begin+1));

  //std::cout << "### HttpServerConnection::handleStartLine: key="
  //          << key << " value=" << value << std::endl;

  m_req.headers[key] = value;
} /* HttpServerConnection::handleHeader */


void HttpServerConnection::onSendBufferFull(bool is_full)
{
  cout << "### HttpServerConnection::onSendBufferFull: is_full="
       << is_full << "\n";
  //if (!is_full)
  //{
  //  while (!m_txq.empty())
  //  {
  //    QueueItem *qi = m_txq.front();
  //    int ret = TcpConnection::write(qi->m_buf+qi->m_pos, qi->m_size-qi->m_pos);
  //    //cout << "###   count=" << (qi->m_size-qi->m_pos) << " ret=" << ret << endl;
  //    if (ret <= 0)
  //    {
  //      return;
  //    }
  //    qi->m_pos += ret;
  //    if (qi->m_pos < qi->m_size)
  //    {
  //      break;
  //    }
  //    m_txq.pop_front();
  //    delete qi;
  //    qi = 0;
  //  }
  //}
} /* HttpServerConnection::onSendBufferFull */


void HttpServerConnection::disconnectCleanup(void)
{
  //std::cout << "### HttpServerConnection::disconnectCleanup" << std::endl;
  m_state = STATE_DISCONNECTED;
  //for (TxQueue::iterator it = m_txq.begin(); it != m_txq.end(); ++it)
  //{
  //  delete *it;
  //}
  //m_txq.clear();
} /* HttpServerConnection::disconnectCleanup */


const char* HttpServerConnection::codeToString(unsigned code)
{
  switch (code)
  {
    case 200:
      return "OK";
    case 404:
      return "Not Found";
    case 501:
      return "Not Implemented";
    default:
      return "?";
  }
} /* HttpServerConnection::codeToString */


/*
 * This file has not been truncated
 */

