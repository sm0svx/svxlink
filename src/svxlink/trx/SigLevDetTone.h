/**
@file	 SigLevDetTone.h
@brief   A signal level detector using tones in the 5.5 to 6.5kHz band
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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
@brief	A signal level detector using tones in the 5.5 to 6.5kHz band
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
    /**
     * @brief 	Default constuctor
     */
    SigLevDetTone(void);
  
    /**
     * @brief 	Destructor
     */
    ~SigLevDetTone(void);
    
    /**
     * @brief 	Initialize the signal detector
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(Async::Config &cfg, const std::string& name);
    
    /**
     * @brief 	Read the latest measured signal level
     * @return	Returns the latest measured signal level
     */
    virtual float lastSiglev(void) const { return last_siglev; }

    /**
     * @brief   Reset the signal level detector
     */
    virtual void reset(void);
    
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);
    
    
  protected:
    
  private:
    class MyGoertzel;
    class HammingWindow;
    
    static const unsigned BLOCK_SIZE = 320;

    std::vector<int>  tone_siglev_map;
    HammingWindow     *hwin;
    MyGoertzel        *det[10];
    unsigned          block_idx;
    int               last_siglev;
    
    SigLevDetTone(const SigLevDetTone&);
    SigLevDetTone& operator=(const SigLevDetTone&);
    
};  /* class SigLevDetTone */


//} /* namespace */

#endif /* SIG_LEV_DET_TONE_INCLUDED */



/*
 * This file has not been truncated
 */

