/**
@file	 AsyncUdpHandler.h
@brief   An internal class used by Async::Serial
@author  Tobias Blomberg / SM0SVX
@date	 2005-03-10

This is an internal class that is used by the Async::Serial class. It should
never be used directly by the user of the Async lib.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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



#ifndef ASYNC_SERIAL_DEVICE_INCLUDED
#define ASYNC_SERIAL_DEVICE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <termios.h>

#include <string>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncUdpSocket.h>
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
@brief	An internal class used by the Async::Serial class
@author Tobias Blomberg
@date   2005-03-10
*/
class UdpHandler : public sigc::trackable
{
  public:


    explicit UdpHandler(const uint16_t &port, const IpAddress &ip_addr);

    ~UdpHandler(void);

    /**
     * @brief 	Retrieve a UdpHandler object and open the port
     * @param 	port The name of the port to open
     * @return	Returns a UdpHandler object or 0 on failure
     *
     * Use this static function to retrieve a UdpHandler object. If
     * one does not already exist, a new one will be created and
     * the serial port will be opened.
     */
    Async::UdpSocket *open(void);

    /**
     * @brief 	Release a UdpHandler object and close the port
     * @param 	dev The UdpHandler object to release
     * @return	Return \em true on success or \em false on failure
     *
     * Use this static function to release a UdpHandler object. If
     * there are no more users left, the given object will be deleted
     * and the serial port closed.
     */
    bool close(UdpHandler *dev);

    /**
     * @brief   Set a flag to restore port settings on close
     *
     * Call this function once, before or after open, to instruct this class
     * to restore port settings, that was saved when opening the port, on
     * closing the port. When the port is closed the flag is reset again.
     */
    void setRestoreOnClose(void) {  }

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
     * @param   port  The remote port number
     * @param 	buf   The buffer containing the read data
     * @param 	count The number of bytes read
     */
    sigc::signal<void, const IpAddress&, uint16_t, void*, int> dataReceived;

    void udpDataReceived(const Async::IpAddress& ip_addr,uint16_t port, void *data, int len);


  protected:


  private:

    static std::map<uint16_t, Async::UdpSocket *> dev_map;

    uint16_t               portnr;
    Async::IpAddress       ip_addr;
    int                    use_count;


};  /* class UdpHandler */


} /* namespace */

#endif /* ASYNC_SERIAL_DEVICE_INCLUDED */



/*
 * This file has not been truncated
 */

