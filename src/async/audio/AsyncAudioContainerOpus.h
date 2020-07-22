/**
@file   AsyncAudioContainerOpus.h
@brief  Handle Ogg/Opus type audio container
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

#ifndef ASYNC_AUDIO_CONTAINER_OPUS_INCLUDED
#define ASYNC_AUDIO_CONTAINER_OPUS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <ogg/ogg.h>
#include <vector>
#include <cstring>


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

class AudioEncoder;


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
@brief  Handle a Ogg/Opus type audio container
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

This is an implementation of an Ogg/Opus audio container. An audio container is
a format for storing audio to file or stream it over a network. Audio
containers are normally created using a factory, via the
Async::createAudioContainer function, but it is possible to create an audio
container directly as well.

\example AsyncAudioContainer_demo.cpp
*/
class AudioContainerOpus : public AudioContainer
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char *OBJNAME = "opus";

    /**
     * @brief   Default constructor
     */
    AudioContainerOpus(void);

    /**
     * @brief   Destructor
     */
    virtual ~AudioContainerOpus(void);

    /**
     * @brief   Retrieve the media type for the audio container
     * @return  Returns a string representing the media type, e.g. audio/wav
     */
    virtual const char* mediaType(void) const { return "audio/opus"; }

    /**
     * @brief   Get the standard filename extension for the audio container
     * @return  Returns a string containing the filename extention, e.g. wav
     */
    virtual const char* filenameExtension(void) const { return "opus"; }

    /**
     * @brief   Indicate to the container that the stream has ended
     *
     * This function should be called when the stream has ended. The container
     * can then flush the last samples in any buffers and maybe write an end
     * record if applicable. All this will happen through the writeBlock
     * signal.
     */
    virtual void endStream(void);

    /**
     * @brief   Get the size of the header for this container
     * @return  Returns the size of the header
     *
     * This function is normally called directly after opening a file to
     * reserve space for writing the header later, usually as the last thing
     * before closing the file.
     */
    virtual size_t headerSize(void) const { return m_header.size(); }

    /**
     * @brief   Get the header data
     * @return  Return a pointer to the header data
     *
     * This function is normally called as the last thing before closing a file
     * in order to write the header to the beginning of the file. This of
     * course requires that space have been reserved as described in the
     * documentation for the headerSize function.
     */
    virtual const char* header(void) { return m_header.data(); }

  protected:

  private:
    static constexpr const size_t FRAME_SIZE = 20;

    Async::AudioEncoder*          m_enc;
    ogg_stream_state              m_ogg_stream;
    ogg_packet                    m_packet{0};
    //int                           m_ogg_serial      = 1;
    int                           m_pending_packets = 0;
    AudioContainer*               m_container       = nullptr;
    std::vector<char>             m_header;
    std::vector<char>             m_block;

    AudioContainerOpus(const AudioContainerOpus&);
    AudioContainerOpus& operator=(const AudioContainerOpus&);
    void onWriteBlock(const char *buf, size_t len);
    void onWriteEncodedSamples(const void *data, int len);
    void oggpackWriteString(oggpack_buffer* oggbuf,
                            const char *str, int lenbits=32);
    void oggpackWriteCommentList(oggpack_buffer* oggbuf,
                                 const std::vector<const char*> &comments);
    bool writePage(const ogg_page& page, std::vector<char>& buf);
    bool writeOggOpusHeader(void);

};  /* class AudioContainerOpus */


} /* namespace */

#endif /* ASYNC_AUDIO_CONTAINER_OPUS_INCLUDED */

/*
 * This file has not been truncated
 */
