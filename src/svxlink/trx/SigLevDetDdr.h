/**
@file	 SigLevDetDdr.h
@brief   A signal level detector measuring power levels using a DDR
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-17

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

#ifndef SIG_LEV_DET_DDR_INCLUDED
#define SIG_LEV_DET_DDR_INCLUDED


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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"
#include "RtlTcp.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class AudioFilter;
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
@brief	A signal level detector measuring power levels using a DDR
@author Tobias Blomberg / SM0SVX
@date   2014-07-17

*/
class SigLevDetDdr : public SigLevDet
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "DDR";

    /**
     * @brief 	Default constuctor
     */
    explicit SigLevDetDdr(void);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDetDdr(void);
    
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
    virtual int writeSamples(const float *samples, int count) { return count; }
    virtual void flushSamples(void) {}


  private:
    static const unsigned BLOCK_LENGTH    = 10;  // 10ms

    int	                sample_rate;
    unsigned            block_idx;
    float               last_siglev;
    unsigned            integration_time;
    std::deque<float>   siglev_values;
    int                 update_interval;
    int                 update_counter;
    double              pwr_sum;
    double     	        slope;
    double     	        offset;
    unsigned            block_size;
    
    SigLevDetDdr(const SigLevDetDdr&);
    SigLevDetDdr& operator=(const SigLevDetDdr&);
    void processSamples(const std::vector<RtlTcp::Sample> &samples);
    
};  /* class SigLevDetDdr */


//} /* namespace */

#endif /* SIG_LEV_DET_DDR_INCLUDED */



/*
 * This file has not been truncated
 */

