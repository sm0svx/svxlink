/**
@file	 AsyncTcpClient.h
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

/** @example AsyncTcpClient_demo.cpp
An example of how to use the Async::TcpClient class
*/



#ifndef ASYNC_TCP_CLIENT_INCLUDED
#define ASYNC_TCP_CLIENT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>

#include <string>


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

class FdWatch;
class DnsLookup;
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
@brief	A class for creating a TCP client connection
@author Tobias Blomberg
@date   2003-04-12

This class is used to create a TCP client connection. All details of how to
create the connection is hidden inside the class. This make it very easy to
create and use the connections. An example usage is shown below.

\include AsyncTcpClient_demo.cpp
*/
class TcpClient : public SigC::Object
{
  public:
    /**
     * @brief Reason code for disconnects
     */
    typedef enum
    { 
      DR_HOST_NOT_FOUND,      ///< The specified host was not found in the DNS
      DR_REMOTE_DISCONNECTED, ///< The remote host disconnected
      DR_SYSTEM_ERROR,	      ///< A system error occured (check errno)
      DR_RECV_BUFFER_OVERFLOW ///< Receiver buffer overflow
    } DisconnectReason;
    
    /**
     * @brief The default length of the reception buffer
     */
    static const int DEFAULT_RECV_BUF_LEN = 1024;
      
    /**
     * @brief 	Constructor
     * @param 	remote_host   The hostname of the remote host
     * @param 	remote_port   The port on the remote host to connect to
     * @param 	recv_buf_len  The length of the receiver buffer to use
     *
     * The object will be constructed and variables will be initialized but
     * no connection will be created until the connect function
     * (see @ref TcpClient::connect) is called.
     */
    TcpClient(const std::string& remote_host, short remote_port,
      	      size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);
    
    /**
     * @brief 	Destructor
     */
    ~TcpClient(void);
    
    /**
     * @brief 	Connect to the remote host
     *
     * This function will initiate a connection to the remote host. The
     * connection must not be written to before the connected signal
     * (see @ref TcpClient::connected) has been emitted. If the connection is
     * already established or pending, nothing will be done.
     */
    void connect(void);
    
    /**
     * @brief 	Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    void disconnect(void);
    
    /**
     * @brief 	Write data to the TCP connection
     * @param 	buf   The buffer containing the data to send
     * @param 	count The number of bytes to send from the buffer
     * @return	Returns the number of bytes written or -1 on failure
     */
    int write(const void *buf, int count);
    
    /**
     * @brief 	Return the name of the remote host
     * @return	Returns the name of the remote host
     *
     * This function returns the name of the remote host as given to the
     * constructor. If an IP-address was given to the constructor, this
     * function will also return an IP-address.
     */
    const std::string& remoteHost(void) const { return remote_host; }
    
    /**
     * @brief 	Return the remote port used
     * @return	Returns the remote port
     */
    short remotePort(void) const { return remote_port; }
    
    /**
     * @brief 	A signal that is emitted when a connection has been established
     */
    SigC::Signal0<void>       	      	  connected;
    
    /**
     * @brief 	A signal that is emitted when a connection has been terminated
     * @param 	reason The reason for the disconnect
     */
    SigC::Signal1<void, DisconnectReason> disconnected;
    
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
    SigC::Signal2<int, void *, int> dataReceived;
    
    /**
     * @brief 	A signal that is emitted when the send buffer status changes
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if a buffer full condition has been cleared
     */
    SigC::Signal1<void, bool> sendBufferFull;

        
  protected:
    
  private:
    std::string     remote_host;
    short           remote_port;
    size_t          recv_buf_len;
    DnsLookup *     dns;
    int       	    sock;
    FdWatch *       rd_watch;
    FdWatch *       wr_watch;
    char *    	    recv_buf;
    size_t          recv_buf_cnt;
    
    void dnsResultsReady(DnsLookup& dns_lookup);
    void connectToRemote(const IpAddress& ip_addr);
    void recvHandler(FdWatch *watch);
    void connectHandler(FdWatch *watch);
    void writeHandler(FdWatch *watch);

};  /* class TcpClient */


} /* namespace */

#endif /* ASYNC_TCP_CLIENT_INCLUDED */



/*
 * This file has not been truncated
 */

