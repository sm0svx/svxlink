/**
@file	 SigLevDetTone.h
@brief   A signal level detector using tones in the 5.5 to 6.5kHz band
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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


#ifndef SIG_LEV_DET_TONE_INCLUDED
#define SIG_LEV_DET_TONE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <deque>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <CppStdCompat.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class AudioFilter;
};

class Goertzel;


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
@brief	A signal level detector using tones in the 5.5 to 6.4kHz band
@author Tobias Blomberg / SM0SVX
@date   2009-05-23

This is not a signal level detector on its own but rather a transport mechanism
to transfer signal level measurements from a remote receiver site that is
linked in using RF. The remote site should modulate one of ten tones
(5500Hz, 5600Hz, 5700Hz, ..., 6400Hz) on the link signal to indicate the
locally measured signal strength. The received tone is then translated by this
class to a signal level value that can be compared to signal level measurements
from other receivers.
*/
class SigLevDetTone : public SigLevDet
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "TONE";

    /**
     * @brief 	Constuctor
     */
    explicit SigLevDetTone(void);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDetTone(void);
    
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
    virtual void setContinuousUpdateInterval(int interval_ms);
    
    /**
     * @brief	Set the integration time to use
     * @param	time_ms The integration time in milliseconds
     * 
     * This function will set up the integration time for the signal level
     * detector. That is, the detector will build a mean value of the
     * detected signal strengths over the given integration time.
     */
    virtual void setIntegrationTime(int time_ms);

    /**
     * @brief 	Read the latest measured signal level
     * @return	Returns the latest measured signal level
     */
    virtual float lastSiglev(void) const { return last_siglev; }

    /**
     * @brief   Read the integrated siglev value
     * @return  Returns the integrated siglev value
     */
    virtual float siglevIntegrated(void) const;

    /**
     * @brief   Reset the signal level detector
     */
    virtual void reset(void);
    
  protected:
    
  private:
    class HammingWindow;
    
    static CONSTEXPR float    ALPHA         = 0.1f; // IIR filter time constant
    static CONSTEXPR unsigned BLOCK_SIZE    = 100;  // 160Hz bandwidth, 6.25ms
    static CONSTEXPR float    ENERGY_THRESH = 1.0e-6f; // Min tone energy
    static CONSTEXPR float    DET_THRESH    = 0.7f; // Detection threshold

    int	                sample_rate;
    std::vector<int>    tone_siglev_map;
    Goertzel            *det[10];
    unsigned            block_idx;
    int                 last_siglev;
    float               passband_energy;
    Async::AudioFilter  *filter;
    float               prev_peak_to_tot_pwr;
    unsigned            integration_time;
    std::deque<int>     siglev_values;
    int                 update_interval;
    int                 update_counter;
    
    SigLevDetTone(const SigLevDetTone&);
    SigLevDetTone& operator=(const SigLevDetTone&);
    int processSamples(const float *samples, int count);
    
};  /* class SigLevDetTone */


//} /* namespace */

#endif /* SIG_LEV_DET_TONE_INCLUDED */



/*
 * This file has not been truncated
 */

