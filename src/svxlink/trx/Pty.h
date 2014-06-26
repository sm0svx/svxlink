/**
@file	 Pty.h
@brief   A class that wrap up some functionality to use a PTY
@author  Tobias Blomberg / SM0SVX
@date	 2014-06-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#ifndef PTY_INCLUDED
#define PTY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

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

namespace Async
{
  class FdWatch;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


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
@brief	A wrapper class for using a PTY
@author Tobias Blomberg / SM0SVX
@date   2014-06-07

This class wrap up some functionality that is nice to have when using a PTY.
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
     * @brief   Write a command to the PTY
     * @param   cmd The command to send
     * @return  Returns \em true on success or \em false on failure
     */
    bool write(char cmd);

    /**
     * @brief   Signal that is emitted when a command has been received
     * @param   cmd The received command
     */
    sigc::signal<void, char> cmdReceived;
    
  protected:
    
  private:
    std::string     slave_link;
    int     	    master;
    int             slave;
    Async::FdWatch  *watch;

    Pty(const Pty&);
    Pty& operator=(const Pty&);
    
    void charactersReceived(Async::FdWatch *w);

};  /* class Pty */


//} /* namespace */

#endif /* PTY_INCLUDED */



/*
 * This file has not been truncated
 */
