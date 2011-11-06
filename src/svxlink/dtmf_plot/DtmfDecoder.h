/**
@file	 DtmfDecoder.h
@brief   This file contains a class that implements a DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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


#ifndef DTMF_DECODER_INCLUDED
#define DTMF_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <sys/time.h>


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

class DtmfToneDetector;


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

/*
 *----------------------------------------------------------------------------
 * Class:     DtmfDecoder
 * Purpose:   DtmfDecoder class
 * Inherits:  
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */   
class DtmfDecoder : public sigc::trackable, public Async::AudioSink
{
  public:
    /**
     * @brief 	Default constructor
     */
    DtmfDecoder(void);
    
    /**
     * @brief 	Destructor
     */
    ~DtmfDecoder(void);
    
    /**
     * @brief 	Write samples into the DTMF decoder
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the DTMF decoder to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    void flushSamples(void)
    {
      sourceAllSamplesFlushed();
    }
    
    /**
     * @brief 	Return the active digit
     * @return	Return the active digit if any or a '?' if none.
     */
    char activeDigit(void) const
    {
      return digit_activated ? last_detected_digit : '?';
    }

    /*
     * @brief 	A signal that is emitted when a DTMF digit is first detected
     * @param 	digit The detected digit
     */
    sigc::signal<void, char> digitActivated;

    /*
     * @brief 	A signal that is emitted when a DTMF digit is no longer present
     * @param 	digit 	  The detected digit
     * @param 	duration  The time that the digit was active
     */
    sigc::signal<void, char, int> digitDeactivated;
    
    
  protected:
    
  private:
    DtmfToneDetector  *row[4];
    DtmfToneDetector  *col[4];
    int       	      active_row;
    int       	      active_col;
    float     	      sample_buf[512];
    int       	      buffered_samples;
    char      	      last_detected_digit;
    bool      	      digit_activated;
    struct timeval    det_time;

    void checkTones(void);
    void setDigitActive(bool is_active);

};  /* class DtmfDecoder */


//} /* namespace */

#endif /* DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

