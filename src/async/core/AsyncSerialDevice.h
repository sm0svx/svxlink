/**
@file	 AsyncSerialDevice.h
@brief   An internal class used by Async::Serial
@author  Tobias Blomberg / SM0SVX
@date	 2005-03-10

This is an internal class that is used by the Async::Serial class. It should
never be used directly by the user of the Async lib.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

#include <termios.h>

#include <string>
#include <map>


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
class SerialDevice
{
  public:
    static SerialDevice *open(const std::string& port);
    static bool close(SerialDevice *dev);
    
    int desc(void) { return fd; }
    
    
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    
  protected:
    
  private:
    static std::map<std::string, SerialDevice *>  dev_map;
    
    std::string     port_name;
    int       	    use_count;
    int       	    fd;
    struct termios  old_port_settings;
    
    SerialDevice(const std::string& port);
    ~SerialDevice(void);
    bool openPort(void);
    bool closePort(void);

  
    
};  /* class SerialDevice */


} /* namespace */

#endif /* ASYNC_SERIAL_DEVICE_INCLUDED */



/*
 * This file has not been truncated
 */

