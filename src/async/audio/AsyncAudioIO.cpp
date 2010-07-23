/**
@file	 AsyncAudioIO.cpp
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




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <stdint.h>

#include <cstdio>
#include <cassert>
#include <cerrno>
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

#include "AsyncAudioDevice.h"
#include "AsyncAudioReader.h"
#include "AsyncAudioPassthrough.h"
#include "AsyncAudioValve.h"
#include "AsyncAudioIO.h"
#include "AsyncAudioDebugger.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class Async::AudioIO::DelayedFlushAudioReader
  : public AudioReader, public SigC::Object
{
  public:
    DelayedFlushAudioReader(AudioDevice *audio_dev)
      : audio_dev(audio_dev), flush_timer(0),
        is_idle(true), is_flushing(false)
    {
    }
    
    ~DelayedFlushAudioReader(void)
    {
      delete flush_timer;
    }

    bool isIdle(void) const { return is_idle; }
    bool isFlushing(void) const { return is_flushing; }
    
    virtual int writeSamples(const float *samples, int count)
    {
      is_idle = false;
      is_flushing = false;

      if (flush_timer != 0)
      {
	delete flush_timer;
	flush_timer = 0;
      }

      if ((audio_dev->mode() != AudioDevice::MODE_WR) &&
          (audio_dev->mode() != AudioDevice::MODE_RDWR))
      {
        return count;
      }

      audio_dev->audioToWriteAvailable();
      return AudioReader::writeSamples(samples, count);
    }
    
    virtual void flushSamples(void)
    {
      is_idle = true;
      if ((audio_dev->mode() != AudioDevice::MODE_WR) &&
          (audio_dev->mode() != AudioDevice::MODE_RDWR))
      {
        is_flushing = false;
        sourceAllSamplesFlushed();
        return;
      }

      is_flushing = true;
      audio_dev->flushSamples();

      long flushtime =
              1000 * audio_dev->samplesToWrite() / audio_dev->sampleRate();
      delete flush_timer;
      flush_timer = new Timer(flushtime);
      flush_timer->expired.connect(
      	  slot(*this, &DelayedFlushAudioReader::flushDone));
    }
    
  private:
    AudioDevice *audio_dev;
    Timer     	*flush_timer;
    bool        is_idle;
    bool        is_flushing;
  
    void flushDone(Timer *timer)
    {
      delete flush_timer;
      flush_timer = 0;
      is_flushing = false;
      AudioReader::flushSamples();
    } /* AudioIO::flushDone */

}; /* class Async::AudioIO::DelayedFlushAudioReader */



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

void AudioIO::setSampleRate(int rate)
{
  AudioDevice::setSampleRate(rate);
} /* AudioIO::setSampleRate */


void AudioIO::setBlocksize(int size)
{
  AudioDevice::setBlocksize(size);
} /* AudioIO::setBlocksize */


int AudioIO::blocksize(void)
{
  return audio_dev->blocksize();
} /* AudioIO::blocksize */


void AudioIO::setBlockCount(int count)
{
  AudioDevice::setBlockCount(count);
} /* AudioIO::setBlockCount */


void AudioIO::setChannels(int channels)
{
  return AudioDevice::setChannels(channels);
} /* AudioIO::setBufferCount */



AudioIO::AudioIO(const string& dev_name, int channel)
  : io_mode(MODE_NONE), audio_dev(0), m_gain(1.0),
    m_channel(channel), input_valve(0), audio_reader(0)
{
  audio_dev = AudioDevice::registerAudioIO(dev_name, this);
  if (audio_dev == 0)
  {
    return;
  }
  
  sample_rate = audio_dev->sampleRate();
  
  input_valve = new AudioValve;
  input_valve->setOpen(false);
  AudioSink::setHandler(input_valve);
  
  audio_reader = new DelayedFlushAudioReader(audio_dev);
  input_valve->registerSink(audio_reader, true);
  
  //new AudioDebugger(input_manager);

} /* AudioIO::AudioIO */


AudioIO::~AudioIO(void)
{
  close();
  AudioSink::clearHandler();
  delete input_valve;
  AudioDevice::unregisterAudioIO(this);
} /* AudioIO::~AudioIO */


bool AudioIO::isFullDuplexCapable(void)
{
  return audio_dev->isFullDuplexCapable();
} /* AudioIO::isFullDuplexCapable */


bool AudioIO::open(Mode mode)
{
  if (audio_dev == 0)
  {
    return false;
  }
  
  if (mode == io_mode)
  {
    return true;
  }
  
  close();
  
  if (mode == MODE_NONE)
  {
    return true;
  }
  
    /* FIXME: Yes, I knwow... Ugly cast... */
  bool open_ok = audio_dev->open((AudioDevice::Mode)mode);
  if (open_ok)
  {
    io_mode = mode;
  }
  
  input_valve->setOpen(true);

  return open_ok;
  
} /* AudioIO::open */


void AudioIO::close(void)
{
  if (io_mode == MODE_NONE)
  {
    return;
  }
  
  io_mode = MODE_NONE;
  
  input_valve->setOpen(false);

  audio_dev->close(); 
  
} /* AudioIO::close */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Private member functions used by friend class AudioDevice
 *
 ****************************************************************************/

int AudioIO::readSamples(float *samples, int count)
{
  //printf("AudioIO[%s:%d]::readSamples\n", audio_dev->devName().c_str(), m_channel);

  int samples_read = audio_reader->readSamples(samples, count);
  
  //printf("AudioIO[%s:%d]::readSamples: count=%d  sample_read=%d\n", audio_dev->devName().c_str(), m_channel, count, samples_read);

  if (m_gain != 1.0)
  {
    for (int i=0; i<samples_read; ++i)
    {
      samples[i] = m_gain * samples[i];
    }
  }
  
  /*
  if (do_flush)
  {
    if (write_fifo->samplesInFifo() < 100)
    {
      int samples_left = write_fifo->samplesInFifo() + samples_read;
      int pos = max(0, 100 - samples_left);
      int start_pos = max(0, samples_left - 100);
      for (int i=start_pos; i<samples_read; i++)
      {
      	float fade_gain = pow(2, (100.0 - pos - (i - start_pos)) / 10.0)
	      	      / pow(2, 10.0);
      	samples[i] = fade_gain * samples[i];
      }
    }
  }
  
  if (write_fifo->empty() && do_flush)
  {
    flushSamplesInDevice(samples_read);
  }
  */
  
  return samples_read;
  
} /* AudioIO::readSamples */


bool AudioIO::isFlushing(void) const
{
  //printf("AudioIO::isFlushing\n");
  return audio_reader->isFlushing();
} /* AudioIO::isFlushing */


bool AudioIO::isIdle(void) const
{
  //printf("AudioIO::isIdle\n");
  return audio_reader->isIdle();
} /* AudioIO::isIdle */


int AudioIO::audioRead(float *samples, int count)
{
  return sinkWriteSamples(samples, count);
} /* AudioIO::audioRead */



/*
 * This file has not been truncated
 */

