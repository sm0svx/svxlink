/**
@file	 AsyncSerial.cpp
@brief   A class for using asynchronous serial communications
@author  Tobias Blomberg / SM0SVX
@date	 2004-08-02

This file contains a class that is used to communicate over an
asynchronous serial link (e.g. RS-232 port).

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
#include <sys/select.h>
#include <fcntl.h>
#include <errno.h>

#include <cstdio>
#include <cassert>
#include <iostream>


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
Serial::Serial(const string& serial_port, int speed, Parity parity,
      	       int bits, int stop_bits, Flow flow)
  : serial_port(serial_port), speed(speed), parity(parity), bits(bits),
    stop_bits(stop_bits), flow(flow), canonical(false), fd(-1), rd_watch(0)
{

} /* Serial::Serial */


Serial::~Serial(void)
{
  close();
} /* Serial::~Serial */



bool Serial::open(void)
{
  if (fd != -1)
  {
    return true;
  }
  
    /* Setup the serial port parameters */
  memset(&port_settings, 0, sizeof(port_settings));
  port_settings.c_iflag = INPCK   /* Perform parity checking */
      	      	      	| IGNPAR  /* Ignore characters with parity errors */
			| IGNBRK  /* Ignore BREAK condition */
      	      	      	;
  port_settings.c_cflag = CREAD   /* Enable receiver */
      	      	      	| CLOCAL  /* Ignore modem status lines */
			;
  switch (flow)
  {
    case FLOW_NONE:
      break;
    case FLOW_HW:
      port_settings.c_cflag |= CRTSCTS /*(CCTS_OFLOW | CRTS_IFLOW)*/;
      break;
    case FLOW_XONOFF:
      port_settings.c_iflag |= (IXON | IXOFF);
      break;
    default:
      errno = EINVAL;
      return false;
  }
  switch (bits)
  {
    case 5:
      port_settings.c_cflag |= CS5;
      break;
    case 6:
      port_settings.c_cflag |= CS6;
      break;
    case 7:
      port_settings.c_cflag |= CS7;
      break;
    case 8:
      port_settings.c_cflag |= CS8;
      break;
    default:
      errno = EINVAL;
      return false;
  }
  switch (stop_bits)
  {
    case 1:
      break;
    case 2:
      port_settings.c_cflag |= CSTOPB;
      break;
    default:
      errno = EINVAL;
      return false;
  }
  switch (parity)
  {
    case PARITY_NONE:
      break;
    case PARITY_EVEN:
      port_settings.c_cflag |= PARENB;
      break;
    case PARITY_ODD:
      port_settings.c_cflag |= (PARENB | PARODD);
      break;
    default:
      errno = EINVAL;
      return false;
  }
  
  speed_t serial_speed;
  switch (speed)
  {
    case 50:
      serial_speed = B50;
      break;
    case 75:
      serial_speed = B75;
      break;
    case 110:
      serial_speed = B110;
      break;
    case 134:
      serial_speed = B134;
      break;
    case 150:
      serial_speed = B150;
      break;
    case 200:
      serial_speed = B200;
      break;
    case 300:
      serial_speed = B300;
      break;
    case 600:
      serial_speed = B600;
      break;
    case 1200:
      serial_speed = B1200;
      break;
    case 1800:
      serial_speed = B1800;
      break;
    case 2400:
      serial_speed = B2400;
      break;
    case 4800:
      serial_speed = B4800;
      break;
    case 9600:
      serial_speed = B9600;
      break;
    case 19200:
      serial_speed = B19200;
      break;
    case 38400:
      serial_speed = B38400;
      break;
    case 57600:
      serial_speed = B57600;
      break;
    case 115200:
      serial_speed = B115200;
      break;
    case 230400:
      serial_speed = B230400;
      break;
    default:
      errno = EINVAL;
      return false;
  }
  
  if (cfsetospeed(&port_settings, serial_speed) == -1)
  {
    return false;
  }
  if (cfsetispeed(&port_settings, serial_speed) == -1)
  {
    return false;
  }
  
  fd = ::open(serial_port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd == -1)
  {
    return false;
  }
  
  if (tcflush(fd, TCIOFLUSH) == -1)
  {
    int errno_tmp = errno;
    ::close(fd);
    fd = -1;
    errno = errno_tmp;
    return false;
  }
  
  if(tcgetattr(fd, &old_port_settings) == -1)
  {
    int errno_tmp = errno;
    ::close(fd);
    fd = -1;
    errno = errno_tmp;
    return false;
  }
  
  if (tcsetattr(fd, TCSANOW, &port_settings) == -1)
  {
    int errno_tmp = errno;
    ::close(fd);
    fd = -1;
    errno = errno_tmp;
    return false;
  }
  
  setCanonical(canonical);
  
  rd_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
  rd_watch->activity.connect(slot(this, &Serial::onIncomingData));
  
  return true;
  
} /* Serial::open */


bool Serial::close(void)
{
  if (fd == -1)
  {
    return true;
  }
  
  delete rd_watch;
  rd_watch = 0;
  
  if (tcsetattr(fd, TCSANOW, &old_port_settings) == -1)
  {
    int errno_tmp = errno;
    ::close(fd);
    fd = -1;
    errno = errno_tmp;
    return false;
  }
  
  if (::close(fd) == -1)
  {
    return false;
  }
  
  fd = -1;
  
  return true;
  
} /* Serial::close */


bool Serial::setCanonical(bool canonical)
{
  this->canonical = canonical;
  
  if (fd != -1)
  {
    if (canonical)
    {
      port_settings.c_lflag |= ICANON;
    }
    else
    {
      port_settings.c_lflag &= ~ICANON;
      //port_settings.c_cc[VMIN] = 80;
      //port_settings.c_cc[VTIME] = 1;
    }

    if (tcsetattr(fd, TCSAFLUSH, &port_settings) == -1)
    {
      return false;
    }
  }
   
  return true;
  
} /* Serial::setCanonical */


bool Serial::stopInput(bool stop)
{
  return tcflow(fd, stop ? TCIOFF : TCION) == 0;
} /* Serial::stopInput */



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
void Serial::onIncomingData(FdWatch *watch)
{
  char buf[READ_BUFSIZE];
  int cnt;
  
  cnt = ::read(fd, buf, sizeof(buf)-1);
  if (cnt == -1)
  {
    perror("read");
    return;
  }
  
  buf[cnt] = 0;
  charactersReceived(buf, cnt);
  
} /* Serial::onIncomingData */






/*
 * This file has not been truncated
 */

