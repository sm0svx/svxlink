/**
@file	 AsyncTcpServer.h
@brief   A class for creating a TCP server
@author  Tobias Blomberg
@date	 2003-12-07

This class is used to create a TCP server that listens to a TCP-port.

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
#include <vector>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpServerBase.h>


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
template <typename ConT=TcpConnection>
class TcpServer : public TcpServerBase
{
  public:
    /**
     * @brief 	Default constuctor
     * @param 	port_str A port number or service name to listen to
     * @param 	bind_ip The IP to bind the server to
     */
    TcpServer(const std::string& port_str,
              const Async::IpAddress &bind_ip=IpAddress())
      : TcpServerBase(port_str, bind_ip)
    {
    }
  
    /**
     * @brief 	Destructor
     */
    virtual ~TcpServer(void) {}
  
    /**
     * @brief 	Get the client object pointer from the server
     * @param 	index The wanted client by number 0 - numberOfClients()-1
     * @return 	The TcpConnection pointer to the client (zero if not found)
     */
    ConT *getClient(unsigned int index)
    {
      TcpConnection *con = TcpServerBase::getClient(index);
      return dynamic_cast<ConT*>(con);
    }

    /**
     * @brief 	A signal that is emitted when a client connect to the server
     * @param 	con The connected TcpConnection object
     */
    sigc::signal<void, ConT*>  clientConnected;
  
    /**
     * @brief 	A signal that is emitted when a client disconnect from the
     *	      	server
     * @param 	con The disconnected TcpConnection object
     */
    sigc::signal<void, ConT*, typename ConT::DisconnectReason>
            clientDisconnected;
  
  protected:
    virtual void createConnection(int sock, const IpAddress& remote_addr,
                                  uint16_t remote_port)
    {
      ConT *con = new ConT(sock, remote_addr, remote_port);
      con->disconnected.connect(
          mem_fun(*this, &TcpServer<ConT>::onDisconnected));
      addConnection(con);
      clientConnected(con);
    }
    
  private:
    void onDisconnected(ConT *con, typename ConT::DisconnectReason reason)
    {
      clientDisconnected(con, reason);
      removeConnection(con);
    }
    
};  /* class TcpServer */


} /* namespace */

#endif /* ASYNC_TCP_SERVER_INCLUDED */



/*
 * This file has not been truncated
 */



