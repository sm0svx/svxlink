/**
@file	 Rx.h
@brief   The base class for a receiver
@author  Tobias Blomberg
@date	 2004-03-21

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

/** @example Template_demo.cpp
An example of how to use the Template class
*/


#ifndef RX_INCLUDED
#define RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

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
class Rx : public SigC::Object, public Async::AudioSource
{
  public:
    //static Rx *create(Async::Config& cfg, const std::string& name);
    
    /**
     * @brief 	Default constuctor
     */
    explicit Rx(const std::string& name)
      : m_name(name), m_verbose(true), m_sql_open(false) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~Rx(void) {}
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void) = 0;
    
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
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    virtual void mute(bool do_mute) = 0;
    
    /**
     * @brief 	Check the squelch status
     * @return	Return \em true if the squelch is open or else \em false
     */
    bool squelchIsOpen(void) const { return m_sql_open; }
    
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
    virtual int sqlRxId(void) const { return 0; }
    
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    virtual void reset(void) = 0;
    
    /**
     * @brief 	A signal that indicates if the squelch is open or not
     * @param 	is_open \em True if the squelch is open or \em false if not
     */
    SigC::Signal1<void, bool> squelchOpen;
    
    /**
     * @brief 	A signal that is emitted when an audio block has been received
     * @param 	samples The buffer where the samples are stored
     * @param 	count 	The number of samples in the buffer
     * @return	The number of samples that the associated slot took care of
     */
    //SigC::Signal2<int, float *, int> audioReceived;
    
    /**
     * @brief 	A signal that is emitted when a DTMF digit has been detected
     * @param 	digit The detected digit (0-9, A-D, *, #)
     * @param 	duration Tone duration in milliseconds
     */
    SigC::Signal2<void, char, int> dtmfDigitDetected;
    
    /**
     * @brief 	A signal that is emitted when a previously specified tone has
     *	      	been detected for the specified duration
     * @param 	fq The frequency of the tone
     */
    SigC::Signal1<void, float> toneDetected;
    
    
  protected:
    /**
     * @brief 	Set the state of the squelch
     * @param	is_open Set to \em true if the squelch is open and to
     *	      	      	\em false if it is closed.
     */
    void setSquelchState(bool is_open);
    
    
  private:
    std::string   m_name;
    bool          m_verbose;
    bool      	  m_sql_open;
    
};  /* class Rx */


class RxFactory
{
  public:
    static Rx *createNamedRx(Async::Config& cfg, const std::string& name);

    RxFactory(const std::string &name);
    
    virtual ~RxFactory(void);
    
    //const std::string &name(void) const { return m_name; }
    
    
  protected:
    virtual Rx *createRx(Async::Config& cfg, const std::string& name) = 0;
  
  private:
    static std::map<std::string, RxFactory*> rx_factories;
    static bool is_initialized;
    
    std::string m_name;
    
};  /* class RxFactory */


//} /* namespace */

#endif /* RX_INCLUDED */



/*
 * This file has not been truncated
 */

