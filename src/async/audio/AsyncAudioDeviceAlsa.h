/**
@file	 AsyncAudioDeviceAlsa.h
@brief   Handle Alsa audio devices
@author  Tobias Blomberg / SM0SVX
@date	 2009-07-21

Implements the low level interface to an Alsa audio device.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_DEVICE_ALSA_INCLUDED
#define ASYNC_AUDIO_DEVICE_ALSA_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <alsa/asoundlib.h>


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

#include "AsyncAudioDevice.h"


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
@brief	Implements the low level interface to an Alsa audio device
@author Tobias Blomberg / SM0SVX
@date   2009-07-21

This class implements the low level interface to an Alsa audio device. This
class is not intended to be used by the end user of the Async library. It is
used by the Async::AudioIO class, which is the Async API frontend for using
audio in an application.
*/
class AudioDeviceAlsa : public AudioDevice
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	dev_name  The name of the Alsa PCM to associate this object with
     */
    explicit AudioDeviceAlsa(const std::string& dev_name);
  
    /**
     * @brief 	Destructor
     */
    ~AudioDeviceAlsa(void);
  
    /**
     * @brief 	Find out what the read (recording) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    virtual size_t readBlocksize(void);

    /**
     * @brief 	Find out what the write (playback) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    virtual size_t writeBlocksize(void);

    /**
     * @brief 	Check if the audio device has full duplex capability
     * @return	Returns \em true if the device has full duplex capability
     *	      	or else \em false
     */
    virtual bool isFullDuplexCapable(void);
    
    /**
     * @brief 	Tell the audio device handler that there are audio to be
     *	      	written in the buffer
     */
    virtual void audioToWriteAvailable(void);

    /**
     * @brief	Tell the audio device to flush its buffers
     */
    virtual void flushSamples(void);
    
    /**
     * @brief 	Find out how many samples there are in the output buffer
     * @return	Returns the number of samples in the output buffer on
     *          success or -1 on failure.
     *
     * This function can be used to find out how many samples there are
     * in the output buffer at the moment. This can for example be used
     * to find out how long it will take before the output buffer has
     * been flushed.
     */
    virtual int samplesToWrite(void) const;
    
    
  protected:
    /**
     * @brief 	Open the audio device
     * @param 	mode The mode to open the audio device in (See AudioIO::Mode)
     * @return	Returns \em true on success or else \em false
     */
    virtual bool openDevice(Mode mode);

    /**
     * @brief 	Close the audio device
     */
    virtual void closeDevice(void);


  private:
    class       AlsaWatch;
    size_t      play_block_size;
    size_t      play_block_count;
    size_t      rec_block_size;
    size_t      rec_block_count;
    snd_pcm_t   *play_handle;
    snd_pcm_t   *rec_handle;
    AlsaWatch   *play_watch;
    AlsaWatch   *rec_watch;
    bool        duplex;
    bool        zerofill_on_underflow;

    AudioDeviceAlsa(const AudioDeviceAlsa&);
    AudioDeviceAlsa& operator=(const AudioDeviceAlsa&);
    void audioReadHandler(FdWatch *watch, unsigned short revents);
    void writeSpaceAvailable(FdWatch *watch, unsigned short revents);
    bool initParams(snd_pcm_t *pcm_handle);
    bool getBlockAttributes(snd_pcm_t *pcm_handle, size_t &block_size,
                            size_t &period_size);
    bool startPlayback(snd_pcm_t *pcm_handle);
    bool startCapture(snd_pcm_t *pcm_handle);
    
};  /* class AudioDeviceAlsa */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_ALSA_INCLUDED */



/*
 * This file has not been truncated
 */

