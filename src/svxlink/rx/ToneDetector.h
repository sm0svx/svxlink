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

#include <sigc++/signal_system.h>

#include <string>


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

extern "C" {
#include "fidlib.h"
};


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
#define FLOATING	double
#define SAMPLE	      	float


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
class ToneDetector : public SigC::Object
{
  public:
    ToneDetector(float tone_hz, int base_N);
    ~ToneDetector(void);
    
    int processSamples(float *buf, int len);
    bool isActivated(void) const { return (det_delay_left == 0); }
    FLOATING value(void) const { return result; }
    float toneFq(void) const { return tone_fq; }
    void setFilter(const std::string &filter_spec);
    void setSnrThresh(float thresh) { snr_thresh = thresh; }
    void reset(void);
    
    SigC::Signal1<void, bool> activated;
    SigC::Signal2<void, ToneDetector*, double>	valueChanged;
    
  protected:
    
  private:
    int       	block_pos;
    int       	is_activated;
    FLOATING  	result;
    float     	tone_fq;
    float     	*block;
    
    FLOATING  	coeff;
    FLOATING  	Q1;
    FLOATING  	Q2;
    FLOATING  	sine;
    FLOATING  	cosine;
    int       	N;
    
    FidFilter 	*ff;
    FidRun    	*ff_run;
    FidFunc   	*ff_func;
    void      	*ff_buf;    
    std::string filter_spec;
    int       	det_delay_left;
    int       	undet_delay_left;
    float     	snr_thresh;
    
    inline void resetGoertzel(void);
    inline void processSample(SAMPLE sample);
    //void getRealImag(FLOATING *realPart, FLOATING *imagPart);
    inline FLOATING getMagnitudeSquared(void);

};  /* class ToneDetector */


//} /* namespace */

#endif /* TONE_DETECTOR_INCLUDED */



/*
 * This file has not been truncated
 */

