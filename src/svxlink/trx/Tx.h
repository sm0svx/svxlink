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
    typedef enum
    {
      TX_OFF, TX_ON, TX_AUTO
    } TxCtrlMode;
    
    /**
     * @brief 	Default constuctor
     */
    Tx(void) {}
  
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
     * @brief 	This signal is emitted when the tx timeout timer expires
     *
     * This signal is emitted when the transmitter have been transmitting
     * for too long. This is to prevent the transmitter from transmitting
     * endlessly if an error occurs.
     */
    SigC::Signal0<void> txTimeout;
    
    /**
     * @brief 	This signal is emitted when the transmitter starts or stops
     *          transmitting
     * @param 	is_transmitting Set to \em true if the transmitter
     *          is transmitting or else \em false.
     */
    SigC::Signal1<void, bool> transmitterStateChange;
    
};  /* class Tx */


class TxFactory
{
  public:
    static Tx *createNamedTx(Async::Config& cfg, const std::string& name);

    TxFactory(const std::string &name);
    
    virtual ~TxFactory(void);
    
  protected:
    virtual Tx *createTx(Async::Config& cfg, const std::string& name) = 0;
  
  private:
    static std::map<std::string, TxFactory*> tx_factories;
    
    std::string m_name;

};  /* class TxFactory */


//} /* namespace */

#endif /* TX_INCLUDED */



/*
 * This file has not been truncated
 */

