/**
@file   AsyncTcpPrioClient.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2021-06-27

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

/** @example AsyncTcpPrioClient_demo.cpp
An example of how to use the Async::TcpPrioClient class
*/

#ifndef ASYNC_TCP_PRIO_CLIENT_INCLUDED
#define ASYNC_TCP_PRIO_CLIENT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncTcpPrioClientBase.h>
#include <AsyncDnsLookup.h>
#include <AsyncTimer.h>
#include <AsyncApplication.h>


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
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2021-06-27

A_detailed_class_description

\include AsyncTcpPrioClient_demo.cpp
*/
template <typename ConT=TcpConnection>
class TcpPrioClient : public ConT, public TcpPrioClientBase
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
    explicit TcpPrioClient(size_t recv_buf_len = ConT::DEFAULT_RECV_BUF_LEN)
      : ConT(recv_buf_len), TcpPrioClientBase(this)
    {
      initialize();
    }

    /**
     * @brief   Disallow copy construction
     */
    TcpPrioClient(const TcpPrioClient&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    TcpPrioClient& operator=(const TcpPrioClient&) = delete;

    /**
     * @brief   Destructor
     */
    virtual ~TcpPrioClient(void) {}

    /**
     * @brief 	Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    virtual void disconnect(void)
    {
      //std::cout << "### TcpPrioClient::disconnect" << std::endl;
      TcpPrioClientBase::disconnect();
    }

  protected:
    using ConT::operator=;
    using TcpPrioClientBase::operator=;

    /**
     * @brief   Disconnect from the remote peer
     *
     * This function is used internally to close the connection to the remote
     * peer.
     */
    virtual void closeConnection(void)
    {
      ConT::closeConnection();
      TcpPrioClientBase::closeConnection();
    }

    /**
     * @brief   Called when a connection has been terminated
     * @param   reason  The reason for the disconnect
     *
     * This function will be called when the connection has been terminated.
     */
    virtual void onDisconnected(TcpConnection::DisconnectReason reason)
    {
      //std::cout << "### TcpPrioClient::onDisconnected:"
      //          << std::endl;
      ConT::onDisconnected(reason);
      TcpPrioClientBase::onDisconnected(reason);
    }

    /**
     * @brief   Allocate a new TcpClient object
     * @return  Returns a new TcpClient object
     *
     * This function is used to allocate a new TcpClient object.  That object
     * is used when in the background trying to connect to a higher prioritized
     * server. Note that the object should be a "normal" TcpClient and not a
     * TcpPrioClient.
     */
    virtual TcpClientBase *newTcpClient(void)
    {
      return new TcpClient<ConT>;
    }

    virtual void emitDisconnected(TcpConnection::DisconnectReason reason)
    {
      ConT::emitDisconnected(reason);
    }

  private:
    TcpPrioClient<ConT>& operator=(TcpClient<ConT>&& other)
    {
      //std::cout << "### TcpPrioClient::operator=(TcpClient<ConT>&&)"
      //          << std::endl;
      *static_cast<TcpClientBase*>(this) =
        std::move(*static_cast<TcpClientBase*>(&other));
      return *this;
    }
};  /* class TcpPrioClient */


} /* namespace Async */

#endif /* ASYNC_TCP_PRIO_CLIENT_INCLUDED */

/*
 * This file has not been truncated
 */
