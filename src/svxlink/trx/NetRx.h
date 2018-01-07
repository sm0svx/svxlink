/**
@file	 NetRx.h
@brief   Contains a class that connect to a remote receiver via IP
@author  Tobias Blomberg
@date	 2006-04-14

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#include <sigc++/sigc++.h>

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTcpConnection.h>


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

namespace NetTrxMsg
{
  class Msg;
};

namespace Async
{
  class AudioDecoder;
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
@brief	Implements a class that connect to a remote receiver via IP
@author Tobias Blomberg
@date   2006-04-14
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
     * @brief 	Set the mute state for this receiver
     * @param 	mute_state The mute state to set for this receiver
     */
    void setMuteState(MuteState new_mute_state);
    
    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	thresh The detection threshold in dB SNR
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    bool addToneDetector(float fq, int bw, float thresh, int required_duration);
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    float signalStrength(void) const { return last_signal_strength; }
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    char sqlRxId(void) const { return last_sql_rx_id; }
        
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    void reset(void);
    
    /**
     * @brief   Set the receiver frequency
     * @param   fq The frequency in Hz
     */
    virtual void setFq(unsigned fq);

    /**
     * @brief   Set the receiver modulation mode
     * @param   mod The modulation to set (@see Modulation::Type)
     */
    virtual void setModulation(Modulation::Type mod);

    /**
     * @brief Resume audio output to the sink
     *
     * This function will be called when the registered audio sink is
     * ready to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    //virtual void resumeOutput(void) {}

  protected:

  private:
    Async::Config     	&cfg;
    Rx::MuteState       mute_state;
    NetTrxTcpClient  	*tcp_con;
    bool                log_disconnects_once;
    bool                log_disconnect;
    float     	      	last_signal_strength;
    char       	      	last_sql_rx_id;
    std::list<ToneDet*> tone_detectors;
    bool      	      	unflushed_samples;
    bool      	      	sql_is_open;
    Async::AudioDecoder *audio_dec;
    unsigned            fq;
    Modulation::Type    modulation;
    
    void connectionReady(bool is_ready);
    void handleMsg(NetTrxMsg::Msg *msg);
    void sendMsg(NetTrxMsg::Msg *msg);
    void allEncodedSamplesFlushed(void);

};  /* class NetRx */


//} /* namespace */

#endif /* NET_RX_INCLUDED */



/*
 * This file has not been truncated
 */

