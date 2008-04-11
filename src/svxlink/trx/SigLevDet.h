/**
@file	 SigLevDet.h
@brief   A simple signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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
@brief	A simple signal level detector
@author Tobias Blomberg / SM0SVX
@date   2006-05-07
*/
class SigLevDet : public SigC::Object, public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit SigLevDet(int sample_rate);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDet(void);
    
    /**
     * @brief 	Set the detector slope
     * @param 	slope The detector slope to set
     */
    void setDetectorSlope(float slope) { this->slope = slope; }

    /**
     * @brief 	Set the detector offset
     * @param 	offset The offset to set
     */
    void setDetectorOffset(float offset) { this->offset = offset; }
  
    /**
     * @brief 	Read the latest calculated signal level
     * @return	Returns the latest calculated signal level
     */
    double lastSiglev(void) const
    {
      return offset - slope * log10(last_siglev);
    }
     
    void reset(void);
     
    
  protected:
    
  private:
    Async::AudioFilter	  *filter;
    Async::SigCAudioSink  *sigc_sink;
    double    	      	  last_siglev;
    float     	      	  slope;
    float     	      	  offset;
    
    SigLevDet(const SigLevDet&);
    SigLevDet& operator=(const SigLevDet&);
    int processSamples(float *samples, int count);
    
};  /* class SigLevDet */


//} /* namespace */

#endif /* SIG_LEV_DET_INCLUDED */



/*
 * This file has not been truncated
 */

