/**
@file	 AsyncSerialDevice.cpp
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

#include "AsyncSerial.h"
#include "AsyncSerialDevice.h"



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

map<string, SerialDevice *> SerialDevice::dev_map;


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
SerialDevice *SerialDevice::open(const string& port, bool flush)
{
  SerialDevice *dev = 0;
  
  if (dev_map.count(port) == 0)
  {
    dev_map[port] = new SerialDevice(port);
  }
  dev = dev_map[port];
  
  if (dev->use_count++ == 0)
  {
    if (!dev->openPort(flush))
    {
      delete dev;
      dev = 0;
    }
  }
  
  return dev;
  
} /* SerialDevice::instance */


bool SerialDevice::close(SerialDevice *dev)
{
  bool success = true;
  
  if (--dev->use_count == 0)
  {
    dev_map.erase(dev->port_name);
    success = dev->closePort();
    delete dev;
  }
  
  return success;
  
} /* SerialDevice::close */



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
SerialDevice::SerialDevice(const string& port)
  : port_name(port), use_count(0), fd(-1), old_port_settings(),
    rd_watch(0), restore_on_close(false)
{
  
} /* SerialDevice::SerialDevice */


SerialDevice::~SerialDevice(void)
{
  delete rd_watch;
} /* SerialDevice::~SerialDevice */


bool SerialDevice::openPort(bool flush)
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
  rd_watch->activity.connect(mem_fun(*this, &SerialDevice::onIncomingData));
  
  return true;
  
} /* SerialDevice::openPort */


bool SerialDevice::closePort(void)
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
  
} /* SerialDevice::closePort */


void SerialDevice::onIncomingData(FdWatch *watch)
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
  
} /* SerialDevice::onIncomingData */





/*
 * This file has not been truncated
 */

