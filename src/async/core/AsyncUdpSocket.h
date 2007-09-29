/**
 * @file    AsyncUdpSocket.h
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
@brief	A class for working with UDP sockets
@author Tobias Blomberg
@date   2003-04-26

This class is used to work with UDP sockets. An example usage is shown below.

\include AsyncUdpSocket_demo.cpp
*/
class UdpSocket : public SigC::Object
{
  public:
    /**
     * @brief 	Constructor
     * @param 	local_port  The local port to use. If not specified, a random
     *	      	      	    local port will be used.
     */
    UdpSocket(uint16_t local_port=0);
  
    /**
     * @brief 	Destructor
     */
    ~UdpSocket(void);
    
    /**
     * @brief 	Check if the initialization was ok
     * @return	Returns \em true if everything went fine during initialization
     *	      	or \em false if something went wrong
     *
     * This function should always be called after constructing the object to
     * see if everything went fine.
     */
    bool initOk(void) const { return (sock != -1); }
    
    /**
     * @brief 	Write data to the remote host
     * @param 	remote_ip   The IP-address of the remote host
     * @param 	remote_port The remote port to use
     * @param 	buf   	    A buffer containing the data to send
     * @param 	count       The number of bytes to write
     * @return	Return \em true on success or \em false on failure
     */
    bool write(const IpAddress& remote_ip, int remote_port, const void *buf,
	int count);
    
    /**
     * @brief 	A signal that is emitted when data has been received
     * @param 	ip    The IP-address the data was received from
     * @param 	buf   The buffer containing the read data
     * @param 	count The number of bytes read
     */
    SigC::Signal3<void, const IpAddress&, void *, int> dataReceived;
    
    /**
     * @brief 	A signal that is emitted when the send buffer is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    SigC::Signal1<void, bool> sendBufferFull;
    
  protected:
    
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

