/**
@file	 AsyncAudioEncoderNull.h
@brief   A DUMMY audio "encoder" that just throw away audio
@author  Tobias Blomberg / SM0SVX
@date	 2017-10-28

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_ENCODER_DUMMY_INCLUDED
#define ASYNC_AUDIO_ENCODER_DUMMY_INCLUDED


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

#include <AsyncAudioEncoder.h>


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

namespace Async
{


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
@brief	A DUMMY audio "encoder" that just throw away samples
@author Tobias Blomberg / SM0SVX
@date   2017-10-28

This class implements an audio "encoder" that will just throw away samples.
It may be good to use as a placeholder before a real audio encoder has been
selected.
*/
class AudioEncoderDummy : public AudioEncoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioEncoderDummy(void) {}

    /**
     * @brief 	Destructor
     */
    virtual ~AudioEncoderDummy(void) {}

    /**
     * @brief   Get the name of the codec
     * @return  Return the name of the codec
     */
    virtual const char *name(void) const { return "DUMMY"; }

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
    virtual int writeSamples(const float *samples, int count) { return count; }

    /**
     * @brief 	Call this function when all encoded samples have been flushed
     *
     * This DUMMY encoder will just ignore the allSamplesFlushed notification.
     */
    void allEncodedSamplesFlushed(void) {}

    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }

  private:
    AudioEncoderDummy(const AudioEncoderDummy&);
    AudioEncoderDummy& operator=(const AudioEncoderDummy&);

};  /* class AudioEncoderDummy */


} /* namespace */

#endif /* ASYNC_AUDIO_ENCODER_DUMMY_INCLUDED */


/*
 * This file has not been truncated
 */

