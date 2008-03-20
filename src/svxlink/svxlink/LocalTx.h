/**
@file	 LocalTx.h
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that implements a local transmitter.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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


#ifndef LOCAL_TX_INCLUDED
#define LOCAL_TX_INCLUDED


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

#include <AsyncSerial.h>


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
  class AudioIO;
  class Timer;
  class SigCAudioSource;
  class AudioSelector;
  class AudioSource;
  class AudioValve;
};

class DtmfEncoder;


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
     * @brief 	Check if the transmitter is transmitting
     * @return	Return \em true if transmitting or else \em false
     */
    bool isTransmitting(void) const { return is_transmitting; }
    
    /**
     * @brief 	Check if audio is currently being written
     * @return	Returns \em true if audio is being written, else \em false
     *
     * Use this function to check if audio is being written to the audio
     * device or if all audio has been flushed.
     */
    bool isWritingAudio(void) const;
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    void enableCtcss(bool enable);
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     */
    void sendDtmf(const std::string& digits);
    
    
  private:
    class InputHandler;
    class PttCtrl;
    
    std::string       	    name;
    Async::Config     	    &cfg;
    Async::AudioIO    	    *audio_io;
    bool      	      	    is_transmitting;
    Async::Serial     	    *serial;
    Async::Serial::Pin      ptt_pin1;
    bool                    ptt_pin1_rev;
    Async::Serial::Pin      ptt_pin2;
    bool                    ptt_pin2_rev;
    Async::Timer      	    *txtot;
    bool      	      	    tx_timeout_occured;
    int       	      	    tx_timeout;
    SineGenerator     	    *sine_gen;
    bool      	      	    ctcss_enable;
    Async::SigCAudioSource  *sigc_src;
    DtmfEncoder       	    *dtmf_encoder;
    Async::AudioSelector    *selector;
    Async::AudioValve 	    *dtmf_valve;
    InputHandler      	    *input_handler;
    PttCtrl   	      	    *ptt_ctrl;
    
    void txTimeoutOccured(Async::Timer *t);
    int parsePttPin(const char *str, Async::Serial::Pin &pin, bool &rev);
    bool setPtt(bool tx);
    void transmit(bool do_transmit);

};  /* class LocalTx */


//} /* namespace */

#endif /* LOCAL_TX_INCLUDED */



/*
 * This file has not been truncated
 */

