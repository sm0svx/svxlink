/**
@file   AsyncAudioContainerWav.h
@brief  Handle WAV type audio container
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

#ifndef ASYNC_AUDIO_CONTAINER_WAV_INCLUDED
#define ASYNC_AUDIO_CONTAINER_WAV_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <cstring>
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
@brief  Handle a WAV type audio container
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

This is an implementation of a WAV audio container. An audio container is
a format for storing audio to file or stream it over a network. Audio
containers are normally created using a factory, via the
Async::createAudioContainer function, but it is possible to create an audio
container directly as well.

\example AsyncAudioContainer_demo.cpp
*/
class AudioContainerWav : public AudioContainer
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char *OBJNAME = "wav";

    //struct Factory : public AudioContainerSpecificFactory<AudioContainerWav>
    //{
    //  Factory(void) : AudioContainerSpecificFactory<AudioContainerWav>("wav") {}
    //};

    /**
     * @brief   Default constructor
     */
    AudioContainerWav(void);

    /**
     * @brief   Destructor
     */
    virtual ~AudioContainerWav(void);

    /**
     * @brief   Retrieve the media type for the audio container
     * @return  Returns a string representing the media type, e.g. audio/wav
     */
    virtual const char* mediaType(void) const { return "audio/wav"; }

    /**
     * @brief   Get the standard filename extension for the audio container
     * @return  Returns a string containing the filename extention, e.g. wav
     */
    virtual const char* filenameExtension(void) const { return "wav"; }

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

    /**
     * @brief   Indicate to the container that realtime operation is desired
     *
     * This function can be called to inidicate to the container that we are
     * writing the audio to a realtime stream rather than to a file. The
     * container can then choose to indicate this in the audio stream if it
     * have that capability.
     */
    virtual void setRealtime(void) { m_realtime = true; }

    /**
     * @brief   Get the size of the header for this container
     * @return  Returns the size of the header
     *
     * This function is normally called directly after opening a file to
     * reserve space for writing the header later, usually as the last thing
     * before closing the file.
     */
    virtual size_t headerSize(void) const { return WAVE_HEADER_SIZE; }

    /**
     * @brief   Get the header data
     * @return  Return a pointer to the header data
     *
     * This function is normally called as the last thing before closing a file
     * in order to write the header to the beginning of the file. This of
     * course requires that space have been reserved as described in the
     * documentation for the headerSize function.
     */
    virtual const char* header(void);

  private:
    static const size_t WAVE_HEADER_SIZE  = 44;
    static const size_t NUM_CHANNELS      = 1;

    char      m_wave_header[WAVE_HEADER_SIZE];
    int       m_sample_rate     = INTERNAL_SAMPLE_RATE;
    size_t    m_samples_written = 0;
    size_t    m_block_size      = sizeof(int16_t)*INTERNAL_SAMPLE_RATE / 10;
    char*     m_block           = nullptr;
    char*     m_block_ptr       = nullptr;
    bool      m_realtime        = false;

    AudioContainerWav(const AudioContainerWav&);
    AudioContainerWav& operator=(const AudioContainerWav&);

    size_t storeBuf(char *ptr, const char* buf, size_t len)
    {
      std::memcpy(ptr, buf, len);
      return len;
    }

    size_t store32bitValue(char *ptr, uint32_t val)
    {
      *ptr++ = val & 0xff;
      val >>= 8;
      *ptr++ = val & 0xff;
      val >>= 8;
      *ptr++ = val & 0xff;
      val >>= 8;
      *ptr++ = val & 0xff;
      return 4;
    }

    size_t store16bitValue(char *ptr, uint16_t val)
    {
      *ptr++ = val & 0xff;
      val >>= 8;
      *ptr++ = val & 0xff;
      return 2;
    }
};  /* class AudioContainerWav */


} /* namespace */

#endif /* ASYNC_AUDIO_CONTAINER_WAV_INCLUDED */

/*
 * This file has not been truncated
 */
