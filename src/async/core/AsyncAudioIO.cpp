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

#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <cassert>
#include <cerrno>


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

#include "AsyncSampleFifo.h"
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
    flush_timer(0), is_flushing(false)
{
  write_fifo = new SampleFifo(8000);
  write_fifo->setOverwrite(false);
  write_fifo->stopOutput(true);
  write_fifo->fifoFull.connect(writeBufferFull.slot());
  audio_dev = AudioDevice::registerAudioIO(dev_name, this);
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
      read_con = audio_dev->audioRead.connect(audioRead.slot());
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


int AudioIO::write(short *samples, int count)
{
  assert((io_mode == MODE_WR) || (io_mode == MODE_RDWR));
  
  if (do_flush)
  {
    delete flush_timer;
    flush_timer = 0;
    do_flush = false;
    is_flushing = false;
  }
  
  int samples_written = write_fifo->addSamples(samples, count);
  audio_dev->audioToWriteAvailable();
  return samples_written;
  
} /* AudioIO::write */


int AudioIO::samplesToWrite(void) const
{
  return write_fifo->samplesInFifo() + audio_dev->samplesToWrite();
}


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


void AudioIO::clearSamples(void)
{
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
  write_fifo->clear();
  audio_dev->flushSamples();
  flushSamplesInDevice();
} /* AudioIO::clearSamples */




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
  flush_timer->expired.connect(slot(this, &AudioIO::flushDone));
} /* AudioIO::flushSamplesInDevice */


void AudioIO::flushDone(Timer *timer)
{
  //printf("ALL SAMPLES FLUSHED\n");
  is_flushing = false;
  delete flush_timer;
  flush_timer = 0;
  allSamplesFlushed();
} /* AudioIO::writeFileDone */


int AudioIO::readSamples(short *samples, int count)
{
  if (write_fifo->empty())
  {
    return 0;
  }
  
  int samples_read = write_fifo->readSamples(samples, count);
  if (write_fifo->empty() && do_flush)
  {
    flushSamplesInDevice(samples_read);
  }
  
  return samples_read;
  
} /* AudioIO::readSamples */


/*
 * This file has not been truncated
 */

