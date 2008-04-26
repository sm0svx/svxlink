/**
@file	 NetUplink.h
@brief   Contains a class the implements a remote transceiver uplink via IP
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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


#ifndef NET_UPLINK_INCLUDED
#define NET_UPLINK_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTcpConnection.h>
#include <NetTrxMsg.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Uplink.h"



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class TcpServer;
  class SigCAudioSink;
  class SigCAudioSource;
  class AudioFifo;
};

namespace NetTrxMsg
{
  class Msg;
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
@brief	Implements a remote transceiver uplink via an IP network
@author Tobias Blomberg / SM0SVX
@date   2006-04-14

This class implements a remote transceiver uplink via an IP network.
*/
class NetUplink : public Uplink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    NetUplink(Async::Config &cfg, const std::string &name, Rx *rx, Tx *tx,
      	      const std::string& port_str = NET_TRX_DEFAULT_TCP_PORT);
  
    /**
     * @brief 	Destructor
     */
    ~NetUplink(void);
  
    /**
     * @brief 	Initialize the uplink
     * @return	Return \em true on success or \em false on failure
     */
    bool initialize(void);
    

  protected:
    
  private:
    Async::TcpServer  	    *server;
    Async::TcpConnection    *con;
    char      	      	    recv_buf[4096];
    int       	      	    recv_cnt;
    int       	      	    recv_exp;
    Rx	      	      	    *rx;
    Async::SigCAudioSink    *sigc_sink;
    Tx	      	      	    *tx;
    Async::AudioFifo  	    *fifo;
    Async::SigCAudioSource  *sigc_src;
    Async::Config     	    &cfg;
    std::string       	    name;
    
    NetUplink(const NetUplink&);
    NetUplink& operator=(const NetUplink&);
    void clientConnected(Async::TcpConnection *con);
    void clientDisconnected(Async::TcpConnection *con,
      	      	      	    Async::TcpConnection::DisconnectReason reason);
    int tcpDataReceived(Async::TcpConnection *con, void *data, int size);
    void handleMsg(NetTrxMsg::Msg *msg);
    void sendMsg(NetTrxMsg::Msg *msg);

    /**
     * @brief 	Set squelch state to open/closed
     * @param 	is_open Set to \em true if open or \em false if closed
     */
    void squelchOpen(bool is_open);
    
    /**
     * @brief 	Pass on received DTMF digit
     * @param 	digit The received digit
     */
    void dtmfDigitDetected(char digit, int duration);
    
    /**
     * @brief 	Pass on detected tone
     * @param 	tone_fq The frequency of the received tone
     */
    void toneDetected(float tone_fq);
    
    /**
     * @brief 	Pass on received audio
     * @param 	samples The buffer containing the samples
     * @param 	count 	The number of samples in the buffer
     */
    int audioReceived(float *samples, int count);
    
    void txTimeout(void);
    void transmitterStateChange(bool is_transmitting);
    void allSamplesFlushed(void);

};  /* class NetUplink */


//} /* namespace */

#endif /* NET_UPLINK_INCLUDED */



/*
 * This file has not been truncated
 */

