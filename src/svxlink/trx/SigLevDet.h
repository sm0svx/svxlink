/**
@file	 SigLevDet.h
@brief   The base class for a signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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

#ifndef SIG_LEV_DET_INCLUDED
#define SIG_LEV_DET_INCLUDED


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

#include <AsyncAudioSink.h>
#include <AsyncFactory.h>



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
@brief	The base class for a signal level detector
@author Tobias Blomberg / SM0SVX
@date   2006-05-07

This is the base class for all signal level detectors. The calculated signal
level value is in the range of approximately 0 to 100 where 0 is the weakest
signal strength measurement and where 100 is the strongest. The minimun and
maximum values are approximate so higher and lower values may be reported.
*/
class SigLevDet : public sigc::trackable, public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SigLevDet(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~SigLevDet(void);
    
    /**
     * @brief 	Initialize the signal detector
     * @param   cfg An initialized config object
     * @param   name The name of the config section to read config from
     * @param	sample_rate The rate with which samples enter the detector
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(Async::Config &cfg, const std::string& name,
                            int sample_rate);
    
    /**
     * @brief	Set the interval for continuous updates
     * @param	interval_ms The update interval, in milliseconds, to use.
     * 
     * This function will set up how often the signal level detector will
     * report the signal strength.
     */
    virtual void setContinuousUpdateInterval(int interval_ms) = 0;
    
    /**
     * @brief	Set the integration time to use
     * @param	time_ms The integration time in milliseconds
     * 
     * This function will set up the integration time for the signal level
     * detector. That is, the detector will build a mean value of the
     * detected signal strengths over the given integration time.
     */
    virtual void setIntegrationTime(int time_ms) = 0;
    
    /**
     * @brief 	Read the latest measured signal level
     * @return	Returns the latest measured signal level
     */
    virtual float lastSiglev(void) const = 0;

    /**
     * @brief   Read the integrated siglev value
     * @return  Returns the integrated siglev value
     */
    virtual float siglevIntegrated(void) const = 0;
    
    /**
     * @brief   Read the receiver id for the last signal report
     * @returns Returns the receiver id for the last signal report
     */
    char lastRxId(void) const { return last_rx_id; }

    /**
     * @brief   Reset the signal level detector
     */
    virtual void reset(void) = 0;
    
    /**
     * @brief   Call this function when a data frame has been received
     * @param   frame The data bytes received
     *
     * This function should be called by the owning object that has received a
     * data frame. If the message type is recognized it will be processed buf
     * if it's not recognized nothing will happen.
     */
    virtual void frameReceived(std::vector<uint8_t> frame) {}

    /**
     * @brief	A signal that is emitted when the signal strength is updated
     * @param	siglev The updated siglev measurement
     * 
     * This signal is emitted when the detector have calculated a new signal
     * level measurement. How often the signal is emitted is set up by calling
     * the setCountiuousUpdateInterval function.
     */
    sigc::signal<void, float> signalLevelUpdated;
    
  protected:
    /**
     * @brief   Update the receiver id
     * @param   The receiver id to update with
     *
     * The class inheriting from this class may update the receiver id by
     * calling this function. If the receiver id have been set in the config
     * file, the update request will be ignored.
     */
    void updateRxId(char rx_id);
    
  private:
    typedef std::map<std::string, SigLevDet*> DetMap;

    std::string name;
    bool  force_rx_id;
    char  last_rx_id;

    static DetMap& detMap(void)
    {
      static DetMap det_map;
      return det_map;
    }

    SigLevDet(const SigLevDet&);
    SigLevDet& operator=(const SigLevDet&);

    friend SigLevDet* createSigLevDet(Async::Config& cfg,
                                      const std::string& name);

};  /* class SigLevDet */


/**
 * @brief   Convenience typedef for easier access to the factory members
 *
 * This typedef make it easier to access the members in the Async::Factory
 * class e.g. SigLevDetFactory::validFactories().
 */
typedef Async::Factory<SigLevDet> SigLevDetFactory;


/**
 * @brief   Convenience struct to make specific factory instantiation easier
 *
 * This struct make it easier to create a specific factory for a SigLevDet
 * class. The constant OBJNAME must be declared in the class that the specific
 * factory is for. To instantiate a specific factory is then as easy as:
 *
 *   static SigLevDetSpecificFactory<SigLevDetCtcss> ctcss;
 */
template <class T>
struct SigLevDetSpecificFactory : public Async::SpecificFactory<SigLevDet, T>
{
  SigLevDetSpecificFactory(void)
    : Async::SpecificFactory<SigLevDet, T>(T::OBJNAME) {}
};


/**
 * @brief   Create a named SigLevDet-derived object
 * @param   name The OBJNAME of the class
 * @return  Returns a new SigLevDet or nullptr on failure
 */
SigLevDet* createSigLevDet(Async::Config& cfg, const std::string& name);


//} /* namespace */

#endif /* SIG_LEV_DET_INCLUDED */



/*
 * This file has not been truncated
 */

