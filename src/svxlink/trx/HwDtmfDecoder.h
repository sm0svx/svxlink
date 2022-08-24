/**
@file	 HwDtmfDecoder.h
@brief   This file contains the base class for implementing a hw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-02-06

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


#ifndef HW_DTMF_DECODER_INCLUDED
#define HW_DTMF_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>


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

#include "DtmfDecoder.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
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
 * @brief   This is the base class for implementing a hardware DTMF decoder.
 * @author  Tobias Blomberg, SM0SVX
 * @date    2008-02-06
 */   
class HwDtmfDecoder : public DtmfDecoder
{
  public:
    /**
     * @brief 	Destructor
     */
    virtual ~HwDtmfDecoder(void);
    
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
     * @brief 	Return the active digit
     * @return	Return the active digit if any or a '?' if none.
     */
    virtual char activeDigit(void) const
    {
      return (state != STATE_IDLE) ? last_detected_digit : '?';
    }
    
    
  protected:
    /**
     * @brief 	Default constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     *
     * DtmfDecoder objects are created by calling DtmfDecoder::create.
     */
    HwDtmfDecoder(Async::Config &cfg, const std::string &name);
    
    void digitActive(char digit);
    void digitIdle(void);
    
    
  private:
    typedef enum
    {
      STATE_IDLE, STATE_ACTIVE, STATE_HANG
    } State;
    
    static const int MAX_ACTIVE_TIME = 10;
    
    char      	    last_detected_digit;
    State     	    state;
    struct timeval  det_timestamp;
    Async::Timer    *hang_timer;
    Async::Timer    *timeout_timer;

    void hangtimeExpired(Async::Timer *t);
    void timeout(Async::Timer *t);
    void setIdle(void);
    
};  /* class HwDtmfDecoder */


//} /* namespace */

#endif /* HW_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */
