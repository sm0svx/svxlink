/**
@file	 SigLevDetNoise.h
@brief   A simple signal level detector based on noise measurements
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-07

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

#ifndef SIG_LEV_DET_NOISE_INCLUDED
#define SIG_LEV_DET_NOISE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <list>
#include <set>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>


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
  class SigCAudioSink;
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
@brief	A simple noise measuring signal level detector
@author Tobias Blomberg / SM0SVX
@date   2006-05-07
*/
class SigLevDetNoise : public SigLevDet
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "NOISE";

    /**
     * @brief 	Default constuctor
     */
    explicit SigLevDetNoise(void);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDetNoise(void);
    
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
     * @brief 	Set the detector slope
     * @param 	slope The detector slope to set
     */
    void setDetectorSlope(float slope);

    /**
     * @brief 	Set the detector offset
     * @param 	offset The offset to set
     */
    void setDetectorOffset(float offset);

    /**
     * @brief   Set the level above which the squelch is considered closed
     * @param   The threshold (typically something like 120)
     *
     * This function set an upper limit for the estimated signal level. If the
     * estimation goes over the given threshold, a signal level of 0 will be
     * reported. This can be used as a workaround when using a receiver with
     * squelched audio output. When the squelch is closed the receiver audio
     * is silent. The signal level estimator will interpret this as a very
     * strong signal. Setting up the bogus signal level threshold will
     * counteract this behavior but a better solution is to use unsquelched
     * audio if possible.
     */
    void setBogusThresh(float thresh);
  
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
     * @brief 	Read the latest calculated signal level
     * @return	Returns the latest calculated signal level
     */
    virtual float lastSiglev(void) const;
     
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
    typedef std::multiset<double> SsSet;
    typedef SsSet::const_iterator SsSetIter;
    typedef std::list<SsSetIter>  SsIndexList;

    static const unsigned BLOCK_TIME          = 25;     // milliseconds

    unsigned                  sample_rate;
    unsigned                  block_len;
    Async::AudioFilter	      *filter;
    Async::SigCAudioSink      *sigc_sink;
    float     	      	      slope;
    float     	      	      offset;
    int			      update_interval;
    int			      update_counter;
    unsigned		      integration_time;
    SsSet                     ss_values;
    SsIndexList               ss_idx;
    double                    ss;
    unsigned                  ss_cnt;
    float                     bogus_thresh;
    
    SigLevDetNoise(const SigLevDetNoise&);
    SigLevDetNoise& operator=(const SigLevDetNoise&);
    int processSamples(float *samples, int count);
    
};  /* class SigLevDetNoise */


//} /* namespace */

#endif /* SIG_LEV_DET_NOISE_INCLUDED */



/*
 * This file has not been truncated
 */

