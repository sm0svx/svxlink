/**
@file   AsyncAudioContainer.h
@brief  Base class for audio container handlers
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

#ifndef ASYNC_AUDIO_CONTAINER_INCLUDED
#define ASYNC_AUDIO_CONTAINER_INCLUDED


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

#include <AsyncAudioSink.h>
#include <AsyncFactory.h>


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
@brief  Base class for audio container handlers
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

This is the base class for an audio container. An audio container is a format
for storing audio to file or stream it over a network. Audio containers are
normally created using a factory, via the createAudioContainer function, but it
is possible to create an audio container directly as well.

After creating a new container it should be fed with audio by connecing the
object to an audio source. If writing a file, the first this to do after
opening the file is to reserve space for the file header, which is written at
the end just before closing the file. This is done so since most formats
include metadata about the size of the file in the header.

When the AudioContainer class want to write a block of data it emits the
writeBlock signal so you need to connect to this signal to handle the blocks.

It is up to the application to decide when to close a file. The endStream
function should then be called and then the file header, retrieved using the
header function, can be written at the beginning of the file before closing it.

@example AsyncAudioContainer_demo.cpp
*/
class AudioContainer : public Async::AudioSink
{
  public:
    /**
     * @brief   Default constructor
     */
    AudioContainer(void) {}

    /**
     * @brief   Disable copy constructor
     */
    AudioContainer(const AudioContainer&) = delete;

    /**
     * @brief   Disable assignment operator
     */
    AudioContainer& operator=(const AudioContainer&) = delete;

    /**
     * @brief   Destructor
     */
    virtual ~AudioContainer(void) {}

    /**
     * @brief   Retrieve the media type for the audio container
     * @return  Returns a string representing the media type, e.g. audio/wav
     */
    virtual const char* mediaType(void) const = 0;

    /**
     * @brief   Get the standard filename extension for the audio container
     * @return  Returns a string containing the filename extention, e.g. wav
     */
    virtual const char* filenameExtension(void) const = 0;

    /**
     * @brief   Indicate to the container that realtime operation is desired
     *
     * This function can be called to inidicate to the container that we are
     * writing the audio to a realtime stream rather than to a file. The
     * container can then choose to indicate this in the audio stream if it
     * have that capability.
     */
    virtual void setRealtime(void) {}

    /**
     * @brief   Indicate to the container that the stream has ended
     *
     * This function should be called when the stream has ended. The container
     * can then flush the last samples in any buffers and maybe write an end
     * record if applicable. All this will happen through the writeBlock
     * signal.
     */
    virtual void endStream(void) { flushSamples(); }

    /**
     * @brief   Get the size of the header for this container
     * @return  Returns the size of the header
     *
     * This function is normally called directly after opening a file to
     * reserve space for writing the header later, usually as the last thing
     * before closing the file.
     */
    virtual size_t headerSize(void) const { return 0; }

    /**
     * @brief   Get the header data
     * @return  Return a pointer to the header data
     *
     * This function is normally called as the last thing before closing a file
     * in order to write the header to the beginning of the file. This of
     * course requires that space have been reserved as described in the
     * documentation for the headerSize function.
     */
    virtual const char* header(void) { return nullptr; }

    /**
     * @brief   A signal that is emitted when a block is ready to be written
     * @param   buf Pointer to a buffer containing the data block
     * @param   len The size of the data block
     *
     * This signal is emitted each time that the container want to write data.
     * The application should connect to this signal i order to write the data
     * to file or whatever the destination is.
     */
    sigc::signal<void, const char*, size_t> writeBlock;
};  /* class AudioContainer */


/**
 * @brief   Convenience typedef for easier access to the factory members
 *
 * This typedef make it easier to access the members in the Async::Factory
 * class e.g. AudioContainerFactory::validFactories().
 */
typedef Async::Factory<AudioContainer> AudioContainerFactory;


/**
 * @brief   Convenience struct to make specific factory instantiation easier
 *
 * This struct make it easier to create a specific factory for a AudioContainer
 * class. The constant OBJNAME must be declared in the class that the specific
 * factory is for. To instantiate a specific factory is then as easy as:
 *
 *   static AudioContainerSpecificFactory<AudioContainerWav> wav;
 */
template <class T>
struct AudioContainerSpecificFactory
  : public Async::SpecificFactory<AudioContainer, T>
{
  AudioContainerSpecificFactory(void)
    : Async::SpecificFactory<AudioContainer, T>(T::OBJNAME) {}
};


/**
 * @brief   Create a named AudioContainer-derived object
 * @param   name The OBJNAME of the class
 * @return  Returns a new AudioContainer or nullptr on failure
 */
AudioContainer* createAudioContainer(const std::string& name);


} /* namespace */

#endif /* ASYNC_AUDIO_CONTAINER_INCLUDED */

/*
 * This file has not been truncated
 */
