/**
@file   Synchronizer.h
@brief  Create HDLC frames from data bytes
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#ifndef SYNCHRONIZER_INCLUDED
#define SYNCHRONIZER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
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
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

Find the optimal sampling point in the incoming stream of samples to extract
the embedded bitstream. The method used is to track zero crossings and adjust
the sampling point if the zero crossing is too eary or late.
*/
class Synchronizer : public Async::AudioSink, public sigc::trackable
{
  public:
    /**
     * @brief 	Constuctor
     * @param   baudrate    The baudrate of the symbol stream
     * @param   sample_rate The sample rate of the incoming samples
     */
    Synchronizer(unsigned baudrate, unsigned sample_rate=INTERNAL_SAMPLE_RATE);

    /**
     * @brief 	Destructor
     */
    ~Synchronizer(void);

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
    int writeSamples(const float *samples, int len);

    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    void flushSamples(void);

    /**
     * @brief   A signal emitted when new bits have been received
     * @param   A vector of received bits
     */
    sigc::signal<void, std::vector<bool>&> bitsReceived;

  private:
    const unsigned    baudrate;
    const unsigned    sample_rate;
    const unsigned    shift_pos;
    unsigned          pos;
    std::vector<bool> bitbuf;
    bool              was_mark;
    bool              last_stored_was_mark;
    int               err;

    Synchronizer(const Synchronizer&);
    Synchronizer& operator=(const Synchronizer&);

};  /* class Synchronizer */


//} /* namespace */

#endif /* SYNCHRONIZER_INCLUDED */


/*
 * This file has not been truncated
 */
