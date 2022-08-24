/**
@file	 AsyncAudioIO.h
@brief   Contains a class for handling audio input/output to an audio device
@author  Tobias Blomberg
@date	 2003-03-23

This file contains a class for handling audio input and output to an audio
device. See usage instruction in the class documentation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg

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

/** @example  AsyncAudioIO_demo.cpp
An example of how to use the Async::AudioIO class
*/



#ifndef ASYNC_AUDIO_IO_INCLUDED
#define ASYNC_AUDIO_IO_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cstdio>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
#include <AsyncTimer.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>


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

class AudioDevice;
class AudioValve;
class AudioFifo;


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
@brief	A class for handling audio input/output to an audio device
@author Tobias Blomberg
@date   2003-03-23

This is a class for handling audio input and output to an audio
device. For now, the AudioIO class only works with 16 bit stereo samples.
An example usage is shown below.

Multiple AudioIO objects can use the same audio device as long as the device
name is exactly the same.

\include AsyncAudioIO_demo.cpp
*/
class AudioIO : public Async::AudioSource, public Async::AudioSink
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
     * @brief 	Set the sample rate used when doing future opens
     * @param 	rate  The sampling rate to use
     *
     * Use this function to set the sample rate used when opening audio
     * devices.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setSampleRate(int rate);
    
    /**
     * @brief 	Set the blocksize used when opening audio devices
     * @param 	size  The blocksize, in samples per channel, to use
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
    static void setBlocksize(size_t size);

    /**
     * @brief 	Find out what the read (recording) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    size_t readBlocksize(void);

    /**
     * @brief 	Find out what the write (playback) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    size_t writeBlocksize(void);
    
    /**
     * @brief 	Set the block count used when opening audio devices
     * @param 	count 	The block count to use
     *
     * Use this function to set the buffer count used when opening audio
     * devices. The buffer count is the maximum number of blocks the driver
     * will buffer when reading and writing audio to/from the sound card.
     * Lower numbers give less delay but could cause choppy audio if the
     * computer is too slow.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setBlockCount(size_t count);
    
    /**
     * @brief 	Set the number of channels used when doing future opens
     * @param 	channels  The number of channels to use
     *
     * Use this function to set the number of channels used when opening audio
     * devices.
     * This is a global setting so all sound cards will be affected. Already
     * opened sound cards will not be affected.
     */
    static void setChannels(size_t channels);
    
    /**
     * @brief Constructor
     * @param dev_name	The name of the device to use
     * @param channel 	The channel number (zero is the first channel)
     */
    AudioIO(const std::string& dev_name, size_t channel);
    
    /**
     * @brief Destructor
     */
    ~AudioIO(void);
  
    /**
     * @brief 	Check if the audio device is capable of full duplex operation
     * @return	Return \em true if the device is capable of full duplex or
     *	        \em false if it is not
     */
    bool isFullDuplexCapable(void);
  
    /**
     * @brief 	Open the audio device in the specified mode
     * @param 	mode The mode to open the audio device in. See
     *	      	Async::AudioIO::Mode for more information
     * @return	Returns \em true on success or else \em false on failure
     */
    bool open(Mode mode);
  
    /**
     * @brief 	Close the adio device
     */
    void close(void);
  
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
    //int samplesToWrite(void) const;
    
    /*
     * @brief 	Call this method to clear all samples in the buffer
     *
     * This method is used to clear all the samples that are in the buffer.
     * That is, all samples in the buffer will be thrown away. Remaining
     * samples that have already been written to the sound card will be
     * flushed and when finished, the allSamplesFlushed signal is emitted.
     */
    //void clearSamples(void);
    
    /*
     * @brief 	Check if the audio device is busy flushing samples
     * @return	Returns \em true if flushing the buffer or else \em false
     */
    //bool isFlushing(void) const { return is_flushing; }
    
    /*
     * @brief 	Find out the current IO mode
     * @return	Returns the current IO mode
     */
    Mode mode(void) const { return io_mode; }
    
    /**
     * @brief 	Set the gain to use
     * @param 	gain  The new gain to set
     *
     * This function will setup the gain to use for this audio stream.
     * The default gain is 1.0, that is no amplification or attenuation.
     * A value < 1.0 will attenuate the audio stream and a value > 1.0
     * will result in an amplification of the audio stream.
     */
    void setGain(float gain) { m_gain = gain; }

    /**
     * @brief 	Return the gain
     * @return	Returns the gain
     */
    float gain(void) const { return m_gain; }
    
    /**
     * @brief 	Return the sample rate
     * @return	Returns the sample rate
     */
    int sampleRate(void) const { return sample_rate; }
    
    /**
     * @brief   Return the audio channel used
     * @return  Returns the audio channel that was given to the constructor
     */
    size_t channel(void) const { return m_channel; }
    
    /**
     * @brief Resume audio output to the sink
     * 
     * This function will be called when the registered audio sink is
     * ready to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    void resumeOutput(void) {}
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    void allSamplesFlushed(void) {}

#if 0
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    void flushSamples(void);
#endif

            
  protected:
    
  private:
    class InputFifo;
    class DelayedFlushAudioReader;
    
    Mode      	      	    io_mode;
    AudioDevice       	    *audio_dev;
    float     	      	    m_gain;
    int       	      	    sample_rate;
    size_t                  m_channel;
    AudioValve              *input_valve;
    InputFifo         	    *input_fifo;
    DelayedFlushAudioReader *audio_reader;

      // Methods accessed by the Async::AudioDevice class
    friend class AudioDevice;
    AudioDevice *device(void) const { return audio_dev; }
    int readSamples(float *samples, int count);
    bool doFlush(void) const;
    bool isIdle(void) const;
    int audioRead(float *samples, int count);
    unsigned samplesAvailable(void);

};  /* class AudioIO */


} /* namespace */

#endif /* ASYNC_AUDIO_IO_INCLUDED */


/*
 * This file has not been truncated
 */

