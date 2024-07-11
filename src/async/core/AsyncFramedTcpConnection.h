/**
@file	 AsyncFramedTcpConnection.h
@brief   A TCP connection with framed instead of streamed content
@author  Tobias Blomberg / SM0SVX
@date	 2017-03-30

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

/** @example AsyncFramedTcpClient_demo.cpp
An example of how to use the FramedTcpConnection class in a client
*/

/** @example AsyncFramedTcpServer_demo.cpp
An example of how to use the FramedTcpConnection class in a server
*/


#ifndef ASYNC_FRAMED_TCP_CONNECTION_INCLUDED
#define ASYNC_FRAMED_TCP_CONNECTION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <vector>
#include <deque>
#include <cstring>


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
@brief	A TCP connection with framed instead of streamed content
@author Tobias Blomberg / SM0SVX
@date   2017-03-30

This class implements a framed TCP connection. It will make sure that the data
given to one call to the write function will arrive at the other end in one
piece or not at all. This makes it easier to implement message based protocols
that only want to see completely transfered messages at the other end.

\include AsyncFramedTcpClient_demo.cpp

\include AsyncFramedTcpServer_demo.cpp
*/
class FramedTcpConnection : public TcpConnection
{
  public:
    /**
     * @brief 	Constructor
     * @param 	recv_buf_len  The length of the receiver buffer to use
     */
    explicit FramedTcpConnection(size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);

    /**
     * @brief 	Constructor
     * @param 	sock  	      The socket for the connection to handle
     * @param 	remote_addr   The remote IP-address of the connection
     * @param 	remote_port   The remote TCP-port of the connection
     * @param 	recv_buf_len  The length of the receiver buffer to use
     */
    FramedTcpConnection(int sock, const IpAddress& remote_addr,
                        uint16_t remote_port,
                        size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);

    /**
     * @brief 	Destructor
     */
    virtual ~FramedTcpConnection(void);

    /**
     * @brief   Move assignmnt operator
     * @param   other_base The object to move from
     * @return  Returns this object
     *
     * The move operator move the state of a specified TcpFramedConnection
     * object into this object. After the move, the state of the other object
     * will be the same as if it had just been default constructed.
     */
    virtual TcpConnection& operator=(TcpConnection&& other_base) override;

    /**
     * @brief   Set the maximum frame size
     * @param   frame_size The maximum frame size in bytes
     *
     * Use this function to set the maximum allowed frame size. If a frame size
     * number larger than this is received a disconnection is immediately
     * issued. The default maximum frame size is DEFAULT_MAX_FRAME_SIZE.
     */
    void setMaxFrameSize(uint32_t frame_size) { m_max_frame_size = frame_size; }

    /**
     * @brief 	Send a frame on the TCP connection
     * @param 	buf The buffer containing the frame to send
     * @param 	count The number of bytes in the frame
     * @return	Return bytes written or -1 on failure
     *
     * This function will send a frame of data on the TCP connection. The
     * frame will either be completely transmitted or discarded on error.
     * There is no inbetween so this function will always return either the
     * given count or -1 on error.
     */
    virtual int write(const void *buf, int count) override;

    /**
     * @brief 	A signal that is emitted when a connection has been terminated
     * @param 	con   	The connection object
     * @param 	reason  The reason for the disconnect
     */
    sigc::signal<void, FramedTcpConnection *, DisconnectReason> disconnected;

    /**
     * @brief 	A signal that is emitted when a frame has been received on the
     *	      	connection
     * @param 	buf   A buffer containg the read data
     * @param 	count The number of bytes in the buffer
     *
     * This signal is emitted when a frame has been received on this connection.
     */
    sigc::signal<void, FramedTcpConnection *,
                 std::vector<uint8_t>&> frameReceived;

  protected:
    sigc::signal<int, TcpConnection*, void*, int> dataReceived;
    sigc::signal<void, bool> sendBufferFull;

    FramedTcpConnection& operator=(const FramedTcpConnection&) = delete;

    /**
     * @brief   Disconnect from the remote peer
     *
     * This function is used internally to close the connection to the remote
     * peer.
     */
    virtual void closeConnection(void) override;

    /**
     * @brief 	Called when a connection has been terminated
     * @param 	reason  The reason for the disconnect
     *
     * This function will be called when the connection has been terminated.
     * The default action for this function is to emit the disconnected signal.
     */
    virtual void onDisconnected(DisconnectReason reason) override;

    /**
     * @brief 	Called when data has been received on the connection
     * @param 	buf   A buffer containg the read data
     * @param 	count The number of bytes in the buffer
     * @return	Return the number of processed bytes
     *
     * This function is called when data has been received on this connection.
     * The buffer will contain the bytes read from the operating system.
     * The function will return the number of bytes that has been processed. The
     * bytes not processed will be stored in the receive buffer for this class
     * and presented again to the slot when more data arrives. The new data
     * will be appended to the old data.
     * The default action for this function is to emit the dataReceived signal.
     */
    virtual int onDataReceived(void *buf, int count) override;

    /**
     * @brief   Emit the disconnected signal
     * @param   reason The reason for the disconnection
     */
    virtual void emitDisconnected(DisconnectReason reason) override
    {
      disconnected(this, reason);
      TcpConnection::emitDisconnected(reason);
    }

  private:
    static const uint32_t DEFAULT_MAX_FRAME_SIZE = 1024 * 1024; // 1MB

    struct QueueItem
    {
      char* m_buf;
      int   m_size;
      int   m_pos;

      QueueItem(const void* buf, int count)
        : m_buf(0), m_size(4+count), m_pos(0)
      {
        m_buf = new char[4+count];
        char *ptr = m_buf;
        *ptr++ = static_cast<uint32_t>(count) >> 24;
        *ptr++ = (static_cast<uint32_t>(count) >> 16) & 0xff;
        *ptr++ = (static_cast<uint32_t>(count) >> 8) & 0xff;
        *ptr++ = (static_cast<uint32_t>(count)) & 0xff;
        std::memcpy(ptr, buf, count);
      }
      ~QueueItem(void) { delete [] m_buf; }
    };
    typedef std::deque<QueueItem*> TxQueue;

    uint32_t              m_max_frame_size;
    bool                  m_size_received;
    uint32_t              m_frame_size;
    std::vector<uint8_t>  m_frame;
    TxQueue               m_txq;

    FramedTcpConnection(const FramedTcpConnection&) = delete;
    //void onSendBufferFull(bool is_full);
    void disconnectCleanup(void);

};  /* class FramedTcpConnection */


} /* namespace */

#endif /* ASYNC_FRAMED_TCP_CONNECTION_INCLUDED */



/*
 * This file has not been truncated
 */
