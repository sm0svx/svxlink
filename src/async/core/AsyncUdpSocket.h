/**
@file    AsyncUdpSocket.h
@brief   Contains a class for using UDP sockets
@author  Tobias Blomberg
@date    2003-04-26

This file contains a class for communication over a UDP sockets.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2023 Tobias Blomberg

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

/** @example AsyncUdpSocket_demo.cpp
An example of how to use the Async::UdpSocket class
*/


#ifndef ASYNC_UDP_SOCKET_INCLUDED
#define ASYNC_UDP_SOCKET_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <stdint.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>


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

class UdpPacket;


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
@brief	A class for working with UDP sockets
@author Tobias Blomberg
@date   2003-04-26

This class is used to work with UDP sockets. An example usage is shown below.

\include AsyncUdpSocket_demo.cpp
*/
class UdpSocket : public sigc::trackable
{
  public:
    /**
     * @brief 	Constructor
     * @param 	local_port  The local port to use. If not specified, a random
     *	      	      	    local port will be used.
     * @param  	bind_ip     Bind to the interface with the given IP address.
     *	      	            If left empty, bind to all interfaces.
     */
    UdpSocket(uint16_t local_port=0, const IpAddress &bind_ip=IpAddress());
  
    /**
     * @brief 	Destructor
     */
    virtual ~UdpSocket(void);

    /**
     * @brief 	Check if the initialization was ok
     * @return	Returns \em true if everything went fine during initialization
     *	      	or \em false if something went wrong
     *
     * This function should always be called after constructing the object to
     * see if everything went fine.
     */
    virtual bool initOk(void) const { return (sock != -1); }

    /**
     * @brief   Get the local IP address associated with this connection
     * @return  Returns an IP address
     */
    Async::IpAddress localAddr(void) const;

    /**
     * @brief   Get the local UDP port associated with this connection
     * @return  Returns a port number
     */
    uint16_t localPort(void) const;

    /**
     * @brief 	Write data to the remote host
     * @param 	remote_ip   The IP-address of the remote host
     * @param 	remote_port The remote port to use
     * @param 	buf   	    A buffer containing the data to send
     * @param 	count       The number of bytes to write
     * @return	Return \em true on success or \em false on failure
     */
    virtual bool write(const IpAddress& remote_ip, int remote_port,
        const void *buf, int count);

    /**
     * @brief   Get the file descriptor for the UDP socket
     * @return  Returns the file descriptor associated with the socket or
     *          -1 on error
     */
    virtual int fd(void) const { return sock; }

    /**
     * @brief 	A signal that is emitted when data has been received
     * @param 	ip    The IP-address the data was received from
     * @param   port  The remote port number
     * @param 	buf   The buffer containing the read data
     * @param 	count The number of bytes read
     */
    sigc::signal<void, const IpAddress&, uint16_t, void*, int> dataReceived;
    
    /**
     * @brief 	A signal that is emitted when the send buffer is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    sigc::signal<void, bool> sendBufferFull;
    
  protected:
    virtual void onDataReceived(const IpAddress& ip, uint16_t port, void* buf,
        int count);

  private:
    int       	sock;
    FdWatch * 	rd_watch;
    FdWatch * 	wr_watch;
    UdpPacket * send_buf;
    
    void cleanup(void);
    void handleInput(FdWatch *watch);
    void sendRest(FdWatch *watch);

};  /* class UdpSocket */


} /* namespace */

#endif /* ASYNC_UDP_SOCKET_INCLUDED */



/*
 * This file has not been truncated
 */

