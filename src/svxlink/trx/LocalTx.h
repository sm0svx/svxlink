/**
@file	 LocalTx.h
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that implements a local transmitter.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2019 Tobias Blomberg / SM0SVX

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


#ifndef LOCAL_TX_INCLUDED
#define LOCAL_TX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <vector>


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

#include <RefCountingPty.h>
#include "Tx.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class AudioIO;
  class Timer;
  class SigCAudioSource;
  class AudioSelector;
  class AudioSource;
  class AudioValve;
  class AudioPassthrough;
  class AudioMixer;
};

class DtmfEncoder;
class PttCtrl;
class HdlcFramer;
class AfskModulator;


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

class SineGenerator;
class Ptt;


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
@brief	A class that implements a local transmitter
@author Tobias Blomberg
@date   2004-03-21

This class implements a local transmitter. A local transmitter is connected
directly to the sound card in the PC where the logic core runs.
*/
class LocalTx : public Tx
{
  public:
    /**
     * @brief 	Default constuctor
     */
    LocalTx(Async::Config& cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~LocalTx(void);
  
    /**
     * @brief 	Initialize the transmitter object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
  
    /**
     * @brief 	Set the transmit control mode
     * @param 	mode The mode to use to set the transmitter on or off.
     *
     * Use this function to turn the transmitter on (TX_ON) or off (TX__OFF).
     * There is also a third mode (TX_AUTO) that will automatically turn the
     * transmitter on when there is audio to transmit.
     */
    void setTxCtrlMode(TxCtrlMode mode);
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    void enableCtcss(bool enable);
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     * @param   duration The tone duration in milliseconds
     */
    void sendDtmf(const std::string& digits, unsigned duration=0);

    /**
     * @brief 	Send a data frame
     * @param 	msg The frame data
     */
    void sendData(const std::vector<uint8_t> &msg);
    
    /**
     * @brief   Set the signal level value that should be transmitted
     * @param   rx_id  The id of the RX that the signal was received on
     * @param   siglev The signal level to transmit
     *
     * This function does not set the output power of the transmitter but
     * instead sets a signal level value that is transmitted with the
     * transmission if the specific Tx object supports it. This can be used
     * on a link transmitter to transport signal level measurements to the
     * link receiver.
     */
    void setTransmittedSignalStrength(char rx_id, float siglev);
    
    /**
     * @brief   Set the transmitter frequency
     * @param   fq The frequency in Hz
     */
    void setFq(unsigned fq);

    /**
     * @brief   Set the transmitter modulation mode
     * @param   mod The modulation to set (@see Modulation::Type)
     */
    void setModulation(Modulation::Type mod);

  private:
    Async::Config     	    &cfg;
    Async::AudioIO    	    *audio_io;
    Async::Timer      	    *txtot;
    bool      	      	    tx_timeout_occured;
    int       	      	    tx_timeout;
    SineGenerator     	    *sine_gen;
    bool      	      	    ctcss_enable;
    DtmfEncoder       	    *dtmf_encoder;
    Async::AudioSelector    *selector;
    Async::AudioValve 	    *dtmf_valve;
    Async::AudioMixer       *mixer;
    HdlcFramer              *hdlc_framer;
    AfskModulator           *fsk_mod;
    //Async::AudioValve 	    *fsk_valve;
    Async::AudioPassthrough *input_handler;
    PttCtrl   	      	    *ptt_ctrl;
    Async::AudioValve 	    *audio_valve;
    SineGenerator           *siglev_sine_gen;
    std::vector<int>        tone_siglev_map;
    Async::Timer            *ptt_hangtimer;
    Ptt                     *ptt;
    bool                    fsk_trailer_transmitted;
    char                    last_rx_id;
    bool                    fsk_first_packet_transmitted;
    HdlcFramer              *hdlc_framer_ib;
    AfskModulator           *fsk_mod_ib;
    RefCountingPty          *ctrl_pty;
    bool                    audio_dev_keep_open;
    
    void txTimeoutOccured(Async::Timer *t);
    bool setPtt(bool tx, bool with_hangtime=false);
    void transmit(bool do_transmit);
    void allDtmfDigitsSent(void);
    void pttHangtimeExpired(Async::Timer *t);
    bool preTransmitterStateChange(bool do_transmit);
    void sendFskSiglev(char rxid, uint8_t siglev);
    void sendFskDtmf(const std::string &digits, unsigned duration);

};  /* class LocalTx */


//} /* namespace */

#endif /* LOCAL_TX_INCLUDED */



/*
 * This file has not been truncated
 */

