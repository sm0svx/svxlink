/**
@file	 AsyncAudioIO.h
@brief   Contains a class for handling audio input/output to an audio device
@author  Tobias Blomberg
@date	 2003-03-23

This file contains a class for handling audio input and output to an audio
device. See usage instruction in the class documentation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003  Tobias Blomberg

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

#include <sigc++/sigc++.h>

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
class SampleFifo;


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
device. For now, the AudioIO class only works with 16 bit mono samples at 8 Khz
sampling rate. An example usage is shown below.

\include AsyncAudioIO_demo.cpp
*/
class AudioIO
  : public SigC::Object, public Async::AudioSource, public Async::AudioSink
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
     * @brief Default constructor
     */
    AudioIO(const std::string& dev_name);
    
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
     * @brief 	Write samples to the audio device
     * @param 	buf   The buffer that contains the samples to write
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples written on success or else
     *	      	-1 on failure
     */
    //int write(float *buf, int count);
    
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
    
    /*
     * @brief 	Call this method to flush all samples in the buffer
     *
     * This method is used to flush all the samples that are in the buffer.
     * That is, all samples in the buffer will be written to the audio device
     * and when finished, emit the allSamplesFlushed signal.
     */
    //void flushSamples(void);
    
    /*
     * @brief 	Call this method to clear all samples in the buffer
     *
     * This method is used to clear all the samples that are in the buffer.
     * That is, all samples in the buffer will be thrown away. Remaining
     * samples that have already been written to the sound card will be
     * flushed and when finished, the allSamplesFlushed signal is emitted.
     */
    void clearSamples(void);
    
    /*
     * @brief 	Check if the audio device is busy flushing samples
     * @return	Returns \em true if flushing the buffer or else \em false
     */
    bool isFlushing(void) const { return is_flushing; }
    
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
    
    void resumeOutput(void) {}
    
    void allSamplesFlushed(void) {}


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

    
    /**
     * @brief 	A signal that is emitted when a block of audio has been
     *	      	received from the audio device
     * @param 	buf   A buffer containing the read samples
     * @param 	count The number of samples in the buffer
     */
    //SigC::Signal2<int, float *, int> audioRead;

    /**
     * @brief 	A signal that is emitted when the write buffer is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    //SigC::Signal1<void, bool> writeBufferFull;
    
    /**
     * @brief 	This signal is emitted when all samples in the buffer
     *	      	has been flushed.
     *
     * When an application is done writing samples to the audio device it
     * should call the flush-method. When all samples has been flushed
     * from the audio device, this signal is emitted.
     */
    //SigC::Signal0<void> allSamplesFlushed;

            
  protected:
    
  private:
    Mode      	      io_mode;
    AudioDevice       *audio_dev;
    SampleFifo	      *write_fifo;
    SigC::Connection  read_con;
    bool      	      do_flush;
    Async::Timer      *flush_timer;
    bool      	      is_flushing;
    int       	      lead_in_pos;
    float     	      m_gain;
    int       	      sample_rate;

    void audioReadHandler(Async::FdWatch *watch);
    void flushSamplesInDevice(int extra_samples=0);
    void flushDone(Timer *timer);
    
      // Methods accessed by the Async::AudioDevice class
    friend class AudioDevice;
    AudioDevice *device(void) const { return audio_dev; }
    SampleFifo &writeFifo(void) const { return *write_fifo; }
    int readSamples(float *samples, int count);
    bool doFlush(void) const { return do_flush; }
    int audioRead(float *samples, int count);
    void fifoBufferFull(bool is_full);

};  /* class AudioIO */


} /* namespace */

#endif /* ASYNC_AUDIO_IO_INCLUDED */



/*
 * This file has not been truncated
 */

