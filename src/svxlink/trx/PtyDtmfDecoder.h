/**
@file	 PtyDtmfDecoder.h
@brief   This file contains a class that add support for the Pty interface
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2014-03-21

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014  Tobias Blomberg / SM0SVX

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


#ifndef PTY_DTMF_DECODER_INCLUDED
#define PTY_DTMF_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



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

#include "HwDtmfDecoder.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class RefCountingPty;


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
 * @brief   This class add support for the Pty interface board
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2014-03-21
 *
 * This class implements support for an external DTMF decoder interfaced via
 * a PTY device. DTMF digits can be fed into SvxLink from an external script
 * for example. Signalling a DTMF digit is a two step process. One need to
 * indicate both when a digit starts and when it stops so that SvxLink can
 * measure the length of the digit. Digit start is indicated by sending the
 * actual digit (0-9, A-F, *, #). "E" is the same as "*" and "F" is the same
 * as "#". To indicate digit end, send a space character.
 */
class PtyDtmfDecoder : public HwDtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    PtyDtmfDecoder(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Destructor
     */
    virtual ~PtyDtmfDecoder(void);

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
    virtual int writeSamples(const float *samples, int count) { return count; }
    
    /**
     * @brief 	Tell the DTMF decoder to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }
    
    /**
     * @brief   The detection time for this detector
     * @returns Returns the detection time in milliseconds
     *
     * This function will return the time in milliseconds that it will take
     * for the detector to detect a DTMF digit. That is, the time from the
     * moment when the tone is activated until the digitActivated signal is
     * emitted.
     * The time can for example be used in a DTMF muting function.
     */
    virtual int detectionTime(void) const { return 75; }

  protected:

  private:
    RefCountingPty *pty;

    void dataReceived(const void *buf, size_t count);

};  /* class PtyDtmfDecoder */


//} /* namespace */

#endif /* PTY_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

