/**
@file	 Tx.h
@brief   The base class for a transmitter
@author  Tobias Blomberg
@date	 2004-03-21

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


#ifndef TX_INCLUDED
#define TX_INCLUDED


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
#include <AsyncAudioSink.h>


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
@brief	This is the base class for a transmitter
@author Tobias Blomberg
@date   2004-03-21

This is the base class for transmitters. It is an abstract class so it cannot
be used standalone. It must be inherited from.
*/
class Tx : public SigC::Object, public Async::AudioSink
{
  public:
    typedef enum TxCtrlMode
    {
      TX_OFF, TX_ON, TX_AUTO
    };
    
    /**
     * @brief 	Create the named transmitter object
     * @param 	cfg   The configuration object to use
     * @param 	name  The name of the transmitter configuration section
     * @return	Returns a transmitter object of the specified type on success.
     *          On failure, 0 is returned.
     *
     * This static function is used to create a new transmitter of a certain
     * type. The type is read from the configuration object "cfg" in the
     * configuration section given by parameter "name".
     */
    static Tx *create(Async::Config& cfg, const std::string& name);
    
    /**
     * @brief 	Default constuctor
     */
    explicit Tx(const std::string name) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~Tx(void) {}
  
    /**
     * @brief 	Initialize the transmitter object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void) = 0;
  
    /**
     * @brief 	Set the transmit control mode
     * @param 	mode The mode to use to set the transmitter on or off.
     *
     * Use this function to turn the transmitter on (TX_ON) or off (TX__OFF).
     * There is also a third mode (TX_AUTO) that will automatically turn the
     * transmitter on when there is audio to transmit.
     */
    virtual void setTxCtrlMode(TxCtrlMode mode) = 0;
    
    /**
     * @brief 	Check if the transmitter is transmitting
     * @return	Return \em true if transmitting or else \em false
     */
    virtual bool isTransmitting(void) const = 0;
    
    /**
     * @brief 	Send some audio to the transmitter
     * @param 	samples The buffer of samples to transmit
     * @param 	count 	The number of samples in the supplied buffer
     * @return	Returns the number of samples transmitted
     */
    //virtual int transmitAudio(float *samples, int count) { return count; }
    
    /**
     * @brief 	Call this method to flush all samples in the buffer
     *
     * This method is used to flush all the samples that are in the buffer.
     * That is, all samples in the buffer will be written to the audio device
     * and when finished, emit the allSamplesFlushed signal.
     */
    //virtual void flushSamples(void) = 0;
    
    /**
     * @brief 	Check if the tx is busy flushing samples
     * @return	Returns \em true if flushing the buffer or else \em false
     */
    //virtual bool isFlushing(void) const = 0;
    
    /**
     * @brief 	Check if audio is currently being written
     * @return	Returns \em true if audio is being written, else \em false
     *
     * Use this function to check if audio is being written to the audio
     * device or if all audio has been flushed.
     */
    virtual bool isWritingAudio(void) const = 0;
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    virtual void enableCtcss(bool enable) { }
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     */
    virtual void sendDtmf(const std::string& digits) {}
    
    /**
     * @brief 	A signal that is emitted when the audio transmit buffer full
     *	      	state is set or cleared
     * @param 	is_full Set to \em true to indicate that the transmit buffer
     *	      	      	is full or \em false if the buffer is non-full
     */
    //SigC::Signal1<void, bool> transmitBufferFull;
    
    /**
     * @brief 	This signal is emitted when all samples in the buffer has
     *	      	been transmitted
     */
    //SigC::Signal0<void> allSamplesFlushed;
    
    /**
     * @brief 	This signal is emitted when the tx timeout timer expires
     *
     * This signal is emitted when the transmitter have been transmitting
     * for too long. This is to prevent the transmitter from transmitting
     * endlessly if an error occurs.
     */
    SigC::Signal0<void> txTimeout;
    
    /**
     * @brief 	This signal is emitted when the transmitter starts or stops
     * @param 	is_tranmitting Set to \em true if the transmitter is
     *          transmitting or else \em false.
     */
    SigC::Signal1<void, bool> transmitterStateChange;
    
    
  protected:
    /*
    Async::AudioSource *inputHandler(void) const
    {
      return reinterpret_cast<Async::AudioSource *>(input_handler);
    }
    */
    //virtual void transmit(bool do_transmit) = 0;
    
    
  private:
    //class InputHandler;
    
    //InputHandler *input_handler;
    
    
};  /* class Tx */


//} /* namespace */

#endif /* TX_INCLUDED */



/*
 * This file has not been truncated
 */

