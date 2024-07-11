/**
@file    AsyncTcpPrioClientBase.h
@brief   Contains a base class for creating prioritized TCP client connections
@author  Tobias Blomberg
@date    2022-02-12

This file contain the base class for creating prioritized TCP connections. See
Async::TcpPrioClient for more information.

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

#ifndef ASYNC_TCP_PRIO_CLIENT_BASE_INCLUDED
#define ASYNC_TCP_PRIO_CLIENT_BASE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClientBase.h>
#include <AsyncDnsLookup.h>
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
@brief  A base class for creating a prio controlled TCP client connection
@author Tobias Blomberg
@date   2022-02-12

This is the base class for creating prioritized TCP connections. See
Async::TcpPrioClient for more information.
*/
class TcpPrioClientBase : public TcpClientBase
{
  public:
    /**
     * @brief   Constructor
     * @param   con The connection object associated with this client
     *
     * The object will be constructed and variables will be initialized but
     * no connection will be created until the connect function
     * (see @ref TcpClient::connect) is called.
     * When using this variant of the constructor the connect method which
     * take host and port must be used.
     */
    explicit TcpPrioClientBase(TcpConnection *con);

    /**
     * @brief   Deleted constructor not making sense in this context
     */
    TcpPrioClientBase(TcpConnection *con, const std::string& remote_host,
                      uint16_t remote_port) = delete;

    /**
     * @brief   Deleted constructor not making sense in this context
     */
    TcpPrioClientBase(TcpConnection *con, const IpAddress& remote_ip,
                      uint16_t remote_port) = delete;

    /**
     * @brief   Destructor
     */
    virtual ~TcpPrioClientBase(void);

    /**
     * @brief   Minimum time between reconnects
     * @param   t Time in milliseconds
     */
    void setReconnectMinTime(unsigned t);

    /**
     * @brief   Maximum time between reconnects
     * @param   t Time in milliseconds
     */
    void setReconnectMaxTime(unsigned t);

    /**
     * @brief   Percent to increase reconnect time with each try
     * @param   p Percent
     */
    void setReconnectBackoffPercent(unsigned p);

    /**
     * @brief   Percent to randomize reconnect time
     * @param   p Percent
     */
    void setReconnectRandomizePercent(unsigned p);

    /**
     * @brief   Use a DNS service resource record for connections
     * @param   srv_name    The name of the service
     * @param   srv_proto   The protocol for the service (e.g. tcp or udp)
     * @param   srv_domain  The domain providing the service
     *
     * This function will set up information for connecting to a remote host
     * using SRV records in the DNS system. SRV records is a more advanced way
     * of looking up host information in the DNS system where it is possible to
     * load balance and prioritize between multiple hosts.
     * Use the connect() function to actually start connecting.
     */
    void setService(const std::string& srv_name, const std::string& srv_proto,
                    const std::string& srv_domain);

    /**
     * @brief   Add a static service resource record
     * @param   ttl     The time-to-live for the record
     * @param   prio    The priority for the record, lower mean higher
     * @param   weight  The weight for records with the same priority
     * @param   port    The network port for the service
     * @param   target  The FQDN of the host where the service is hosted
     *
     * Use this function to add static SRV records. This may be useful if there
     * is no DNS service available or if it does not have SRV record support.
     * It may also be used to add more records to a DNS answer.
     *
     * The added records will survive over the whole lifetime of the DNS object
     * and will be added to all DNS lookups made during that time.
     *
     * If the setService() function have been used to set up the service
     * parameters, the name of the static service records will be the same as
     * for the whole service name. If no service parameters have been set up
     * the name of the records will be set to "static".
     *
     * NOTE: The target should normally be an FQDN but it is also possible to
     * specify an IP address or unqualified hostname.
     *
     * Setting the TTL to zero has a special meaning when combined with real
     * DNS records from a DNS lookup. If any records are found in the DNS the
     * TTL for the static records will be set to the maximum value. If no
     * records are found in the DNS the TTL will be zero.
     */
    void addStaticSRVRecord(DnsResourceRecordSRV::Ttl     ttl,
                            DnsResourceRecordSRV::Prio    prio,
                            DnsResourceRecordSRV::Weight  weight,
                            DnsResourceRecordSRV::Port    port,
                            DnsResourceRecordSRV::Target  target);

    /**
     * @brief   Get the full service name
     * @return  Return the full name of the service
     *
     * This function fill return the full name of the service as set up by the
     * setService() function. If no service has been set up, an empty string
     * will returned.
     */
    const std::string& service(void) const;

    /**
     * @brief   Connect to the remote host
     *
     * This function will initiate a connection to the remote host. The
     * connection must not be written to before the connected signal
     * (see @ref TcpClientBase::connected) has been emitted. If the connection
     * is already established or pending, nothing will be done.
     */
    void connect(void);

    /**
     * @brief   Deleted function not making sense in this context
     */
    void connect(const std::string &remote_host,
                 uint16_t remote_port) = delete;

    /**
     * @brief   Deleted function not making sense in this context
     */
    void connect(const Async::IpAddress& remote_ip,
                 uint16_t remote_port) = delete;

    /**
     * @brief 	Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    virtual void disconnect(void);

    /**
     * @brief   Mark connection as established
     *
     * The application must use this function to mark a connection as
     * established when the application layer deem the connection as
     * successful. It is up to the application to decide this, e.g. after the
     * connection has been authenticated.
     * If a connection has not been marked as established when a disconnection
     * occurs, a new connection will be tried again after the exponential
     * backoff timer has expired.
     * On the other hand, if the connection has been marked as established, a
     * reconnect will be retried after the minimal reconnect delay.
     */
    void markAsEstablished(void);

    /**
     * @brief   Check if a connection has been marked as established
     */
    bool markedAsEstablished(void) const;

    /**
     * @brief   Check if the connection is idle
     * @return  Returns \em true if the connection is idle
     *
     * A connection being idle means that it is not connected nor connecting.
     */
    bool isIdle(void) const;

    /**
     * @brief   Check if connected to the primary server
     */
    bool isPrimary(void) const;

    /**
     * @brief   Inherit the assignment operator from TcpClientBase
     */
    using TcpClientBase::operator=;

  protected:
    /**
     * @brief   Must be called from the inheriting class constructor
     *
     * This function must be called by the inheriting class to initialize this
     * class. That is because this class cannot be initialized until the
     * inheriting class has been initialized, e.g. because this class need to
     * call the pure virtual function newTcpClient that is implemented in the
     * inheriting class.
     */
    void initialize(void);

    /**
     * @brief   Check if the connection has been fully connected
     * @return  Return \em true if the connection was successful
     *
     * This function return true when the connection has been fully
     * established. It will continue to return true even after disconnection
     * and will be reset at the moment when a new connection attempt is made.
     */
    //virtual bool successfulConnect(void) override
    //{
    //  return m_successful_connect && TcpClientBase::successfulConnect();
    //}

    /**
     * @brief   Called when the connection has been established to the server
     *
     * This function may be overridden by inheriting classes to get informed of
     * when a connection has been established. The overriding function should
     * normally call this function.
     */
    virtual void connectionEstablished(void) override;

    /**
     * @brief   Called when a connection has been terminated
     * @param   reason  The reason for the disconnect
     *
     * This function will be called when the connection has been terminated.
     */
    virtual void onDisconnected(TcpConnection::DisconnectReason reason);

    /**
     * @brief   Allocate a new TcpClient object
     * @return  Returns a new TcpClient object
     *
     * This function is used to allocate a new TcpClient object.  That object
     * is used when in the background trying to connect to a higher prioritized
     * server. Note that the object should be a "normal" TcpClient and not a
     * TcpPrioClient.
     */
    virtual TcpClientBase *newTcpClient(void) = 0;

    /**
     * @brief   Emit the disconnected signal
     * @param   reason  The reason for the disconnection
     */
    virtual void emitDisconnected(TcpConnection::DisconnectReason reason) = 0;

  private:
    class Machine;
    Machine*  m_machine             = nullptr;
    //bool      m_successful_connect  = false;

};  /* class TcpPrioClientBase */


} /* namespace */

#endif /* ASYNC_TCP_PRIO_CLIENT_BASE_INCLUDED */



/*
 * This file has not been truncated
 */

