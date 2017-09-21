/**
@file	 AsyncAudioDevice.h
@brief   Base class for handling audio devices
@author  Tobias Blomberg / SM0SVX
@date	 2009-07-18

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2009  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_DEVICE_INCLUDED
#define ASYNC_AUDIO_DEVICE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <stdint.h>

#include <string>
#include <map>
#include <list>


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

class AudioIO;
class FdWatch;


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
@brief	Base class for handling audio devices
@author Tobias Blomberg / SM0SVX
@date   2004-03-20

This class implements a base class for supporting different audio devices. This
class is not intended to be used by the end user of the Async library. It is
used by the Async::AudioIO class, which is the Async API frontend for using
audio in an application.
*/
class AudioDevice : public sigc::trackable
{
  public:
    /**
     * @brief The different modes to open a device in
     */
    typedef enum
    {
      MODE_NONE,  ///< No mode. The same as close
      MODE_RD,	  ///< Read
      MODE_WR,	  ///< Write
      MODE_RDWR   ///< Both read and write
    } Mode;
  
    /**
     * @brief 	Register an AudioIO object with the given device name
     * @param 	dev_designator The name of the audio device
     * @param 	audio_io  A previously created AudioIO object
     * @return	Returns an AudioDevice object associated with the given device
     *
     * This function is used to register an AudioIO object with the given
     * audio device. If an AudioDevice object already exist for the given
     * device, it is returned. If there is no AudioDevice object for the
     * given device, a new one is created.
     */
    static AudioDevice *registerAudioIO(const std::string& dev_designator,
      	    AudioIO *audio_io);
    
    /**
     * @brief 	Unregister a previously registered AudioIO object
     * @param 	audio_io  A previously registered AudioIO object
     */
    static void unregisterAudioIO(AudioIO *audio_io);
    
    /**
     * @brief 	Set the sample rate used when doing future opens
     * @param 	rate  The sampling rate to use
     *
     * Use this function to set the sample rate used when opening audio
     * devices.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setSampleRate(int rate) { sample_rate = rate; }
    
    /**
     * @brief 	Set the blocksize used when opening audio devices
     * @param 	size  The blocksize, in samples per channel, to use
     * @return	Returns the blocksize actually set
     *
     * Use this function to set the block size used when opening audio
     * devices. The block size is the size of the blocks used when reading
     * and writing audio to/from the sound card. Smaller blocks give less
     * delay but could cause choppy audio if the computer is too slow.
     * The blocksize is set as samples per channel. For example, a blocksize
     * of 256 samples at 8kHz sample rate will give a delay of 256/8000 = 32ms.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setBlocksize(int size)
    {
      block_size_hint = size;
    }
    
    /**
     * @brief 	Find out what the read (recording) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    virtual int readBlocksize(void) = 0;

    /**
     * @brief 	Find out what the write (playback) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    virtual int writeBlocksize(void) = 0;
    
    /**
     * @brief 	Set the buffer count used when opening audio devices
     * @param 	count 	The buffer count to use
     * @return	Returns the buffer count actually set
     *
     * Use this function to set the buffer count used when opening audio
     * devices. The buffer count is the maximum number of blocks the driver
     * will buffer when reading and writing audio to/from the sound card.
     * Lower numbers give less delay but could cause choppy audio if the
     * computer is too slow.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setBlockCount(int count)
    {
      block_count_hint = (count <= 0) ? 0 : count;
    }
    
    /**
     * @brief 	Set the number of channels used when doing future opens
     * @param 	channels  The number of channels to use
     *
     * Use this function to set the number of channels used when opening audio
     * devices.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setChannels(int channels)
    {
      AudioDevice::channels = channels;
    }

    
    /**
     * @brief 	Check if the audio device has full duplex capability
     * @return	Returns \em true if the device has full duplex capability
     *	      	or else \em false
     */
    virtual bool isFullDuplexCapable(void) = 0;
    
    /**
     * @brief 	Open the audio device
     * @param 	mode The mode to open the audio device in (See AudioIO::Mode)
     * @return	Returns \em true on success or else \em false
     */
    bool open(Mode mode);
    
    /**
     * @brief 	Close the audio device
     */
    void close(void);
    
    /**
     * @brief 	Get the current operating mode of this audio device
     * @return	Returns the current mode (See AudioIO::Mode)
     */
    Mode mode(void) const { return current_mode; }
    
    /**
     * @brief 	Tell the audio device handler that there are audio to be
     *	      	written in the buffer
     */
    virtual void audioToWriteAvailable(void) = 0;

    /*
     * @brief	Tell the audio device to flush its buffers
     */
    virtual void flushSamples(void) = 0;
    
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
    virtual int samplesToWrite(void) const = 0;
    
    /**
     * @brief 	Return the sample rate
     * @return	Returns the sample rate
     */
    int sampleRate(void) const { return sample_rate; }

    /**
     * @brief   Return the device name
     * @return  Returns the device name
     */
    const std::string& devName(void) const { return dev_name; }
    
    
  protected:
    static int	      	sample_rate;
    static int	      	block_size_hint;
    static int	      	block_count_hint;
    static int	      	channels;

    std::string       	dev_name;
    
    /**
     * @brief 	Constuctor
     * @param 	dev_name  The name of the device to associate this object with
     */
    explicit AudioDevice(const std::string& dev_name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioDevice(void);
    
    /**
     * @brief 	Open the audio device
     * @param 	mode The mode to open the audio device in (See AudioIO::Mode)
     * @return	Returns \em true on success or else \em false
     */
    virtual bool openDevice(Mode mode) = 0;

    /**
     * @brief 	Close the audio device
     */
    virtual void closeDevice(void) = 0;

    void putBlocks(int16_t *buf, int frame_cnt);
    int getBlocks(int16_t *buf, int block_cnt);
    
    
  private:
    static const int  DEFAULT_SAMPLE_RATE = INTERNAL_SAMPLE_RATE;
    static const int  DEFAULT_CHANNELS = 2;
    static const int  DEFAULT_BLOCK_COUNT_HINT = 4;
    static const int  DEFAULT_BLOCK_SIZE_HINT = 256; // Samples/channel/block
    
    static std::map<std::string, AudioDevice*>  devices;
    
    Mode      	      	current_mode;
    int       	      	use_count;
    std::list<AudioIO*> aios;

};  /* class AudioDevice */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_INCLUDED */



/*
 * This file has not been truncated
 */

