/**
@file	 MultiTx.h
@brief   Make it possible to use multiple transmitters for one logic
@author  Tobias Blomberg / SM0SVX
@date	 2008-07-08

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#ifndef MULTI_TX_INCLUDED
#define MULTI_TX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <list>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



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

namespace Async
{
  class Config;
  class AudioSplitter;
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
@brief	Make it possible to use multiple transmitters for one logic
@author Tobias Blomberg / SM0SVX
@date   2008-07-08

This class make it possible to configure multiple transmitters for one logic.
*/
class MultiTx : public Tx
{
  public:
    /**
     * @brief 	Default constuctor
     */
    MultiTx(Async::Config& cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~MultiTx(void);
  
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
    //virtual bool isTransmitting(void) const;
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    virtual void enableCtcss(bool enable);
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     * @param   duration The tone duration in milliseconds
     */
    virtual void sendDtmf(const std::string& digits, unsigned duration);
    
    /**
     * @brief   Set the signal level value that should be transmitted
     * @param   siglev The signal level to transmit
     * @param   rx_id  The id of the receiver that received the signal
     *
     * This function does not set the output power of the transmitter but
     * instead sets a signal level value that is transmitted with the
     * transmission if the specific Tx object supports it. This can be used
     * on a link transmitter to transport signal level measurements to the
     * link receiver.
     */
    virtual void setTransmittedSignalStrength(char rx_id, float siglev);

    /**
     * @brief 	Send a data frame
     * @param 	msg The frame data
     */
    virtual void sendData(const std::vector<uint8_t> &msg);
    
  protected:
    
  private:
    Async::Config     	  &cfg;
    std::list<Tx *>   	  txs;
    Async::AudioSplitter  *splitter;
    
    MultiTx(const MultiTx&);
    MultiTx& operator=(const MultiTx&);
    void onTransmitterStateChange(void);
    
};  /* class MultiTx */


//} /* namespace */

#endif /* MULTI_TX_INCLUDED */



/*
 * This file has not been truncated
 */

