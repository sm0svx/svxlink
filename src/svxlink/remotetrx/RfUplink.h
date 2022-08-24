/**
@file	 RfUplink.h
@brief   Uplink type that communicates to the SvxLink core through a transceiver
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-20

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2010  Tobias Blomberg / SM0SVX

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

#ifndef RF_UPLINK_INCLUDED
#define RF_UPLINK_INCLUDED


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

#include <AsyncConfig.h>



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
  class AudioSelector;
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
@brief	Uplink type that communicates to the SvxLink core through a transceiver
@author Tobias Blomberg / SM0SVX
@date   2008-03-20

This class is a simple uplink that just retransmit what comes into the
connected receiver(s).
*/
class RfUplink : public Uplink
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg A previously initialized config object
     * @param 	name The name of the uplink
     * @param 	rx The receiver
     * @param 	tx The transmitter
     */
    RfUplink(Async::Config &cfg, const std::string &name, Rx *rx, Tx *tx);
  
    /**
     * @brief 	Destructor
     */
    ~RfUplink(void);
  
    /**
     * @brief 	Initialize the uplink
     * @return	Return \em true on success or \em false on failure
     */
    virtual bool initialize(void);
        
  protected:
    
  private:
    Async::Config        &cfg;
    std::string          name;
    Rx	      	         *rx;
    Tx	      	         *tx;
    Tx	      	         *uplink_tx;
    Rx	      	         *uplink_rx;
    Async::AudioSelector *tx_audio_sel;
    
    RfUplink(const RfUplink&);
    RfUplink& operator=(const RfUplink&);
    
    void uplinkRxSquelchOpen(bool is_open);
    void uplinkRxDtmfRcvd(char digit, int duration);
    void uplinkRxSignalLevelUpdated(float siglev);
    void rxSquelchOpen(bool is_open);
    void rxSignalLevelUpdated(float siglev);
    void rxDtmfDigitDetected(char digit, int duration);
    void rxToneDetected(float fq);
    void uplinkTxTransmitterStateChange(bool is_transmitting);

};  /* class RfUplink */


//} /* namespace */

#endif /* RF_UPLINK_INCLUDED */



/*
 * This file has not been truncated
 */

