/**
@file	 NetTx.h
@brief   Contains a class that connect to a remote transmitter via IP
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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


#ifndef NET_TX_INCLUDED
#define NET_TX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

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

#include "Tx.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace NetTrxMsg
{
  class Msg;
};

namespace Async
{
  class AudioPacer;
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

class NetTrxTcpClient;


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
@brief	Implements a class that connect to a remote transmitter via IP
@author Tobias Blomberg / SM0SVX
@date   2008-03-09
*/
class NetTx : public Tx
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg   The configuration object to use
     * @param 	name  The name of the configuration section to use
     */
    NetTx(Async::Config& cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~NetTx(void);
  
    /**
     * @brief 	Initialize the transmitter object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void);
  
    /**
     * @brief 	Set the transmit control mode
     * @param 	mode The mode to use to set the transmitter on or off.
     *
     * Use this function to turn the transmitter on (TX_ON) or off (TX__OFF).
     * There is also a third mode (TX_AUTO) that will automatically turn the
     * transmitter on when there is audio to transmit.
     */
    virtual void setTxCtrlMode(TxCtrlMode mode);
    
    /**
     * @brief 	Check if the transmitter is transmitting
     * @return	Return \em true if transmitting or else \em false
     */
    virtual bool isTransmitting(void) const;
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    virtual void enableCtcss(bool enable);
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     */
    virtual void sendDtmf(const std::string& digits);
    
    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int sendAudio(float *samples, int count);

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void sendFlush(void);
    
  protected:

  private:
    Async::Config     	&cfg;
    std::string       	name;
    NetTrxTcpClient   	*tcp_con;
    bool      	      	is_transmitting;
    Tx::TxCtrlMode    	mode;
    bool      	      	ctcss_enable;
    Async::AudioPacer 	*pacer;
    
    void tcpConnected(void);
    void tcpDisconnected(Async::TcpConnection *con,
      	      	      	 Async::TcpConnection::DisconnectReason reason);
    void handleMsg(NetTrxMsg::Msg *msg);
    void sendMsg(NetTrxMsg::Msg *msg);

};  /* class NetTx */


//} /* namespace */

#endif /* NET_TX_INCLUDED */



/*
 * This file has not been truncated
 */

