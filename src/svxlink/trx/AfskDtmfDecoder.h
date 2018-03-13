/**
@file	 AfskDtmfDecoder.h
@brief   This file contains a class that read DTMF digits from the data stream
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

#ifndef AFSK_DTMF_DECODER_INCLUDED
#define AFSK_DTMF_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <stdint.h>
//#include <sigc++/sigc++.h>


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
 * @brief   This class read DTMF frames from the incoming data stream
 * @author  Tobias Blomberg, SM0SVX
 * @date    2013-05-10
 *
 * This is not precisely a DTMF decoder. It listens to the incoming data
 * for DATA_CMD_DTMF frames, decodes them and emits DTMF signals.
 */
class AfskDtmfDecoder : public DtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param   rx The receiver object that own this DTMF decoder
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    AfskDtmfDecoder(Rx *rx, Async::Config &cfg, const std::string &name);

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
     * @brief 	Tell the DTMF decoder to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }

    /**
     * @brief 	Return the active digit
     * @return	Return the active digit if any or a '?' if none.
     */
    char activeDigit(void) const
    {
      return '?';
    }

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
    virtual int detectionTime(void) const { return 300; }

  private:
    /**
     * @brief   This function is called when a data frame has been received
     *
     * This function will be called when a data frame has been received from
     * the remote station. This will happen if the receiver is a link RX and
     * are using AFSK to communicate data frames.
     */
    void dataReceived(const std::vector<uint8_t> &frame);

};  /* class AfskDtmfDecoder */


//} /* namespace */

#endif /* AFSK_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */
