/**
@file	 EchoLinkProxy.h
@brief   A class implementing the EchoLink Proxy protocol
@author  Tobias Blomberg / SM0SVX
@date	 2013-04-28

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

#ifndef ECHOLINK_PROXY_INCLUDED
#define ECHOLINK_PROXY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
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

namespace EchoLink
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
@brief	Implements the EchoLink Proxy protocol
@author Tobias Blomberg / SM0SVX
@date   2013-04-28

This class implements the EchoLink Proxy protocol. This protocol is used to
wrap all EchoLink protocol connections in one TCP connection. Both the two
UDP connections (port 5198 and 5199) and the TCP connection to the EchoLink
directory server will be wrapped inside the TCP connection to the EchoLink
proxy server. This is of most use when the two UDP ports cannot be forwarded
to your local EchoLink computer for one or the other reason, like when being
on a public network. Instead of your computer listening directly to the two
UDP ports, the EchoLink proxy server will do it for you.
*/
class Proxy : public sigc::trackable
{
  public:
    typedef enum
    {
      STATE_DISCONNECTED, STATE_WAITING_FOR_DIGEST, STATE_CONNECTED
    } ProxyState;

    typedef enum
    {
      TCP_STATE_DISCONNECTED, TCP_STATE_DISCONNECTING, TCP_STATE_CONNECTING,
      TCP_STATE_CONNECTED
    } TcpState;

    static Proxy *instance(void) { return the_instance; }

    /**
     * @brief 	Default constuctor
     */
    Proxy(const std::string &host, uint16_t port, const std::string &callsign,
          const std::string &password);
  
    /**
     * @brief 	Destructor
     */
    ~Proxy(void);
  
    /**
     * @brief 	Connect to the proxy server
     */
    void connect(void);

    /**
     * @brief   Disconnect from the proxy server
     */
    void disconnect(void);
    
    /**
     * @brief   Disconnect from proxy server then connect again after a delay
     */
    void reset(void);

    /**
     * @brief   Open a TCP connection to port 5200 to the specified host
     * @param   remote_ip The remote IP address to open the TCP connection to
     * @return  Returns \em true on success or else \em false.
     *
     * This function is used to initialize the connection to the EchoLink
     * directory server. It will indicate success if the connection request
     * was successfully sent tot he proxy server. Success does not mean that
     * the connection to the directory server has been established. That is
     * indicated by the emission of the tcpConnected signal. If the connection
     * fail, the tcpDisconnected signal will be emitted instead.
     */
    bool tcpOpen(const Async::IpAddress &remote_ip);

    /**
     * @brief   Close an active TCP connection
     * @return  Return \em true on success or else \em false
     *
     * This function is used to close a previously established TCP connection
     * to an EchoLink directory server. The function will indicate success if
     * the disconnect is successfully sent to the EchoLink proxy server.
     * Success does not mean that the connection has been closed. This is
     * indicated by the emission of the tcpDisconnected signal
     * If no connection is established at the moment, the function will
     * indicate success.
     */
    bool tcpClose(void);

    /**
     * @brief   Read back the current TCP connection state
     * @return  Returns the current TCP connection state (@see TcpState)
     */
    TcpState tcpState(void) { return tcp_state; }

    /**
     * @brief   Send TCP data through an established TCP connection
     * @param   data Pointer to a buffer containing the data to send
     * @param   len The size of the data buffer
     * @return  Returns \em true on success or else \em false
     *
     * Use this function to send data to the EchoLink directory server through
     * an already established connection through the proxy server. Before
     * calling this function, tcpOpen should have been called to open the
     * connection.
     */
    bool tcpData(const void *data, unsigned len);

    /**
     * @brief   Send UDP data to the specified remote IP
     * @param   addr The remote IP address to send to
     * @param   data Pointer to a buffer containing the data to send
     * @param   len The size of the data buffer
     * @return  Returns \em true on success or else \em false
     *
     * Use this function to send UDP frames to the specified remote IP address.
     * The UDP frames should contain data, like audio.
     */
    bool udpData(const Async::IpAddress &addr, const void *data, unsigned len);

    /**
     * @brief   Send UDP control data to the specified remote IP
     * @param   addr The remote IP address to send to
     * @param   data Pointer to a buffer containing the data to send
     * @param   len The size of the data buffer
     * @return  Returns \em true on success or else \em false
     *
     * Use this function to send UDP frames to the specified remote IP address.
     * The UDP frames should contain control data.
     */
    bool udpCtrl(const Async::IpAddress &addr, const void *data, unsigned len);

    /**
     * @brief   A signal that is emitted when the proxy is ready for operation
     * @param   is_ready Set to true if the proxy is ready or false if it's not
     *
     * Beofre calling any communication functions in this class one should wait
     * for this signal to be emitted with a \em true argument. The user of this
     * class should first call the connect method and then wait for this
     * signal.
     */
    sigc::signal<void, bool> proxyReady;

    /**
     * @brief   Signal that is emitted when a TCP connection is established
     *
     * This signal will be emitted when a TCP connection to the EchoLink
     * directory server has been established through the proxy server.
     */
    sigc::signal<void> tcpConnected;

    /**
     * @brief   Signal that is emitted when a TCP connection is closed
     *
     * This signal will be emitted when a TCP connection to the EchoLink
     * directory server has been closed.
     */
    sigc::signal<void> tcpDisconnected;

    /**
     * @brief   Signal emitted when TCP data has been received
     * @param   data Pointer to a buffer containing the data to send
     * @param   len The size of the data buffer
     * @return  Return the number of bytes that was processed
     *
     * This signal will be emitted when TCP data has been received from the
     * EchoLink directory server via the proxy. The receiver of the signal
     * must indicate with the return value how many bytes of the received
     * data was processed. Any unprocessed data will be present in the next
     * emission of this signal. The signal will not be emitted again until
     * more data have been received. This behaviour will make it easy to
     * handle the data stream in suitable chunks.
     */
    sigc::signal<int, void*, unsigned> tcpDataReceived;

    /**
     * @brief   Signal emitted when UDP data has been received
     * @param   addr The remote IP address
     * @param   port The remote UDP port number
     * @param   data Pointer to a buffer containing the data to send
     * @param   len The size of the data buffer
     *
     * This signal will be emitted when UDP data, like audio, have been
     * received through the EchoLink proxy server.
     */
    sigc::signal<void, const Async::IpAddress&, uint16_t, void*,
                 unsigned> udpDataReceived;

    /**
     * @brief   Signal emitted when UDP control data has been received
     * @param   addr The remote IP address
     * @param   port The remote UDP port number
     * @param   data Pointer to a buffer containing the data to send
     * @param   len The size of the data buffer
     *
     * This signal will be emitted when UDP control data have been
     * received through the EchoLink proxy server.
     */
    sigc::signal<void, const Async::IpAddress&, uint16_t, void*,
                 unsigned> udpCtrlReceived;

    /**
     * @brief   Signal emitted when the TCP_STATUS proxy message is received
     * @param   status The status word
     *
     * This signal will be emitted when a TCP_STATUS message has been
     * received from the EchoLink proxy server. The user of this class should
     * not need to use this raw protocol message signal since it's easier to
     * use the tcpConnected signal.
     * A status word set to zero will indicate a successful connection. A
     * non zero status word does not mean anything special other than that the
     * connection failed.
     */
    sigc::signal<void, uint32_t> tcpStatusReceived;

    /**
     * @brief   Signal emitted when the TCP_CLOSE proxy message is received
     *
     * This signal will be emitted when a TCP_CLOSE proxy protocol message
     * is received. This signal should normally not be used since it's better
     * to use the tcpDisconnected signal.
     */
    sigc::signal<void> tcpCloseReceived;

  protected:
    
  private:
    typedef enum
    {
      MSG_TYPE_TCP_OPEN=1, MSG_TYPE_TCP_DATA, MSG_TYPE_TCP_CLOSE,
      MSG_TYPE_TCP_STATUS, MSG_TYPE_UDP_DATA, MSG_TYPE_UDP_CONTROL,
      MSG_TYPE_SYSTEM
    } MsgBlockType;

    static const int NONCE_SIZE         = 8;
    static const int MSG_HEADER_SIZE    = 1 + 4 + 4;
    static const int RECONNECT_INTERVAL = 10000;
    static const int CMD_TIMEOUT        = 10000;
    static const int recv_buf_size      = 16384;

    static Proxy *the_instance;

    Async::TcpClient<>  con;
    const std::string   callsign;
    std::string         password;
    ProxyState          state;
    TcpState            tcp_state;
    uint8_t             recv_buf[recv_buf_size];
    int                 recv_buf_cnt;
    Async::Timer        reconnect_timer;
    Async::Timer        cmd_timer;

    Proxy(const Proxy&);
    Proxy& operator=(const Proxy&);
    bool sendMsgBlock(MsgBlockType type,
                      const Async::IpAddress &remote_ip=Async::IpAddress(),
                      const void *data=0, unsigned len=0);
    void onConnected(void);
    int onDataReceived(Async::TcpConnection *con, void *data, int len);
    void onDisconnected(Async::TcpConnection *con,
        Async::TcpClient<>::DisconnectReason reason);
    void disconnectHandler(void);
    int handleAuthentication(const unsigned char *buf, int len);
    int parseProxyMessageBlock(unsigned char *buf, int len);
    void handleProxyMessageBlock(MsgBlockType type,
        const Async::IpAddress &remote_ip, uint32_t len,
        unsigned char *data);
    void handleTcpDataMsg(uint8_t *buf, int len);
    void handleTcpCloseMsg(const uint8_t *buf, int len);
    void handleTcpStatusMsg(const uint8_t *buf, int len);
    void handleUdpDataMsg(const Async::IpAddress &remote_ip, uint8_t *buf,
                          int len);
    void handleUdpCtrlMsg(const Async::IpAddress &remote_ip, uint8_t *buf,
                          int len);
    void handleSystemMsg(const unsigned char *buf, int len);
    void cmdTimeout(void);
    
};  /* class Proxy */


} /* namespace */

#endif /* ECHOLINK_PROXY_INCLUDED */



/*
 * This file has not been truncated
 */
