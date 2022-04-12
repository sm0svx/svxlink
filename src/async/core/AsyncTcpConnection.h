/**
@file	 AsyncTcpConnection.h
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



#ifndef ASYNC_TCP_CONNECTION_INCLUDED
#define ASYNC_TCP_CONNECTION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <stdint.h>

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncFdWatch.h>


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

class IpAddress;


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
@brief	A class for handling exiting TCP connections
@author Tobias Blomberg
@date   2003-12-07

This class is used to handle an existing TCP connection. It is not meant to
be used directly but could be. It it mainly created to handle connections
for Async::TcpClient and Async::TcpServer.
*/
class TcpConnection : virtual public sigc::trackable
{
  public:
    /**
     * @brief Reason code for disconnects
     */
    typedef enum
    { 
      DR_HOST_NOT_FOUND,       ///< The specified host was not found in the DNS
      DR_REMOTE_DISCONNECTED,  ///< The remote host disconnected
      DR_SYSTEM_ERROR,	       ///< A system error occured (check errno)
      DR_RECV_BUFFER_OVERFLOW, ///< Receiver buffer overflow
      DR_ORDERED_DISCONNECT,   ///< Disconnect ordered locally
      DR_PROTOCOL_ERROR,       ///< Protocol error
      DR_SWITCH_PEER,          ///< A better peer was found so reconnecting
      DR_BAD_STATE             ///< The connection ended up in a bad state
    } DisconnectReason;
    
    /**
     * @brief The default length of the reception buffer
     */
    static const int DEFAULT_RECV_BUF_LEN = 1024;
    
    /**
     * @brief Translate disconnect reason to a string
     */
    static const char *disconnectReasonStr(DisconnectReason reason);
    
    /**
     * @brief 	Constructor
     * @param 	recv_buf_len  The length of the receiver buffer to use
     */
    explicit TcpConnection(size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);
    
    /**
     * @brief 	Constructor
     * @param 	sock  	      The socket for the connection to handle
     * @param 	remote_addr   The remote IP-address of the connection
     * @param 	remote_port   The remote TCP-port of the connection
     * @param 	recv_buf_len  The length of the receiver buffer to use
     */
    TcpConnection(int sock, const IpAddress& remote_addr,
      	      	  uint16_t remote_port,
      	      	  size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);
    
    /**
     * @brief 	Destructor
     */
    virtual ~TcpConnection(void);

    /**
     * @brief   Move assignmnt operator
     * @param   other The object to move from
     * @return  Returns this object
     *
     * The move operator move the state of a specified TcpConnection object
     * into this object. After the move, the state of the other object will be
     * the same as if it had just been default constructed.
     */
    virtual TcpConnection& operator=(TcpConnection&& other);

    /**
     * @brief   Set a new receive buffer size
     * @param   recv_buf_len The new receive buffer size in bytes
     *
     * This function will resize the receive buffer to the specified size.
     * If the buffer size is reduced and there are more bytes in the current
     * buffer than can be fitted into the new buffer, an overflow disconnection
     * will be issued on the next reception.
     */
    void setRecvBufLen(size_t recv_buf_len);

    /**
     * @brief 	Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    virtual void disconnect(void) { closeConnection(); }
    
    /**
     * @brief 	Write data to the TCP connection
     * @param 	buf   The buffer containing the data to send
     * @param 	count The number of bytes to send from the buffer
     * @return	Returns the number of bytes written or -1 on failure
     */
    virtual int write(const void *buf, int count);
    
    /**
     * @brief 	Return the IP-address of the remote host
     * @return	Returns the IP-address of the remote host
     *
     * This function returns the IP-address of the remote host.
     */
    const IpAddress& remoteHost(void) const { return remote_addr; }
    
    /**
     * @brief 	Return the remote port used
     * @return	Returns the remote port
     */
    uint16_t remotePort(void) const { return remote_port; }
    
    /**
     * @brief 	Check if the connection is established or not
     * @return	Returns \em true if the connection is established or
     *	      	\em false if the connection is not established
     */
    bool isConnected(void) const { return sock != -1; }

    /**
     * @brief   Check if the connection is idle
     * @return  Returns \em true if the connection is idle
     *
     * A connection being idle means that it is not connected
     */
    bool isIdle(void) const { return sock == -1; }
    
    /**
     * @brief 	A signal that is emitted when a connection has been terminated
     * @param 	con   	The connection object
     * @param 	reason  The reason for the disconnect
     */
    sigc::signal<void, TcpConnection *, DisconnectReason> disconnected;
    
    /**
     * @brief 	A signal that is emitted when data has been received on the
     *	      	connection
     * @param 	buf   A buffer containg the read data
     * @param 	count The number of bytes in the buffer
     * @return	Return the number of processed bytes
     *
     * This signal is emitted when data has been received on this connection.
     * The buffer will contain the bytes read from the operating system.
     * The slot must return the number of bytes that has been processed. The
     * bytes not processed will be stored in the receive buffer for this class
     * and presented again to the slot when more data arrives. The new data
     * will be appended to the old data.
     */
    sigc::signal<int, TcpConnection *, void *, int> dataReceived;
    
    /**
     * @brief 	A signal that is emitted when the send buffer status changes
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if a buffer full condition has been cleared
     */
    sigc::signal<void, bool> sendBufferFull;

        
  protected:
    /**
     * @brief 	Setup information about the connection
     * @param 	sock  	      The socket for the connection to handle
     *
     * Use this function to set up the socket for the connection.
     */
    void setSocket(int sock);
    
    /**
     * @brief 	Setup information about the connection
     * @param 	remote_addr   The remote IP-address of the connection
     *
     * Use this function to set up the remote IP-address for the connection.
     */
    void setRemoteAddr(const IpAddress& remote_addr);
    
    /**
     * @brief 	Setup information about the connection
     * @param 	remote_port   The remote TCP-port of the connection
     *
     * Use this function to set up the remote port for the connection.
     */
    void setRemotePort(uint16_t remote_port);
    
    /**
     * @brief 	Return the socket file descriptor
     * @return	Returns the currently used socket file descriptor
     *
     * Use this function to get the socket file descriptor that is currently
     * in use. If it is -1 it has not been set.
     */
    int socket(void) const { return sock; }

    /**
     * @brief   Disconnect from the remote peer
     *
     * This function is used internally to close the connection to the remote
     * peer.
     */
    virtual void closeConnection(void);

    /**
     * @brief 	Called when a connection has been terminated
     * @param 	reason  The reason for the disconnect
     *
     * This function will be called when the connection has been terminated.
     * The default action for this function is to emit the disconnected signal.
     */
    virtual void onDisconnected(DisconnectReason reason)
    {
      emitDisconnected(reason);
    }

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
    virtual int onDataReceived(void *buf, int count)
    {
      return dataReceived(this, buf, count);
    }

    /**
     * @brief   Emit the disconnected signal
     * @param   reason The reason for the disconnection
     */
    virtual void emitDisconnected(DisconnectReason reason)
    {
      disconnected(this, reason);
    }

  private:
    friend class TcpClientBase;

    IpAddress remote_addr;
    uint16_t  remote_port;
    size_t    recv_buf_len;
    int       sock;
    FdWatch   rd_watch;
    FdWatch   wr_watch;
    char *    recv_buf;
    size_t    recv_buf_cnt;
    
    void recvHandler(FdWatch *watch);
    void writeHandler(FdWatch *watch);

};  /* class TcpConnection */


} /* namespace */

#endif /* ASYNC_TCP_CONNECTION_INCLUDED */



/*
 * This file has not been truncated
 */

