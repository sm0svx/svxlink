/**
@file	 AsyncSerial.h
@brief   A class for using asynchronous serial communications
@author  Tobias Blomberg / SM0SVX
@date	 2004-08-02

This file contains a class that is used to communicate over an
asynchronous serial link (e.g. RS-232 port).

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

/** @example AsyncSerial_demo.cpp
An example of how to use the Serial class
*/


#ifndef SERIAL_INCLUDED
#define SERIAL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>
#include <termios.h>
#include <unistd.h>

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
class SerialDevice;
  

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
@brief	A class for using asyncronous serial communications
@author Tobias Blomberg
@date   2004-08-02
@note 	The flow control code is untested. Report success/failure to me.

This class is used to communicate over an asynchronous serial link
(e.g. RS-232 port). An example of how to use it is shown below.

\include AsyncSerial_demo.cpp
*/
class Serial : public SigC::Object
{
  public:
    /**
     * @brief A type that defines the possible choices for parity
     */
    typedef enum
    {
      PARITY_NONE,  ///< Parity not used
      PARITY_EVEN,  ///< Use even parity
      PARITY_ODD    ///< Use odd parity
    } Parity;

    /**
     * @brief A type that defines the possible choices for flow control
     */
    typedef enum
    {
      FLOW_NONE,    ///< No flow control in use
      FLOW_HW,	    ///< Use hardware flow control
      FLOW_XONOFF   ///< Use software (XON/XOFF) flow control
    } Flow;
    
    /**
     * @brief A type that defines the output pins in the serial port
     */
    typedef enum
    {
      PIN_RTS,	///< Request To Send
      PIN_DTR 	///< Data Terminal Ready
    } OutPin;
    
    /**
     * @brief A type that defines the input pins in the serial port
     */
    typedef enum
    {
      PIN_CTS,	///< Clear To Send
      PIN_DSR, 	///< Data Set Ready
      PIN_DCD,	///< Data Carrier Detect
      PIN_RI  	///< Ring Indicate
    } InPin;
    
    /**
     * @brief 	The maximum number of characters that can be read at once.
     */
    static const int READ_BUFSIZE = 1024;
        

    /**
     * @brief 	Constuctor
     * @param 	serial_port The serial port to use
     */
    Serial(const std::string& serial_port);
  
    /**
     * @brief 	Destructor
     */
    ~Serial(void);
    
    /**
     * @brief 	Setup the serial port communications parameter
     * @param 	speed 	    The serial speed to use
     * @param 	parity	    The parity to use (see @ref Serial::Parity)
     * @param 	bits  	    The number of bits in each serial word
     * @param 	stop_bits   The number of stop bits to use
     * @param 	flow  	    The type of flow control to use
     *	      	      	    (see @ref Serial::Flow)
     * @return	Return \em true on success or else \em false on failue. On
     *	      	failure the global variable \em errno will be set to indicate
     *	      	the cause of the error.
     *
     * This function is used to setup the serial communication parameters
     * for the serial port. This must be done after calling the open
     * function.
     */
    bool setParams(int speed, Parity parity, int bits, int stop_bits,
      	      	   Flow flow);
    
    /**
     * @brief 	Open the serial port using the previously defined parameters
     * @return	Return \em true on success or else \em false on failue. On
     *	      	failure the global variable \em errno will be set to indicate
     *	      	the cause of the error.
     *
     * Call this method to open the serial port using the parameters specified
     * to the constructor. If the open method is called when the port is
     * already opened, it just returns \em true without doing anything.
     */
    bool open(void);
    
    /**
     * @brief 	Close a previously opened serial port
     * @return	Return \em true on success or else \em false on failue. On
     *	      	failure the global variable \em errno will be set to indicate
     *	      	the cause of the error.
     *
     * Use this method to close a previously opened serial port. If the port is
     * already closed, \em true is returned and nothing is done.
     */
    bool close(void);
    
    /**
     * @brief 	Write data to the serial port
     * @param 	buf   A buffer containing the data to write
     * @param 	count The number of bytes to write
     * @returns The number of bytes written is returned on success. If an error
     *	      	occurs, -1 is returned and the global variable \em errno is
     *	      	set to indicate the cause of the error.
     */
    int write(const char *buf, size_t count)
    {
      return ::write(fd, buf, count);
    }

    /**
     * @brief 	Set or clear canonical mode
     * @return	Return \em true on success or else \em false on failue. On
     *	      	failure the global variable \em errno will be set to indicate
     *	      	the cause of the error.
     *
     * Call this method to configure the serial port for canonical mode.
     * In this mode, a couple of control characters are parsed by the kernel
     * driver and interpreted in special ways. Most notably, only whole lines
     * of text are sent up to the application. Don't use canonical mode when
     * the serial stream is not readable text that is split into lines with
     * a line feed character.
     *
     * This function may be called both before or after the port has been
     * opened and the setting is remembered after a close.
     */
    bool setCanonical(bool canonical);
    
    /**
     * @brief 	Stop/start input of data
     * @param 	stop  Stop input if \em true or start input if \em false
     * @return	Return \em true on success or else \em false on failue. On
     *	      	failure the global variable \em errno will be set to indicate
     *	      	the cause of the error.
     *
     * Use this function to start/stop input of data when the flow control has
     * been enabled.
     */
    bool stopInput(bool stop);

    bool setPin(OutPin pin, bool set);
    bool getPin(InPin pin, bool &is_set);

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
    SigC::Signal2<void, char*, int> charactersReceived;
    
      
  protected:
    
  private:    
    const std::string	serial_port;
    bool      	      	canonical;
    
    int       	      	fd;
    struct termios    	port_settings;
    SerialDevice      	*dev;
    

};  /* class Serial */


} /* namespace */

#endif /* SERIAL_INCLUDED */



/*
 * This file has not been truncated
 */

