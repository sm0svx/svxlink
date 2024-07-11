/**
@file	 AsyncTcpConnection.h
@brief   Contains a class for handling exiting TCP connections
@author  Tobias Blomberg
@date	 2003-04-12

This file contains a class to handle exiting TCP connections
to a remote host. See usage instructions in the class definition.

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

#ifndef ASYNC_TCP_CONNECTION_INCLUDED
#define ASYNC_TCP_CONNECTION_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <stdint.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>

#include <string>
#include <cassert>
#include <cstring>
#include <vector>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncFdWatch.h>
#include <AsyncSslContext.h>
#include <AsyncSslX509.h>


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
@brief	A class for handling exiting TCP connections
@author Tobias Blomberg
@date   2003-12-07

This class is used to handle an existing TCP connection. It is not meant to
be used directly but could be. It it mainly created to handle connections
for Async::TcpClient and Async::TcpServer.

It can also handle SSL/TLS connections. A raw TCP connection can be switched to
be encrypted using the setSslContext() and enableSsl() functions.

The reception buffer size given at construction time or using the
setRecvBufLen() function is an initial value. If during the connection a larger
buffer is needed the size will be automatically increased.
*/
class TcpConnection : virtual public sigc::trackable
{
  public:
    /**
     * @brief Reason code for disconnects
     */
    typedef enum
    { 
      DR_HOST_NOT_FOUND,       ///< The specified host was not found in the DNS
      DR_REMOTE_DISCONNECTED,  ///< The remote host disconnected
      DR_SYSTEM_ERROR,	       ///< A system error occured (check errno)
      DR_ORDERED_DISCONNECT,   ///< Disconnect ordered locally
      DR_PROTOCOL_ERROR,       ///< Protocol error
      DR_SWITCH_PEER,          ///< A better peer was found so reconnecting
      DR_BAD_STATE             ///< The connection ended up in a bad state
    } DisconnectReason;

    /**
     * @brief   A sigc return value accumulator for signals returning bool
     *
     * This sigc accumulator will return \em true if all connected slots return
     * \em true.
     */
    struct if_all_true_acc
    {
      typedef bool result_type;
      template <class I>
      bool operator()(I first, I last)
      {
        bool success = true;
        for (; first != last; first ++)
        {
          success &= *first;
        }
        return success;
      }
    };

    /**
     * @brief The default size of the reception buffer
     */
    static const int DEFAULT_RECV_BUF_LEN = 1024;
    
    /**
     * @brief Translate disconnect reason to a string
     */
    static const char *disconnectReasonStr(DisconnectReason reason);
    
    /**
     * @brief 	Constructor
     * @param 	recv_buf_len  The initial size of the receive buffer
     */
    explicit TcpConnection(size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);
    
    /**
     * @brief 	Constructor
     * @param 	sock  	      The socket for the connection to handle
     * @param 	remote_addr   The remote IP-address of the connection
     * @param 	remote_port   The remote TCP-port of the connection
     * @param 	recv_buf_len  The initial size of the receive buffer
     */
    TcpConnection(int sock, const IpAddress& remote_addr,
      	      	  uint16_t remote_port,
      	      	  size_t recv_buf_len = DEFAULT_RECV_BUF_LEN);
    
    /**
     * @brief 	Destructor
     */
    virtual ~TcpConnection(void);

    /**
     * @brief   Move assignmnt operator
     * @param   other The object to move from
     * @return  Returns this object
     *
     * The move operator move the state of a specified TcpConnection object
     * into this object. After the move, the state of the other object will be
     * the same as if it had just been default constructed.
     */
    virtual TcpConnection& operator=(TcpConnection&& other);

    /**
     * @brief   Set a new receive buffer size
     * @param   recv_buf_len The new receive buffer size in bytes
     *
     * This function will resize the receive buffer to the specified size.
     * If the buffer size is reduced and there are more bytes in the current
     * buffer than can be fitted into the new buffer, the buffer resize
     * request vill be silently ignored.
     */
    void setRecvBufLen(size_t recv_buf_len);

    size_t recvBufLen(void) const { return m_recv_buf.capacity(); }

    /**
     * @brief 	Disconnect from the remote host
     *
     * Call this function to disconnect from the remote host. If already
     * disconnected, nothing will be done. The disconnected signal is not
     * emitted when this function is called
     */
    virtual void disconnect(void) { closeConnection(); }
    
    /**
     * @brief 	Write data to the TCP connection
     * @param 	buf   The buffer containing the data to send
     * @param 	count The number of bytes to send from the buffer
     * @return	Returns the number of bytes written or -1 on failure
     */
    virtual int write(const void *buf, int count);

    /**
     * @brief   Get the local IP address associated with this connection
     * @return  Returns an IP address
     */
    IpAddress localHost(void) const;

    /**
     * @brief   Get the local TCP port associated with this connection
     * @return  Returns a port number
     */
    uint16_t localPort(void) const;

    /**
     * @brief 	Return the IP-address of the remote host
     * @return	Returns the IP-address of the remote host
     *
     * This function returns the IP-address of the remote host.
     */
    const IpAddress& remoteHost(void) const { return remote_addr; }
    
    /**
     * @brief 	Return the remote port used
     * @return	Returns the remote port
     */
    uint16_t remotePort(void) const { return remote_port; }
    
    /**
     * @brief 	Check if the connection is established or not
     * @return	Returns \em true if the connection is established or
     *	      	\em false if the connection is not established
     */
    bool isConnected(void) const { return sock != -1; }

    /**
     * @brief   Check if the connection is idle
     * @return  Returns \em true if the connection is idle
     *
     * A connection being idle means that it is not connected
     */
    bool isIdle(void) const { return sock == -1; }

    /**
     * @brief   Enable or disable TLS for this connection
     * @param   enable Set to \em true to enable
     */
    void enableSsl(bool enable);

    /**
     * @brief   Get the peer certificate associated with this connection
     * @return  Returns the X509 certificate associated with the peer
     *
     * This function is used to retrieve the peer certificate that the peer has
     * sent to us during the setup phase. If no certificate was sent this
     * function will return a null object so checking for that condition can be
     * done by comparing the returned object with nullptr.
     */
    SslX509 sslPeerCertificate(void);

    /**
     */
    Async::SslX509 sslCertificate(void) const;

    /**
     * @brief   Get the result of the certificate verification process
     * @return  Returns the verification result (e.g. X509_V_OK for ok)
     */
    long sslVerifyResult(void) const;

    /**
     * @brief   Set the OpenSSL context to use when setting up the connection
     * @param   ctx The context object to use
     * @param   is_server Set to \em true if this is a server side connection
     *
     * This function should be called prior to calling enableSsl in order to
     * set up a TLS context to use when setting up the connection.
     */
    void setSslContext(SslContext& ctx, bool is_server);

    SslContext* sslContext(void) { return m_ssl_ctx; }

    bool isServer(void) const { return m_ssl_is_server; }

    /**
     * @brief   Get common name for the SSL connection
     * @return  Returns the common name for the associated X509 certificate
     */
    //std::string sslCommonName(void) const;

    /**
     * @brief 	A signal that is emitted when a connection has been terminated
     * @param 	con   	The connection object
     * @param 	reason  The reason for the disconnect
     */
    sigc::signal<void, TcpConnection *, DisconnectReason> disconnected;
    
    /**
     * @brief 	A signal that is emitted when data has been received on the
     *	      	connection
     * @param 	buf   A buffer containg the read data
     * @param 	count The number of bytes in the buffer
     * @return	Return the number of processed bytes
     *
     * This signal is emitted when data has been received on this connection.
     * The buffer will contain the bytes read from the operating system.
     * The slot must return the number of bytes that has been processed. The
     * bytes not processed will be stored in the receive buffer for this class
     * and presented again to the slot when more data arrives. The new data
     * will be appended to the old data.
     */
    sigc::signal<int, TcpConnection *, void *, int> dataReceived;

    /**
     * @brief   A signal that is emitted on SSL/TLS certificate verification
     * @param   con The connection object
     * @param   preverify_ok Is \em true if the OpenSSL verification is ok
     * @param   x509_store_ctx The X509 store context
     *
     * Connect to this signal to be able to tap in to the certificate
     * verification process. All slots that connect to this signal must return
     * true for the verification process to succeed.
     *
     * For more information on the function arguments have a look at the manual
     * page for the OpenSSL function SSL_set_verify().
     */
    sigc::signal<bool, TcpConnection*, bool,
                 X509_STORE_CTX*>::accumulated<if_all_true_acc> verifyPeer;

    /**
     * @brief   A signal that is emitted when the SSL connection is ready
     * @param   con The connection object
     *
     * This signal is emitted when the SSL initialization and handshake has
     * finished after the application has called the enableSsl() function.
     */
    sigc::signal<void, TcpConnection*> sslConnectionReady;

  protected:
    /**
     * @brief 	Setup information about the connection
     * @param 	sock  	      The socket for the connection to handle
     *
     * Use this function to set up the socket for the connection.
     */
    void setSocket(int sock);
    
    /**
     * @brief 	Setup information about the connection
     * @param 	remote_addr   The remote IP-address of the connection
     *
     * Use this function to set up the remote IP-address for the connection.
     */
    void setRemoteAddr(const IpAddress& remote_addr);
    
    /**
     * @brief 	Setup information about the connection
     * @param 	remote_port   The remote TCP-port of the connection
     *
     * Use this function to set up the remote port for the connection.
     */
    void setRemotePort(uint16_t remote_port);
    
    /**
     * @brief 	Return the socket file descriptor
     * @return	Returns the currently used socket file descriptor
     *
     * Use this function to get the socket file descriptor that is currently
     * in use. If it is -1 it has not been set.
     */
    int socket(void) const { return sock; }

    /**
     * @brief   Disconnect from the remote peer
     *
     * This function is used internally to close the connection to the remote
     * peer.
     */
    virtual void closeConnection(void);

    /**
     * @brief 	Called when a connection has been terminated
     * @param 	reason  The reason for the disconnect
     *
     * This function will be called when the connection has been terminated.
     * The default action for this function is to emit the disconnected signal.
     */
    virtual void onDisconnected(DisconnectReason reason)
    {
      emitDisconnected(reason);
    }

    /**
     * @brief 	Called when data has been received on the connection
     * @param 	buf   A buffer containg the read data
     * @param 	count The number of bytes in the buffer
     * @return	Return the number of processed bytes
     *
     * This function is called when data has been received on this connection.
     * The buffer will contain the bytes read from the operating system.
     * The function will return the number of bytes that has been processed. The
     * bytes not processed will be stored in the receive buffer for this class
     * and presented again to the slot when more data arrives. The new data
     * will be appended to the old data.
     * The default action for this function is to emit the dataReceived signal.
     */
    virtual int onDataReceived(void *buf, int count)
    {
      return dataReceived(this, buf, count);
    }

    /**
     * @brief   Emit the disconnected signal
     * @param   reason The reason for the disconnection
     */
    virtual void emitDisconnected(DisconnectReason reason)
    {
      disconnected(this, reason);
    }

    /**
     * @brief   Emit the verifyPeer signal
     * @param   preverify_ok  The basic certificate verifications passed
     * @param   x509_store_ctx  The OpenSSL X509 store context
     * @return  Returns 1 on success or 0 if verification fail
     */
    virtual int emitVerifyPeer(int preverify_ok,
                               X509_STORE_CTX* x509_store_ctx)
    {
      if (verifyPeer.empty())
      {
        return preverify_ok;
      }
      return verifyPeer(this, preverify_ok == 1, x509_store_ctx) ? 1 : 0;
    }

  private:
    friend class TcpClientBase;

    enum SslStatus { SSLSTATUS_OK, SSLSTATUS_WANT_IO, SSLSTATUS_FAIL };
    struct Char
    {
      char value;
      Char(void) noexcept
      {
        // Do nothing to suppress automatic initialization on container resize
        // for m_recv_buf.
        static_assert(sizeof *this == sizeof value, "invalid size");
        static_assert(__alignof *this == __alignof value, "invalid alignment");
      }
    };

    static constexpr const size_t DEFAULT_BUF_SIZE = 1024;

    static std::map<SSL*, TcpConnection*> ssl_con_map;

    IpAddress         remote_addr;
    uint16_t          remote_port         = 0;
    int               sock                = -1;
    FdWatch           rd_watch;
    std::vector<Char> m_recv_buf;
    Async::FdWatch    m_wr_watch;
    std::vector<char> m_write_buf;

    SslContext*       m_ssl_ctx           = nullptr;
    bool              m_ssl_is_server     = false;
    SSL*              m_ssl               = nullptr;
    BIO*              m_ssl_rd_bio        = nullptr; // SSL reads, we write
    BIO*              m_ssl_wr_bio        = nullptr; // SSL writes, we read
    std::vector<char> m_ssl_encrypt_buf;

    static TcpConnection* lookupConnection(SSL* ssl)
    {
      auto it = ssl_con_map.find(ssl);
      return (it != ssl_con_map.end()) ? it->second : nullptr;
    }
    static int sslVerifyCallback(int preverify_ok,
                                 X509_STORE_CTX* x509_store_ctx);

    void recvHandler(FdWatch *watch);
    void addToWriteBuf(const char *buf, size_t len);
    void onWriteSpaceAvailable(Async::FdWatch* w);
    int rawWrite(const void* buf, int count);

    void sslPrintErrors(const char* fname);
    SslStatus sslGetStatus(int n);
    int sslRecvHandler(char* src, int count);
    SslStatus sslDoHandshake(void);
    int sslEncrypt(void);
    int sslWrite(const void* buf, int count);

};  /* class TcpConnection */


} /* namespace */

#endif /* ASYNC_TCP_CONNECTION_INCLUDED */



/*
 * This file has not been truncated
 */

