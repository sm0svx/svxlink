/**
@file	 LocalTx.h
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that implements a local transmitter.

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

This class implements a local tranmitter. A local tranmitter is connected
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
     * @brief 	Turn the transmitter on or off
     * @param 	do_transmit Set to \em true to transmit or \em false to turn
     *	      	      	    the transmitter off
     */
    void transmit(bool do_transmit);
    
    /**
     * @brief 	Check if the transmitter is transmitting
     * @return	Return \em true if transmitting or else \em false
     */
    bool isTransmitting(void) const { return is_transmitting; }
    
    /**
     * @brief 	Send some audio to the transmitter
     * @param 	samples The buffer of samples to transmit
     * @param 	count 	The number of samples in the supplied buffer
     * @return	Returns the number of samples transmitted
     */
    int transmitAudio(short *samples, int count);
    
    /**
     * @brief 	Return the number of samples left to send
     * @return	Returns the number of samples left to send
     */
    int samplesToWrite(void);
    
    /*
     * @brief 	Call this method to flush all samples in the buffer
     *
     * This method is used to flush all the samples that are in the buffer.
     * That is, all samples in the buffer will be written to the audio device
     * and when finished, emit the allSamplesFlushed signal.
     */
    void flushSamples(void);
    
    /*
     * @brief 	Check if the tx is busy flushing samples
     * @return	Returns \em true if flushing the buffer or else \em false
     */
    bool isFlushing(void) const;
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    void enableCtcss(bool enable);
    
    
  protected:
    
  private:
    std::string       	  name;
    Async::Config     	  &cfg;
    Async::AudioIO    	  *audio_io;
    bool      	      	  is_transmitting;
    Async::Serial     	  *serial;
    Async::Serial::Pin    ptt_pin1;
    bool                  ptt_pin1_rev;
    Async::Serial::Pin    ptt_pin2;
    bool                  ptt_pin2_rev;
    Async::Timer      	  *txtot;
    bool      	      	  tx_timeout_occured;
    int       	      	  tx_timeout;
    int       	      	  tx_delay;
    SineGenerator     	  *sine_gen;
    bool      	      	  ctcss_enable;
    
    void txTimeoutOccured(Async::Timer *t);
    int parsePttPin(const char *str, Async::Serial::Pin &pin, bool &rev);
    bool setPtt(bool tx);

};  /* class LocalTx */


//} /* namespace */

#endif /* LOCAL_TX_INCLUDED */



/*
 * This file has not been truncated
 */

