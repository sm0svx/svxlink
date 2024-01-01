/**
@file	 Rx.h
@brief   The base class for a receiver
@author  Tobias Blomberg
@date	 2004-03-21

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

#ifndef RX_INCLUDED
#define RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <cassert>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Modulation.h>
#include <AsyncConfig.h>
#include <AsyncAudioSource.h>


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

namespace Async
{
  class Timer;
  class Config;
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
@brief	The base class for a receiver
@author Tobias Blomberg
@date   2004-03-21

This class is used as the base class for all receivers.
*/
class Rx : public sigc::trackable, public Async::AudioSource
{
  public:
    /**
     * Muting state for the receiver
     */
    typedef enum
    {
      MUTE_NONE,    //! Nothing muted.
      MUTE_CONTENT, //! Mute only the content (audio, dtmf etc).
      MUTE_ALL      //! Mute everything. Also close the audio device.
    } MuteState;

    static const char ID_UNKNOWN = '?';   //! Unknown RX id

    /**
     * @brief   Translate a mute state to a printable sting
     * @returns Return a printable version of the given mute state
     */
    static std::string muteStateToString(MuteState mute_state);

    /**
     * @brief 	Default constuctor
     */
    explicit Rx(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~Rx(void);

    /**
     * @brief   The config object
     * @returns Returns a reference to the configuration object
     */
    Async::Config& cfg(void) { return m_cfg; }

    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void);
    
    /**
     * @brief 	Return the name of the receiver
     * @return	Return the name of the receiver
     */
    const std::string& name(void) const { return m_name; }
    
    /**
     * @brief 	Set the verbosity level of the receiver
     * @param	verbose Set to \em false to keep the rx from printing things
     */
    virtual void setVerbose(bool verbose) { m_verbose = verbose; }

    /**
     * @brief 	Set the mute state for this receiver
     * @param 	new_mute_state The mute state to set for this receiver
     */
    virtual void setMuteState(MuteState new_mute_state)
    {
      m_mute_state = new_mute_state;
    }

    /**
     * @brief   Get the mute state for this receiver
     * @return  Returns the current mute state for this receiver
     */
    virtual MuteState muteState(void) const { return m_mute_state; }

    /**
     * @brief 	Check the squelch status
     * @return	Return \em true if the squelch is open or else \em false
     */
    bool squelchIsOpen(void) const { return m_sql_open; }

    const std::string& squelchActivityInfo(void) const { return m_sql_info; }

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
    virtual bool addToneDetector(float fq, int bw, float thresh,
      	      	      	      	 int required_duration)
    {
      return false;
    }
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    virtual float signalStrength(void) const { return 0; }
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    virtual char sqlRxId(void) const { return ID_UNKNOWN; }
    
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    virtual void reset(void) = 0;
    
    /**
     * @brief   Find out if the receiver is ready for operation
     * @returns Returns \em true if the receiver is ready for operation
     */
    virtual bool isReady(void) const { return true; }

    /**
     * @brief   Set the receiver frequency
     * @param   fq The frequency in Hz
     */
    virtual void setFq(unsigned fq) {}

    /**
     * @brief   Set the receiver modulation mode
     * @param   mod The modulation to set (@see Modulation::Type)
     */
    virtual void setModulation(Modulation::Type mod) {}

    /**
     * @brief 	A signal that indicates if the squelch is open or not
     * @param 	is_open \em True if the squelch is open or \em false if not
     */
    sigc::signal<void, bool> squelchOpen;
    
    /**
     * @brief 	A signal that is emitted when a DTMF digit has been detected
     * @param 	digit The detected digit (0-9, A-D, *, #)
     * @param 	duration Tone duration in milliseconds
     */
    sigc::signal<void, char, int> dtmfDigitDetected;
    
    /**
     * @brief 	A signal that is emitted when a valid selcall sequence has been
                detected
     * @param 	sequence the selcall sequence
     */
    sigc::signal<void, std::string> selcallSequenceDetected;

    /**
     * @brief 	A signal that is emitted when a previously specified tone has
     *	      	been detected for the specified duration
     * @param 	fq The frequency of the tone
     */
    sigc::signal<void, float> toneDetected;
    
    /**
     * @brief	A signal that is emitted when the signal level is updated
     * @param	siglev The new signal level
     */
    sigc::signal<void, float> signalLevelUpdated;

    /**
     * @brief   A signal that is emitted when digital data have been received
     * @param   frame The data frame that was received
     */
    sigc::signal<void, std::vector<uint8_t>&> dataReceived;
    
    /**
     * @brief	A signal that is emitted to publish a state update event
     * @param	event_name The name of the event
     * @param   msg The state update message
     *
     * This signal is emitted when a receiver wish to publish a state update
     * message. A state update message is a free text message that can be used
     * by subscribers to act on certain state changes within SvxLink. The
     * event name must be unique within SvxLink. The recommended format is
     * <context>:<name>, e.g. Rx:sql_state.
     */
    sigc::signal<void, const std::string&,
                 const std::string&> publishStateEvent;
    
    /**
     * @brief   A signal that is emitted when the ready state changes
     */
    sigc::signal<void> readyStateChanged;

  protected:
    /**
     * @brief   Set the state of the squelch
     * @param   is_open Set to \em true if the squelch is open and to
     *                  \em false if it is closed.
     * @param   info Information about the squelch event
     *
     * This function is used by a receiver implementation to set the state of
     * the squelch, if it's opened or closed. The info argument is used to
     * supply a short text string containing information about why the squelch
     * opened or closed. It may be a signal level or a CTCSS frequency.
     */
    void setSquelchState(bool is_open, const std::string& info="");

    void setAudioSourceHandler(Async::AudioSource* src);

  private:
    std::string         m_name;
    bool                m_verbose;
    bool                m_sql_open;
    Async::Config&      m_cfg;
    Async::Timer*       m_sql_tmo_timer;
    std::string         m_sql_info;
    MuteState           m_mute_state;

    void sqlTimeout(Async::Timer *t);
    
};  /* class Rx */


class RxFactory
{
  public:
    static Rx *createNamedRx(Async::Config& cfg, const std::string& name);

    RxFactory(const std::string &name);
    virtual ~RxFactory(void);
    
  protected:
    virtual Rx *createRx(Async::Config& cfg, const std::string& name) = 0;
  
  private:
    static std::map<std::string, RxFactory*> rx_factories;
    
    std::string m_name;

};  /* class RxFactory */


//} /* namespace */

#endif /* RX_INCLUDED */



/*
 * This file has not been truncated
 */

