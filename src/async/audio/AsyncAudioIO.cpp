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

#include "SampleFifo.h"
#include "AsyncAudioDevice.h"
#include "AsyncFdWatch.h"
#include "AsyncAudioIO.h"



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



AudioIO::AudioIO(const string& dev_name)
  : io_mode(MODE_NONE), audio_dev(0), write_fifo(0), do_flush(true),
    flush_timer(0), is_flushing(false), lead_in_pos(0), m_gain(1.0)
{
  audio_dev = AudioDevice::registerAudioIO(dev_name, this);
  sample_rate = audio_dev->sampleRate();

  write_fifo = new SampleFifo(513);
  write_fifo->setOverwrite(false);
  write_fifo->stopOutput(true);
  write_fifo->fifoFull.connect(slot(*this, &AudioIO::fifoBufferFull));
  //write_fifo->fifoFull.connect(writeBufferFull.slot());
  /*
  audio_dev->writeBufferFull.connect(
      	  slot(write_fifo, &SampleFifo::writeBufferFull));
  */
} /* AudioIO::AudioIO */


AudioIO::~AudioIO(void)
{
  close();
  delete write_fifo;
  AudioDevice::unregisterAudioIO(this);
} /* AudioIO::~AudioIO */


bool AudioIO::isFullDuplexCapable(void)
{
  return audio_dev->isFullDuplexCapable();
} /* AudioIO::isFullDuplexCapable */


bool AudioIO::open(Mode mode)
{
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
    if ((io_mode == MODE_RD) || (io_mode == MODE_RDWR))
    {
      read_con = audio_dev->audioRead.connect(slot(*this, &AudioIO::audioRead));
    }
  }
  
  return open_ok;
  
} /* AudioIO::open */


void AudioIO::close(void)
{
  if (io_mode == MODE_NONE)
  {
    return;
  }
  
  if ((io_mode == MODE_RD) || (io_mode == MODE_RDWR))
  {
    read_con.disconnect();
  }
  
  io_mode = MODE_NONE;
  
  audio_dev->close(); 
  
  write_fifo->clear();
  
  do_flush = true;
  is_flushing = false;
  
  delete flush_timer;
  flush_timer = 0;
  
} /* AudioIO::close */


#if 0
int AudioIO::write(float *samples, int count)
{
  assert((io_mode == MODE_WR) || (io_mode == MODE_RDWR));
  
  if (do_flush)
  {
    delete flush_timer;
    flush_timer = 0;
    do_flush = false;
    is_flushing = false;
    lead_in_pos = 0;
  }
  
  float *buf = samples;;
  if (lead_in_pos < 100)
  {
    buf = new float[count];
    int cnt = min(count, 100-lead_in_pos);
    for (int i=0; i<cnt; ++i)
    {
      float fade_gain = pow(2, lead_in_pos++ / 10.0) / pow(2, 10.0);
      //printf("gain=%.4f  lead_in_pos=%d\n", gain, lead_in_pos);
      buf[i] = fade_gain * samples[i];
    }
    for (int i=cnt; i<count; ++i)
    {
      buf[i] = samples[i];
    }
  }
  
  int samples_written = write_fifo->addSamples(buf, count);
  
  if (buf != samples)
  {
    delete [] buf;
  }
  
  audio_dev->audioToWriteAvailable();
  return samples_written;
  
} /* AudioIO::write */
#endif


int AudioIO::samplesToWrite(void) const
{
  return write_fifo->samplesInFifo() + audio_dev->samplesToWrite();
}


#if 0
void AudioIO::flushSamples(void)
{
  //printf("AudioIO::flushSamples\n");
  
  if (do_flush)
  {
    if (!is_flushing)
    {
      allSamplesFlushed();
    }
    return;
  }
  
  do_flush = true;
  is_flushing = true;
  audio_dev->flushSamples();
  if (write_fifo->empty())
  {
    flushSamplesInDevice();
  }
} /* AudioIO::flushSamples */
#endif


void AudioIO::clearSamples(void)
{
  if (do_flush)
  {
    if (!is_flushing)
    {
      sourceAllSamplesFlushed();
    }
    return;
  }
  
  do_flush = true;
  is_flushing = true;
  write_fifo->clear();
  audio_dev->flushSamples();
  flushSamplesInDevice();
} /* AudioIO::clearSamples */


int AudioIO::writeSamples(const float *samples, int count)
{
  assert((io_mode == MODE_WR) || (io_mode == MODE_RDWR));
  
  if (do_flush)
  {
    delete flush_timer;
    flush_timer = 0;
    do_flush = false;
    is_flushing = false;
    lead_in_pos = 0;
  }
  
  //float *buf = samples;
  float buf[count];
  int cnt = 0;
  if (lead_in_pos < 100)
  {
    //buf = new float[count];
    cnt = min(count, 100-lead_in_pos);
    for (int i=0; i<cnt; ++i)
    {
      float fade_gain = pow(2, lead_in_pos++ / 10.0) / pow(2, 10.0);
      //printf("gain=%.4f  lead_in_pos=%d\n", gain, lead_in_pos);
      buf[i] = fade_gain * samples[i];
    }
  }
  
  for (int i=cnt; i<count; ++i)
  {
    buf[i] = samples[i];
  }

  int samples_written = write_fifo->addSamples(buf, count);
  
  /*
  if (buf != samples)
  {
    delete [] buf;
  }
  */
  
  audio_dev->audioToWriteAvailable();
  return samples_written;
  
} /* AudioIO::writeSamples */


void AudioIO::flushSamples(void)
{
  if (do_flush)
  {
    if (!is_flushing)
    {
      sourceAllSamplesFlushed();
    }
    return;
  }
  
  do_flush = true;
  is_flushing = true;
  audio_dev->flushSamples();
  if (write_fifo->empty())
  {
    flushSamplesInDevice();
  }
} /* AudioIO::flushSamples */






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


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void AudioIO::flushSamplesInDevice(int extra_samples)
{
  long flushtime = 1000 * audio_dev->samplesToWrite() / 8000;
  //printf("flushtime=%ld\n", flushtime);
  delete flush_timer;
  flush_timer = new Timer(flushtime);
  flush_timer->expired.connect(slot(*this, &AudioIO::flushDone));
} /* AudioIO::flushSamplesInDevice */


void AudioIO::flushDone(Timer *timer)
{
  //printf("ALL SAMPLES FLUSHED\n");
  is_flushing = false;
  delete flush_timer;
  flush_timer = 0;
  sourceAllSamplesFlushed();
} /* AudioIO::writeFileDone */


int AudioIO::readSamples(float *samples, int count)
{
  if (write_fifo->empty())
  {
    return 0;
  }
  
  int samples_read = write_fifo->readSamples(samples, count);
  
  if (m_gain != 1.0)
  {
    for (int i=0; i<samples_read; ++i)
    {
      samples[i] = m_gain * samples[i];
    }
  }
  
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
	/*
	printf("gain=%.3f  samples_in_fifo=%d  samples_read=%d  pos=%d "
	       "start_pos=%d  i=%d\n",
	       gain, write_fifo->samplesInFifo(), samples_read, pos,
	       start_pos, i);
	*/
      	samples[i] = fade_gain * samples[i];
      }
    }
  }
  
  if (write_fifo->empty() && do_flush)
  {
    flushSamplesInDevice(samples_read);
  }
  
  return samples_read;
  
} /* AudioIO::readSamples */


int AudioIO::audioRead(float *samples, int count)
{
  return sinkWriteSamples(samples, count);
} /* AudioIO::audioRead */


void AudioIO::fifoBufferFull(bool is_full)
{
  if (!is_full)
  {
    sourceResumeOutput();
  }
} /* AudioIO::fifoBufferFull */




/*
 * This file has not been truncated
 */

