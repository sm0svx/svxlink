/**
@file	 AsyncHttpServerConnection.h
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

/** @example AsyncHttpServer_demo.cpp
An example of how to use the HttpServerConnection class in a client
*/

#ifndef ASYNC_HTTP_SERVER_CONNECTION_INCLUDED
#define ASYNC_HTTP_SERVER_CONNECTION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <vector>
#include <deque>
#include <cstring>
#include <map>
#include <string>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpConnection.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  A class representing a HTTP server side connection
@author Tobias Blomberg / SM0SVX
@date   2019-08-26

This class implement a VERY simple HTTP server side connection. It can be used
together with the Async::TcpServer class to build a HTTP server.

WARNING: This implementation is not suitable to be exposed to the public
Internet. It contains a number of security flaws and probably also
incompatibilities. Only use this class with known clients.

\include AsyncHttpServer_demo.cpp
*/
class HttpServerConnection : public TcpConnection
{
  public:
    typedef std::map<std::string, std::string> Headers;
    struct Request
    {
      std::string method;
      std::string target;
      unsigned ver_major;
      unsigned ver_minor;
      Headers headers;

      Request(void)
      {
        clear();
      }

      void clear(void)
      {
        method.clear();
        target.clear();
        headers.clear();
        ver_major = 0;
        ver_minor = 0;
      }
    };

    class Response
    {
      public:
        Response(void) : m_code(0), m_send_content(true) {}

        unsigned code(void) const { return m_code; }
        void setCode(unsigned code) { m_code = code; }

        const Headers& headers(void) const { return m_headers; }
        template <typename T>
        void setHeader(const std::string& key, T value)
        {
          std::ostringstream os;
          os << value;
          m_headers[key] = os.str();
        }

        const std::string& content(void) const { return m_content; }
        void setContent(const std::string& content_type,
                        const std::string& content)
        {
          m_content = content;
          setHeader("Content-type", content_type);
          setHeader("Content-length", m_content.size());
        }
        bool sendContent(void) const { return m_send_content; }
        void setSendContent(bool send_content)
        {
          m_send_content = send_content;
        }

        void clear(void)
        {
          m_code = 0;
          m_headers.clear();
          m_content.clear();
        }

      private:
        unsigned    m_code;
        Headers     m_headers;
        std::string m_content;
        bool        m_send_content;
    };

    /**
     * @brief   Constructor
     * @param   recv_buf_len  The length of the receiver buffer to use
     */
    explicit HttpServerConnection(size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);

    /**
     * @brief   Constructor
     * @param   sock          The socket for the connection to handle
     * @param   remote_addr   The remote IP-address of the connection
     * @param   remote_port   The remote TCP-port of the connection
     * @param   recv_buf_len  The length of the receiver buffer to use
     */
    HttpServerConnection(int sock, const IpAddress& remote_addr,
                        uint16_t remote_port,
                        size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);

    /**
     * @brief   Destructor
     */
    virtual ~HttpServerConnection(void);

    /**
     * @brief   Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    virtual void disconnect(void);

    /**
     * @brief   Send a HTTP response
     * @param   res The response (@see Response)
     * @return  Return \em true on success or else \em false
     */
    virtual bool write(const Response& res);

    /**
     * @brief   A signal that is emitted when a connection has been terminated
     * @param   con     The connection object
     * @param   reason  The reason for the disconnect
     */
    sigc::signal<void, HttpServerConnection *, DisconnectReason> disconnected;

    /**
     * @brief   A signal that is emitted when a HTTP request has been received
     *          on the connection
     * @param 	req The request object (@see Request)
     *
     * This signal is emitted when a HTTP request has been received on this
     * connection.
     */
    sigc::signal<void, HttpServerConnection *, Request&> requestReceived;

  protected:
    sigc::signal<void, bool> sendBufferFull;

    /**
     * @brief   Called when a connection has been terminated
     * @param   reason  The reason for the disconnect
     *
     * This function will be called when the connection has been terminated.
     */
    virtual void onDisconnected(DisconnectReason reason);

    /**
     * @brief   Called when data has been received on the connection
     * @param   buf   A buffer containg the read data
     * @param   count The number of bytes in the buffer
     * @return  Return the number of processed bytes
     *
     * This function is called when data has been received on this connection.
     * The buffer will contain the bytes read from the operating system.
     * The function will return the number of bytes that has been processed. The
     * bytes not processed will be stored in the receive buffer for this class
     * and presented again to the slot when more data arrives. The new data
     * will be appended to the old data.
     */
    virtual int onDataReceived(void *buf, int count);

  private:
    enum State {
      STATE_DISCONNECTED, STATE_EXPECT_START_LINE, STATE_EXPECT_HEADER,
      STATE_EXPECT_PAYLOAD, STATE_REQ_COMPLETE
    };
    //struct QueueItem
    //{
    //  char* m_buf;
    //  int   m_size;
    //  int   m_pos;

    //  QueueItem(const void* buf, int count)
    //    : m_buf(0), m_size(4+count), m_pos(0)
    //  {
    //    m_buf = new char[4+count];
    //    char *ptr = m_buf;
    //    *ptr++ = static_cast<uint32_t>(count) >> 24;
    //    *ptr++ = (static_cast<uint32_t>(count) >> 16) & 0xff;
    //    *ptr++ = (static_cast<uint32_t>(count) >> 8) & 0xff;
    //    *ptr++ = (static_cast<uint32_t>(count)) & 0xff;
    //    std::memcpy(ptr, buf, count);
    //  }
    //  ~QueueItem(void) { delete [] m_buf; }
    //};
    //typedef std::deque<QueueItem*> TxQueue;

    //TxQueue               m_txq;
    State                   m_state;
    std::string             m_row;
    Request                 m_req;

    HttpServerConnection(const HttpServerConnection&);
    HttpServerConnection& operator=(const HttpServerConnection&);
    void handleStartLine(void);
    void handleHeader(void);
    void onSendBufferFull(bool is_full);
    void disconnectCleanup(void);
    const char* codeToString(unsigned code);

};  /* class HttpServerConnection */


} /* namespace */

#endif /* ASYNC_HTTP_SERVER_CONNECTION_INCLUDED */



/*
 * This file has not been truncated
 */
