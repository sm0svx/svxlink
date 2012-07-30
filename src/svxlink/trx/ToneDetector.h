/**
@file	 ToneDetector.h
@brief   A tone detector that use the Goertzel algorithm
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-15

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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


#ifndef TONE_DETECTOR_INCLUDED
#define TONE_DETECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <vector>
#include <cmath>


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



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Macro:   
 * Purpose: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 * Bugs:    
 *----------------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------------
 * Type:    
 * Purpose: 
 * Members: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 *----------------------------------------------------------------------------
 */


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
@brief	A tone detector that use the Goertzel algorithm
@author Tobias Blomberg / SM0SVX
@date   2003-04-15
*/
class ToneDetector : public sigc::trackable, public Async::AudioSink
{
  public:
    ToneDetector(float tone_hz, float width_hz, int det_delay_ms = 0);
    void setDetectionDelay(int delay_ms);
    int detectionDelay(void) const;
    void setGapDelay(int delay_ms);
    int gapDelay(void) const;
    bool isActivated(void) const { return is_activated; }
    float toneFq(void) const { return tone_fq; }
    void setPeakThresh(float thresh) { peak_thresh = exp10f(thresh/10.0f); }
    void reset(void);
    
    virtual int writeSamples(const float *buf, int len);

    sigc::signal<void, bool> activated;
    sigc::signal<void, float> detected;
    
  private:

    typedef struct
    {
      float v2;
      float v3;
      float fac;
    } GoertzelState;

    void postProcess(void);
    void goertzelInit(GoertzelState *s, float freq, int sample_rate);
    float goertzelResult(GoertzelState *s);

    GoertzelState      center;
    GoertzelState      lower;
    GoertzelState      upper;

    float              tone_fq;
    int       	       det_delay;
    int       	       gap_delay;
    int                block_len;
    int                samples_left;
    bool               is_activated;
    bool               last_active;
    float     	       peak_thresh;
    int                stable_count;

    std::vector<float>                 window_table;
    std::vector<float>::const_iterator win;

};  /* class ToneDetector */


//} /* namespace */

#endif /* TONE_DETECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

