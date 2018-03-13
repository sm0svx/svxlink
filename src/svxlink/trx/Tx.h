/**
@file	 Tx.h
@brief   The base class for a transmitter
@author  Tobias Blomberg
@date	 2004-03-21

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2018 Tobias Blomberg / SM0SVX

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

#include <stdint.h>
#include <sigc++/sigc++.h>

#include <string>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Modulation.h>
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
class Tx : public sigc::trackable, public Async::AudioSink
{
  public:
    typedef enum
    {
      TX_OFF, TX_ON, TX_AUTO
    } TxCtrlMode;

    typedef enum
    {
      DATA_CMD_SIGLEV, DATA_CMD_DTMF, DATA_CMD_TONE_DETECTED
    } DataCmd;

    
    /**
     * @brief 	Default constuctor
     * @param   tx_name   The name of the transmitter
     */
    Tx(std::string tx_name)
      : m_name(tx_name), m_verbose(true), m_is_transmitting(false)
    {
    }
  
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
     * @brief 	Return the name of the transmitter
     * @return	Return the name of the transmitter
     */
    const std::string& name(void) const { return m_name; }

    /*
     * @brief 	Set the verbosity level of the transmitter
     * @param	verbose Set to \em false to keep the rx from printing things
     */
    virtual void setVerbose(bool verbose) { m_verbose = verbose; }

    /**
     * @brief   Check if the transmitter is verbose or not
     * @returns Returns \em true if the transmitter is verbose
     */
    virtual bool isVerbose(void) const { return m_verbose; }

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
    virtual bool isTransmitting(void) const { return m_is_transmitting; }
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    virtual void enableCtcss(bool enable) { }
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     * @param   duration The tone duration in milliseconds
     */
    virtual void sendDtmf(const std::string& digits, unsigned duration=0) {}

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
    virtual void setTransmittedSignalStrength(char rx_id, float siglev) {}

    /**
     * @brief 	Send a data frame
     * @param 	msg The frame data
     */
    virtual void sendData(const std::vector<uint8_t> &msg) {}
    
    /**
     * @brief   Set the transmitter frequency
     * @param   fq The frequency in Hz
     */
    virtual void setFq(unsigned fq) {}

    /**
     * @brief   Set the transmitter modulation mode
     * @param   mod The modulation to set (@see Modulation::Type)
     */
    virtual void setModulation(Modulation::Type mod) {}

    /**
     * @brief 	This signal is emitted when the tx timeout timer expires
     *
     * This signal is emitted when the transmitter have been transmitting
     * for too long. This is to prevent the transmitter from transmitting
     * endlessly if an error occurs.
     */
    sigc::signal<void> txTimeout;
    
    /**
     * @brief 	This signal is emitted when the transmitter starts or stops
     *          transmitting
     * @param 	is_transmitting Set to \em true if the transmitter
     *          is transmitting or else \em false.
     */
    sigc::signal<void, bool> transmitterStateChange;

  protected:
    void setIsTransmitting(bool is_transmitting);

  private:
    std::string m_name;
    bool        m_verbose;
    bool        m_is_transmitting;

};  /* class Tx */


/**
 * @brief   An abstract factory class for creating Tx objects
 * @author  Tobias Blomberg
 * @date    2010-05-09
 *
 * This is the base class for a Tx object factory. When adding a new Tx type,
 * a TxFactory must be added along with it so that an instance of that new
 * Tx type can be created using the static method TxFactory::createNamedTx.
 * A typical factory class may look something like this:
 *
 * class MyTxFactory : public TxFactory
 * {
 *   public:
 *     MyTxFactory(void) : TxFactory("My") {}
 *
 *   protected:
 *     Tx *createTx(Config &cfg, const string& name)
 *     {
 *       return new MyTx(cfg, name);
 *     }
 * };
 */
class TxFactory
{
  public:
    /**
     * @brief   Create a new Tx object of a specific type
     * @param   cfg   A previously initialized config object to read data from
     * @param   name  The name of the config section to read data from
     * @returns Returns a newly created Tx object or 0 on failure
     *
     * Use this static function to create a new Tx object. It should be given
     * a config object and a name of the config section to read configuration
     * data from. The configuration section should then contain a config
     * variable TYPE that describe which type of Tx object that should be
     * created.
     */
    static Tx *createNamedTx(Async::Config& cfg, const std::string& name);

    /**
     * @brief Constructor
     * @param name The type name of the Tx object
     */
    TxFactory(const std::string &name);

    /**
     * Destructor
     */
    virtual ~TxFactory(void);
    
  protected:
    /**
     * @brief Virtual method to create the Tx object
     * @param   cfg   A previously initialized config object to read data from
     * @param   name  The name of the config section to read data from
     * @returns Returns a newly created Tx object or 0 on failure
     *
     * This function must be implemented by the inheriting class. It's in this
     * function that the specific Tx object get created.
     */
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

