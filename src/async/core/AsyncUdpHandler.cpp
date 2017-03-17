/**
@file	 AsyncUdpHandler.cpp
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncUdpSocket.h"
#include "AsyncUdpHandler.h"



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

map<uint16_t, UdpHandler *> UdpHandler::dev_map;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

UdpHandler::UdpHandler(const uint16_t &port, const IpAddress &ip_addr)
  : portnr(port), ip_addr(ip_addr), use_count(0)
{
} /* UdpHandler::UdpHandler */


UdpHandler::~UdpHandler(void)
{
} /* UdpHandler::~UdpHandler */

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
UdpHandler *UdpHandler::open(const uint16_t &port, const IpAddress &ip_addr)
{
  UdpHandler *dev = 0;

  if (dev_map.count(port) == 0)
  {
    dev_map[port] = new UdpHandler(port, ip_addr);
  }
  dev = dev_map[port];

  if (dev->use_count++ == 0)
  {
    if (!dev->openPort())
    {
      delete dev;
      dev = 0;
    }
  }

  return dev;

} /* UdpHandler::instance */


bool UdpHandler::close(UdpHandler *dev)
{
  bool success = true;

  if (--dev->use_count == 0)
  {
    dev_map.erase(dev->portnr);
    success = dev->closePort();
    delete dev;
  }

  return success;

} /* UdpHandler::close */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


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






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


bool UdpHandler::openPort(void)
{
  return true;
} /* UdpHandler::openPort */


bool UdpHandler::closePort(void)
{
  return true;
} /* UdpHandler::closePort */


void UdpHandler::onIncomingData(FdWatch *watch)
{


} /* UdpHandler::onIncomingData */


bool UdpHandler::write(const IpAddress& remote_ip, int remote_port, const void *buf,
	int count)
{

  return true;
}


/*
 * This file has not been truncated
 */

