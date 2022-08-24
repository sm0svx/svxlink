/**
@file	 SigLevDetSim.h
@brief   A simulated signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2015-10-03

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#ifndef SIG_LEV_DET_SIM_INCLUDED
#define SIG_LEV_DET_SIM_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <deque>


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

#include "SigLevDet.h"


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
@brief	A simulated signal level detector
@author Tobias Blomberg / SM0SVX
@date   2015-10-03

This class implements a simulated signal level detector. It's primary use is
for debugging the application using the signal level detector.
*/
class SigLevDetSim : public SigLevDet
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "SIM";

    /**
     * @brief 	Default constuctor
     */
    explicit SigLevDetSim(void);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDetSim(void);
    
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
    virtual int writeSamples(const float *samples, int count);
    virtual void flushSamples(void) {}


  private:
    static const unsigned BLOCK_LENGTH    = 10;  // 10ms
    static unsigned int   next_seed;

    int	                sample_rate;
    unsigned            block_idx;
    int                 last_siglev;
    unsigned            integration_time;
    std::deque<int>     siglev_values;
    unsigned            update_interval;
    unsigned            update_counter;
    unsigned            siglev_toggle_interval;
    unsigned            siglev_toggle_counter;
    unsigned            siglev_rand_interval;
    unsigned            siglev_rand_counter;
    unsigned            block_size;
    float               siglev_min;
    float               siglev_max;
    float               siglev_default;
    unsigned int        seed;
    
    SigLevDetSim(const SigLevDetSim&);
    SigLevDetSim& operator=(const SigLevDetSim&);
    void randNewSiglev(void);
    void toggleSiglev(void);
    
};  /* class SigLevDetSim */


//} /* namespace */

#endif /* SIG_LEV_DET_SIM_INCLUDED */



/*
 * This file has not been truncated
 */

