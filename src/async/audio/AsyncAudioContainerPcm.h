/**
@file   AsyncAudioContainerPcm.h
@brief  Handle PCM type audio container
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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

/** @example AsyncAudioContainer_demo.cpp
An example of how to use the Async::AudioContainer class
*/

#ifndef ASYNC_AUDIO_CONTAINER_PCM_INCLUDED
#define ASYNC_AUDIO_CONTAINER_PCM_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <cstdint>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioContainer.h>


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
@brief  Handle a PCM type audio container
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

This is an implementation of a PCM audio container. An audio container is
a format for storing audio to file or stream it over a network. Audio
containers are normally created using a factory, via the
Async::createAudioContainer function, but it is possible to create an audio
container directly as well.

The PCM audio container really is not a container but a raw format where the
samples are just stored as is, without any header or other metadata.

\example AsyncAudioContainer_demo.cpp
*/
class AudioContainerPcm : public AudioContainer
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char *OBJNAME = "vnd.svxlink.pcm";

    /**
     * @brief   Default constructor
     */
    AudioContainerPcm(void);

    /**
     * @brief   Destructor
     */
    virtual ~AudioContainerPcm(void);

    /**
     * @brief   Retrieve the media type for the audio container
     * @return  Returns a string representing the media type, e.g. audio/wav
     */
    virtual const char* mediaType(void) const
    {
      return "audio/vnd.svxlink.pcm";
    }

    /**
     * @brief   Get the standard filename extension for the audio container
     * @return  Returns a string containing the filename extention, e.g. wav
     */
    virtual const char* filenameExtension(void) const { return "raw"; }

    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count   The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     */
    virtual int writeSamples(const float *samples, int count);

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    virtual void flushSamples(void);

  private:
    size_t                m_block_size  = INTERNAL_SAMPLE_RATE / 10;
    std::vector<int16_t>  m_block;

    AudioContainerPcm(const AudioContainerPcm&);
    AudioContainerPcm& operator=(const AudioContainerPcm&);

};  /* class AudioContainerPcm */


} /* namespace */

#endif /* ASYNC_AUDIO_CONTAINER_PCM_INCLUDED */

/*
 * This file has not been truncated
 */
