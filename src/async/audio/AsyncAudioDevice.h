/**
@file	 AsyncAudioDevice.h
@brief   Handle OSS audio devices
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-20

Implements the low level interface to an OSS audio device.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

#include <string>
#include <map>
#include <list>
#include <cmath>


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
@brief	A class that implements the low level interface to an OSS audio device
@author Tobias Blomberg / SM0SVX
@date   2004-03-20

This class implements the low level interface to an OSS audio device. This
class is not intended to be used by the end user of the Async library. It is
used by the Async::AudioIO class, which is the Async API frontend for using
audio in an application.
*/
class AudioDevice : public SigC::Object
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
     * @param 	dev_name  The name of the audio device
     * @param 	audio_io  A previously created AudioIO object
     * @return	Returns an AudioDevice object associated with the given device
     *
     * This function is used to register an AudioIO object with the given
     * audio device. If an AudioDevice object already exist for the given
     * device, it is returns. If there is no AudioDevice object for the
     * given device, a new one is created.
     */
    static AudioDevice *registerAudioIO(const std::string& dev_name,
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
    static int setBlocksize(int size)
    {
      size = (size <= 0) ? 1 : size * channels * sizeof(int16_t);
      frag_size_log2 = static_cast<int>(log2(size));
      return blocksize();
    }
    
    /**
     * @brief 	Find out what the blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    static int blocksize(void)
    {
      return (int)std::pow(2.0, (double)frag_size_log2) /
      	     (channels * sizeof(int16_t));
    }
    
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
    static int setBufferCount(int count)
    {
      frag_count = (count <=0) ? 0 : count;
      return frag_count;
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
    bool isFullDuplexCapable(void);
    
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
     * @brief 	Tell the audio device handler that there are audio to be
     *	      	written in the buffer
     */
    void audioToWriteAvailable(void);

    /*
     * @brief	Tell the audio device to flush its buffers
     */
    void flushSamples(void);
    
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
    int samplesToWrite(void) const;
    
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
    /**
     * @brief 	Constuctor
     * @param 	dev_name  The name of the device to associate this object with
     */
    explicit AudioDevice(const std::string& dev_name);
  
    /**
     * @brief 	Destructor
     */
    ~AudioDevice(void);
  
    
  private:
    static const int  DEFAULT_SAMPLE_RATE = INTERNAL_SAMPLE_RATE;
    static const int  DEFAULT_CHANNELS = 2;
    static const int  DEFAULT_FRAG_COUNT = 2;
    static const int  DEFAULT_FRAG_SIZE_LOG2 = 10; // 512 samples
    static const int  BUF_FRAG_COUNT = 4;
    
    static std::map<std::string, AudioDevice*>  devices;
    static int	      	      	      	      	sample_rate;
    static int	      	      	      	      	frag_size_log2;
    static int	      	      	      	      	frag_count;
    static int	      	      	      	      	channels;
    
    std::string       	dev_name;
    int       	      	use_count;
    std::list<AudioIO*> aios;
    Mode      	      	current_mode;
    int       	      	fd;
    FdWatch	      	*read_watch;
    FdWatch	      	*write_watch;
    int16_t      	*read_buf;
    int       	      	device_caps;
    bool      	      	use_trigger;
    //bool		prebuf;
    float     	      	*samples;
    int16_t     	*last_frag;
    bool      	      	use_fillin;
    
    void audioReadHandler(FdWatch *watch);
    void writeSpaceAvailable(FdWatch *watch);
    void closeDevice(void);

};  /* class AudioDevice */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_INCLUDED */



/*
 * This file has not been truncated
 */

