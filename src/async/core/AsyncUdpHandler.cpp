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

#include "UdpSock.h"
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

map<string, UdpHandler *> UdpHandler::dev_map;


/****************************************************************************
 *
 * Public member functions
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
UdpHandler *UdpHandler::open(void)
{
  UdpHandler *dev = 0;

  if (dev_map.count(portnr) == 0)
  {
    dev_map[portnr] = new UdpHandler(portnr, ip_ad);
  }
  dev = dev_map[portnr];

  if (dev->use_count++ == 0)
  {
    if (!dev->openPort(flush))
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


/*
 *----------------------------------------------------------------------------
 * Method:
 * Purpose:
 * Input:
 * Output:
 * Author:
 * Created:
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
UdpHandler::UdpHandler(uint16_t port, const IpAddress &ip_addr)
  : portnr(port), ip_ad(ip_addr)
{

} /* UdpHandler::UdpHandler */


UdpHandler::~UdpHandler(void)
{
  delete rd_watch;
} /* UdpHandler::~UdpHandler */


bool UdpHandler::openPort(bool flush)
{
  fd = ::open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd == -1)
  {
    return false;
  }

  if (flush)
  {
    if (tcflush(fd, TCIOFLUSH) == -1)
    {
      int errno_tmp = errno;
      ::close(fd);
      fd = -1;
      errno = errno_tmp;
      return false;
    }
  }

  if(tcgetattr(fd, &old_port_settings) == -1)
  {
    int errno_tmp = errno;
    ::close(fd);
    fd = -1;
    errno = errno_tmp;
    return false;
  }

  rd_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
  rd_watch->activity.connect(mem_fun(*this, &UdpHandler::onIncomingData));

  return true;

} /* UdpHandler::openPort */


bool UdpHandler::closePort(void)
{
  if (restore_on_close)
  {
    if (tcsetattr(fd, TCSANOW, &old_port_settings) == -1)
    {
      int errno_tmp = errno;
      ::close(fd);
      fd = -1;
      errno = errno_tmp;
      return false;
    }
    restore_on_close = false;
  }

  if (::close(fd) == -1)
  {
    return false;
  }

  fd = -1;

  return true;

} /* UdpHandler::closePort */


void UdpHandler::onIncomingData(FdWatch *watch)
{
  char buf[Serial::READ_BUFSIZE];
  int cnt;

  cnt = ::read(fd, buf, sizeof(buf)-1);
  if (cnt == -1)
  {
    perror("read");
    return;
  }

  buf[cnt] = 0;
  charactersReceived(buf, cnt);

} /* UdpHandler::onIncomingData */





/*
 * This file has not been truncated
 */

