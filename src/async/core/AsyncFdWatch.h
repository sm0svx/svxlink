/**
@file	 AsyncFdWatch.h
@brief   Contains a watch for file descriptors
@author  Tobias Blomberg
@date	 2003-03-19

This file contains a watch for file descriptors. When activity is found on the
file descriptor, a signal is emitted.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

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

/** @example  AsyncFdWatch_demo.cpp
An example of how to use the  Async::FdWatch class
*/


#ifndef ASYNC_FD_WATCH_INCLUDED
#define ASYNC_FD_WATCH_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


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
@brief	A class for watching file descriptors
@author Tobias Blomberg
@date   2003-03-19

Use this class to watch a file descriptor for activity. The example
below creates a read watch on the standard input file descriptor. That is,
every time a character is typed on the keyboard (or something is piped to the
application) the \em onActivity method in instance \em this of class \em MyClass
will be called. In the handler function, the data on the file descriptor should
be read. Otherwise the handler function will be called over and over again.
@note Since the stdin is line buffered, the ENTER key has to be pressed before
      anything will be shown.

\include AsyncFdWatch_demo.cpp
*/   
class FdWatch : public sigc::trackable
{
  public:
    /**
     * @brief The type of the file descriptor watch
     */
    typedef enum
    { 
      FD_WATCH_RD,  ///< File descriptor watch for incoming data
      FD_WATCH_WR   ///< File descriptor watch for outgoing data
    } FdWatchType;
    
    /**
     * @brief Default constructor
     *
     * Create a disabled FdWatch. Use the setFd function to set the
     * filedescriptor to watch and the type of watch.
     */
    FdWatch(void);
    
    /**
     * @brief Constructor
     *
     * Add the given file descriptor to the watch list and watch it for
     * incoming data (FD_WATCH_RD) or write buffer space available
     * (FD_WATCH_WR).
     * @param fd    The file descriptor to watch
     * @param type  The type of watch to create (see @ref FdWatchType)
     */
    FdWatch(int fd, FdWatchType type);
    
    /**
     * @brief Destructor
     */
    ~FdWatch(void);

    /**
     * @brief   Assignment move operator
     * @param   other The object to move data from
     * @return  Returns a reference to this object
     *
     * The move operator move the state of a specified FdWatch object into this
     * object. After the move, the state of the other object will be the same
     * as if it had just been default constructed.
     */
    FdWatch& operator=(FdWatch&& other);

    /**
     * @brief Return the file descriptor being watched
     * @return Returns the file descriptor
     */
    int fd(void) const { return m_fd; }
    
    /**
     * @brief Return the type of this watch
     * @return Returns the type (see @ref FdWatchType)
     */
    FdWatchType type(void) const { return m_type; }
    
    /**
     * @brief Enable or disable the watch
     * @param enabled Set to \em true to enable the watch or \em false to
     *	      	      disable it.
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief Check if the watch is enabled or not
     * @return Returns true if the watch is enabled, or else false.
     */
    bool isEnabled(void) const { return m_enabled; }
    
    /**
     * @brief Set the file descriptor to watch
     * @param fd    The file descriptor to watch
     * @param type  The type of watch to create (see @ref FdWatchType)
     *
     * This function can be used at any time to change the file descriptor or
     * type of watch. If the watch was disabled it will stay disabled until
     * explicitly being enabled. If fd < 0 the watch will be disabled if it was
     * enabled.
     */
    void setFd(int fd, FdWatchType type);

    /**
     * @brief Signal to indicate that the descriptor is active
     * @param watch Pointer to the watch object
     */
    sigc::signal<void, FdWatch*> activity;
    
    
  protected:
    
  private:
    int       	m_fd;
    FdWatchType m_type;
    bool      	m_enabled;
  
};  /* class FdWatch */


} /* namespace */

#endif /* ASYNC_FD_WATCH_INCLUDED */



/*
 * This file has not been truncated
 */

