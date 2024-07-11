/**
@file	 AsyncTcpClient.h
@brief   Contains a class for creating TCP client connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class that make it easy to create a new TCP connection
to a remote host. See usage instructions in the class definition.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2024 Tobias Blomberg

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

/** @example AsyncTcpClient_demo.cpp
An example of how to use the Async::TcpClient class
*/



#ifndef ASYNC_TCP_CLIENT_INCLUDED
#define ASYNC_TCP_CLIENT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <stdint.h>

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClientBase.h>


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
class DnsLookup;
class IpAddress;


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
@brief	A class for creating a TCP client connection
@author Tobias Blomberg
@date   2003-04-12

This class is used to create a TCP client connection. All details of how to
create the connection is hidden inside the class. This make it very easy to
create and use the connections. An example usage is shown below.

\include AsyncTcpClient_demo.cpp
*/
template <typename ConT=TcpConnection>
class TcpClient : public ConT, public TcpClientBase
{
  public:
    /**
     * @brief   Constructor
     * @param 	recv_buf_len  The length of the receiver buffer to use
     *
     * The object will be constructed and variables will be initialized but
     * no connection will be created until the connect function
     * (see @ref TcpClient::connect) is called.
     * When using this variant of the constructor the connect method which
     * take host and port must be used.
     */
    explicit TcpClient(size_t recv_buf_len = ConT::DEFAULT_RECV_BUF_LEN)
      : ConT(recv_buf_len), TcpClientBase(this)
    {
    }
    
    /**
     * @brief 	Constructor
     * @param 	remote_host   The hostname of the remote host
     * @param 	remote_port   The port on the remote host to connect to
     * @param 	recv_buf_len  The length of the receiver buffer to use
     *
     * The object will be constructed and variables will be initialized but
     * no connection will be created until the connect function
     * (see @ref TcpClient::connect) is called.
     */
    TcpClient(const std::string& remote_host, uint16_t remote_port,
              size_t recv_buf_len = ConT::DEFAULT_RECV_BUF_LEN)
      : ConT(recv_buf_len), TcpClientBase(this, remote_host, remote_port)
    {
    }
    
    /**
     * @brief 	Constructor
     * @param 	remote_ip     The IP address of the remote host
     * @param 	remote_port   The port on the remote host to connect to
     * @param 	recv_buf_len  The length of the receiver buffer to use
     *
     * The object will be constructed and variables will be initialized but
     * no connection will be created until the connect function
     * (see @ref TcpClient::connect) is called.
     */
    TcpClient(const IpAddress& remote_ip, uint16_t remote_port,
              size_t recv_buf_len = ConT::DEFAULT_RECV_BUF_LEN)
      : ConT(recv_buf_len), TcpClientBase(this, remote_ip, remote_port)
    {
    }
    
    /**
     * @brief 	Destructor
     */
    ~TcpClient(void) {}

    /**
     * @brief 	Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    virtual void disconnect(void) override { closeConnection(); }

    /**
     * @brief   Check if the connection is idle
     * @return  Returns \em true if the connection is idle
     *
     * A connection being idle means that it is not connected nor connecting.
     */
    bool isIdle(void) const
    {
      return TcpClientBase::isIdle() && ConT::isIdle();
    }

    void setSslContext(SslContext& ctx)
    {
      ConT::setSslContext(ctx, false);
    }

  protected:
    /**
     * @brief   Disconnect from the remote peer
     *
     * This function is used internally to close the connection to the remote
     * peer.
     */
    virtual void closeConnection(void) override
    {
      ConT::closeConnection();
      TcpClientBase::closeConnection();
    }

    using ConT::operator=;
    using TcpClientBase::operator=;

  private:

};  /* class TcpClient */


} /* namespace */

#endif /* ASYNC_TCP_CLIENT_INCLUDED */



/*
 * This file has not been truncated
 */

