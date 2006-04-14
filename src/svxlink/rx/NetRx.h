/**
@file	 NetRx.h
@brief   Contains a class that connect to a remote receiver via IP
@author  Tobias Blomberg
@date	 2006-04-14

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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


#ifndef NET_RX_INCLUDED
#define NET_RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTcpClient.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace NetRxMsg
{
  class Msg;
};

namespace Async
{
  class Timer;
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

class ToneDet;
  

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
@brief	Implements a class that connect to a remote receiver via IP
@author Tobias Blomberg
@date   2006-04-14

A_detailed_class_description
*/
class NetRx : public Rx
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg   The configuration object to use
     * @param 	name  The name of the configuration section to use
     */
    explicit NetRx(Async::Config& cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~NetRx(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
    
    /**
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    void mute(bool do_mute);
    
    /**
     * @brief 	Check the squelch status
     * @return	Return \em true if the squelch is open or else \em false
     */
    bool squelchIsOpen(void) const { return squelch_open; }
    
    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    bool addToneDetector(int fq, int bw, int required_duration);
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    float signalStrength(void) const { return last_signal_strength; }
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    int sqlRxId(void) const { return last_sql_rx_id; }
        
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    void reset(void);
    
  protected:
    
    
  private:
    bool      	      	is_muted;
    Async::TcpClient  	*tcp_con;
    char      	      	recv_buf[2048];
    int       	      	recv_cnt;
    int       	      	recv_exp;
    bool      	      	squelch_open;
    float     	      	last_signal_strength;
    int       	      	last_sql_rx_id;
    std::list<ToneDet*> tone_detectors;
    bool      	      	is_connected;
    Async::Timer      	*reconnect_timer;
    
    void tcpConnected(void);
    void tcpDisconnected(Async::TcpConnection *con,
      	      	      	 Async::TcpConnection::DisconnectReason reason);
    int tcpDataReceived(Async::TcpConnection *con, void *data, int size);
    void handleMsg(NetRxMsg::Msg *msg);
    void sendMsg(NetRxMsg::Msg *msg);
    void reconnect(Async::Timer *t);

};  /* class NetRx */


//} /* namespace */

#endif /* NET_RX_INCLUDED */



/*
 * This file has not been truncated
 */

