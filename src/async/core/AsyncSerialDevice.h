/**
@file	 AsyncSerialDevice.h
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
class SerialDevice : public sigc::trackable
{
  public:
    /**
     * @brief 	Retrieve a SerialDevice object and open the port
     * @param 	port The name of the port to open
     * @return	Returns a SerialDevice object or 0 on failure
     *
     * Use this static function to retrieve a SerialDevice object. If
     * one does not already exist, a new one will be created and
     * the serial port will be opened.
     */
    static SerialDevice *open(const std::string& port, bool flush);

    /**
     * @brief 	Release a SerialDevice object and close the port
     * @param 	dev The SerialDevice object to release
     * @return	Return \em true on success or \em false on failure
     *
     * Use this static function to release a SerialDevice object. If
     * there are no more users left, the given object will be deleted
     * and the serial port closed.
     */
    static bool close(SerialDevice *dev);
    
    /**
     * @brief 	Return the file descriptor associated with this serial device
     * @return	Returns the file descriptor associated with this serial device
     */
    int desc(void) { return fd; }

    /**
     * @brief   Set a flag to restore port settings on close
     *
     * Call this function once, before or after open, to instruct this class
     * to restore port settings, that was saved when opening the port, on
     * closing the port. When the port is closed the flag is reset again.
     */
    void setRestoreOnClose(void) { restore_on_close = true; }
    
    /**
     * @brief 	A signal that is emitted when there is data to read
     * @param 	buf   A buffer containing the data that has been read
     * @param 	count The number of bytes that was read
     * @note  	For maximum buffer size see @ref Serial::READ_BUFSIZE
     * 
     * This signal is emitted whenever one or more characters has been
     * received on the serial port. The buffer is always null-terminated
     * but the null is not included in the count.
     */
    sigc::signal<void, char*, int> charactersReceived;
    
    
  protected:
    
  private:
    static std::map<std::string, SerialDevice *>  dev_map;
    
    std::string     port_name;
    int       	    use_count;
    int       	    fd;
    struct termios  old_port_settings;
    FdWatch   	    *rd_watch;
    bool            restore_on_close;
    
    SerialDevice(const std::string& port);
    ~SerialDevice(void);
    bool openPort(bool flush);
    bool closePort(void);
    void onIncomingData(FdWatch *watch);
  
    
};  /* class SerialDevice */


} /* namespace */

#endif /* ASYNC_SERIAL_DEVICE_INCLUDED */



/*
 * This file has not been truncated
 */

