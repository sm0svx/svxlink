/**
@file	 AsyncTcpServer.h
@brief   A class for creating a TCP server
@author  Tobias Blomberg
@date	 2003-12-07

This class is used to create a TCP server that listens to a TCP-port.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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

/** @example AsyncTcpServer_demo.cpp
An example of how to use the Async::TcpServer class
*/


#ifndef ASYNC_TCP_SERVER_INCLUDED
#define ASYNC_TCP_SERVER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sigc++/signal_system.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpConnection.h>


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
@brief	A class for creating a TCP server
@author Tobias Blomberg
@date   2003-12-07

This class is used to create a TCP server that listens to a TCP-port. To use it,
just create an instance and specify the TCP-port to listen to. When a client
connects, a new Async::TcpConnection object is created which is used to do the
actual communication. An example of how to use it is shown below.

\include AsyncTcpServer_demo.cpp
*/
class TcpServer : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     * @param 	port_str A port number or service name to listen to
     */
    TcpServer(const std::string& port_str);
  
    /**
     * @brief 	Destructor
     */
    ~TcpServer(void);
  
    /**
     * @brief 	A signal that is emitted when a client connect to the server
     * @param 	con The new TcpConnection object
     */
    SigC::Signal1<void, TcpConnection *>  clientConnected;
    
    
  protected:
    
  private:
    int     sock;
    FdWatch *rd_watch;
    
    void cleanup(void);
    void incomingConnection(FdWatch *watch);
    
};  /* class TcpServer */


} /* namespace */

#endif /* ASYNC_TCP_SERVER_INCLUDED */



/*
 * This file has not been truncated
 */

