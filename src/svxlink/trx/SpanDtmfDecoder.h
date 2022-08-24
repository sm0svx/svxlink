/**
@file	 SpanDtmfDecoder.h
@brief   This file contains a class that implements a sw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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


#ifndef SPAN_DTMF_DECODER_INCLUDED
#define SPAN_DTMF_DECODER_INCLUDED


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

//#include <AsyncAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DtmfDecoder.h"
#include "spandsp_tone_report_func_args.h"


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
 * @brief   This class implements a software DTMF decoder
 * @author  Tobias Blomberg, SM0SVX
 * @date    2007-05-01
 *
 * This class implements a wrapper for the software DTMF decoder
 * implemented in the "spandsp" library.
 */   
class SpanDtmfDecoder : public DtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    SpanDtmfDecoder(Async::Config &cfg, const std::string &name);
    
    /**
     * @brief 	Destructor
     */
    virtual ~SpanDtmfDecoder(void);
    
    /**
     * @brief 	Initialize the DTMF decoder
     * @returns Returns \em true if the initialization was successful or
     *          else \em false.
     *
     * Call this function to initialize the DTMF decoder. It must be called
     * before using it.
     */
    virtual bool initialize(void);
    
    /**
     * @brief 	Write samples into the DTMF decoder
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Return the active digit
     * @return	Return the active digit if any or a '?' if none.
     */
    char activeDigit(void) const
    {
      return (state != STATE_IDLE) ? last_detected_digit : '?';
    }

    
  protected:
    
  private:
    class PrivateData;
    typedef enum
    {
      STATE_IDLE, STATE_ACTIVE, STATE_HANG
    } State;
    
    PrivateData     *p;
    char      	    last_detected_digit;
    int       	    active_sample_cnt;
    int       	    hang_sample_cnt;
    State     	    state;
    
    static void toneReportCb(SPANDSP_TONE_REPORT_FUNC_ARGS);

    void toneReport(int code);

};  /* class SpanDtmfDecoder */


//} /* namespace */

#endif /* SPAN_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

