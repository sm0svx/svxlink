/**
@file	 AsyncPty.h
@brief   A class that wrap up some functionality to use a PTY
@author  Tobias Blomberg / SM0SVX
@date	 2014-06-07

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_PTY_INCLUDED
#define ASYNC_PTY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <sigc++/sigc++.h>

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


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
@brief	A wrapper class for using a PTY
@author Tobias Blomberg / SM0SVX
@date   2014-06-07

This class wrap up some functionality that is nice to have when using a PTY.

The PTY is opened in raw mode.

If the slave end of the PTY is not open, the master file descriptor will be
continuously polled to detect when it is opened.  When the slave end of the PTY
is open, an FdWatch will be used to check for activity instead of polling.

Data written to the master end will be discarded if the slave end is not open.
*/
class Pty : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constructor
     */
    Pty(const std::string &slave_link="");
  
    /**
     * @brief 	Destructor
     */
    ~Pty(void);

    void setLineBuffered(bool line_buffered)
    {
      m_is_line_buffered = line_buffered;
      m_line_buffer.clear();
    }

    /**
     * @brief   Open the PTY
     * @return  Returns \em true on success or \em false on failure
     *
     * Use this function to open the PTY. If the PTY is already open it will
     * be closed first.
     */
    bool open(void);

    /**
     * @brief   Close the PTY if it's open
     *
     * Close the PTY if it's open. This function is safe to call even if
     * the PTY is not open or if it's just partly opened.
     */
    void close(void);

    /**
     * @brief   Reopen the PTY
     * @return  Returns \em true on success or \em false on failure
     *
     * Try to reopen the PTY. On failure an error message will be printed
     * and the PTY will stay closed.
     */
    bool reopen(void);

    /**
     * @brief   Write data to the PTY
     * @param   buf A buffer containing the data to write
     * @param   count The number of bytes to write
     * @return  On success, the number of bytes written is returned (zero
     *          indicates nothing was written).  On error, -1 is returned,
     *          and errno is set appropriately.
     * 
     * Use this function to write data to the PTY. If the slave end of the PTY
     * is not open, the written data will just be discarded and \em count is
     * used as the return value.
     */
    ssize_t write(const void *buf, size_t count);

    /**
     * @brief   Check if the PTY is open or not
     * @return  Returns \em true if the PTY has been successfully opened
     */
    bool isOpen(void) const { return master >= 0; }

    /**
     * @brief   Signal that is emitted when data has been received
     * @param   buf A buffer containing the received data
     * @param   count The number of bytes in the buffer
     */
    sigc::signal<void, const void*, size_t> dataReceived;
    
  protected:
    
  private:
    static const int POLLHUP_CHECK_INTERVAL = 100;

    std::string     slave_link;
    int     	    master;
    Async::FdWatch  *watch;
    Async::Timer    pollhup_timer;
    bool            m_is_line_buffered = false;
    std::string     m_line_buffer;

    Pty(const Pty&);
    Pty& operator=(const Pty&);
    
    void charactersReceived(void);
    short pollMaster(void);
    void checkIfSlaveEndOpen(void);

};  /* class Pty */


} /* namespace */

#endif /* ASYNC_PTY_INCLUDED */



/*
 * This file has not been truncated
 */
