/**
@file	 AsyncAudioDevice.cpp
@brief   Handle audio devices
@author  Tobias Blomberg
@date	 2004-03-20

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cassert>
#include <algorithm>


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
#include "AsyncFdWatch.h"
#include "AsyncAudioIO.h"
#include "AsyncAudioDevice.h"



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

map<string, AudioDevice*>  AudioDevice::devices;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AudioDevice *AudioDevice::registerAudioIO(const string& dev_name,
      	AudioIO *audio_io)
{
  if (devices.count(dev_name) == 0)
  {
    devices[dev_name] = new AudioDevice(dev_name);
  }
  AudioDevice *dev = devices[dev_name];
  ++dev->use_count;
  dev->aios.push_back(audio_io);
  return dev;
  
} /* AudioDevice::registerAudioIO */


void AudioDevice::unregisterAudioIO(AudioIO *audio_io)
{
  AudioDevice *dev = audio_io->device();
  assert(dev->use_count > 0);
  
  list<AudioIO*>::iterator it =
      	  find(dev->aios.begin(), dev->aios.end(), audio_io);
  assert(it != dev->aios.end());
  dev->aios.erase(it);
  
  if (--dev->use_count == 0)
  {
    delete dev;
  }
} /* AudioDevice::unregisterAudioIO */



bool AudioDevice::open(const Mode& mode)
{
  if (mode == current_mode)
  {
    return true;
  }
  
  if (current_mode != MODE_NONE)
  {
    return false;
  }
  
  int arg;
  
  if (fd != -1)
  {
    close();
  }
  
  int flags = 0; // = O_NONBLOCK; // Not supported according to the OSS docs
  switch (mode)
  {
    case MODE_RD:
      flags |= O_RDONLY;
      break;
    case MODE_WR:
      flags |= O_WRONLY;
      break;
    case MODE_RDWR:
      flags |= O_RDWR;
      break;
    case MODE_NONE:
      return true;
  }
  
  fd = ::open(dev_name.c_str(), flags);
  if(fd < 0)
  {
    perror("open failed");
    return false;
  }

  if (mode == MODE_RDWR)
  {
    ioctl(fd, SNDCTL_DSP_SETDUPLEX, 0);
  }
  
  int caps;
  if (ioctl(fd, SNDCTL_DSP_GETCAPS, &caps) == -1)
  {
    perror("SNDCTL_DSP_GETCAPS ioctl failed");
    close();
    return false;
  }
  printf("The sound device do%s have TRIGGER capability\n",
      (caps & DSP_CAP_TRIGGER) ? "" : " NOT");
  
  if (caps & DSP_CAP_TRIGGER)
  {
    arg = ~(PCM_ENABLE_OUTPUT | PCM_ENABLE_INPUT);
    if(ioctl(fd, SNDCTL_DSP_SETTRIGGER, &arg) == -1)
    {
      perror("SNDCTL_DSP_SETTRIGGER ioctl failed");
      close();
      return false;
    }
  }
  
  /*
  arg  = (FRAG_COUNT << 16) | FRAG_SIZE_LOG2;
  if (ioctl(fd, SNDCTL_DSP_SETFRAGMENT, &arg) == -1)
  {
    perror("SNDCTL_DSP_SETFRAGMENT ioctl failed");
    close();
    return false;
  }
  */
  
  /*
  arg = SIZE;
  if(ioctl(fd, SOUND_PCM_WRITE_BITS, &arg) == -1)
  {
    perror("SOUND_PCM_WRITE_BITS ioctl failed");
    close();
    return false;
  }
  if(arg != SIZE)
  {
    perror("unable to set sample size");
    close();
    return false;
  }
  */

  arg = AFMT_S16_LE; 
  if(ioctl(fd,  SNDCTL_DSP_SETFMT, &arg) == -1)
  {
    perror("SNDCTL_DSP_SETFMT ioctl failed");
    close();
    return false;
  }
  if (arg != AFMT_S16_LE)
  {
    fprintf(stderr, "*** error: The sound device does not support 16 bit "
      	      	    "signed samples\n");
    close();
    return false;
  }
  
  arg = CHANNELS;
  if(ioctl(fd, SNDCTL_DSP_CHANNELS, &arg) == -1)
  {
    perror("SNDCTL_DSP_CHANNELS ioctl failed");
    close();
    return false;
  }
  if(arg != CHANNELS)
  {
    fprintf(stderr, "*** error: Unable to set number of channels to %d. The "
      	      	    "driver suggested %d channels\n",
		    CHANNELS, arg);
    close();
    return false;
  }

  arg = RATE;
  if(ioctl(fd, SNDCTL_DSP_SPEED, &arg) == -1)
  {
    perror("SNDCTL_DSP_SPEED ioctl failed");
    close();
    return false;
  }
  if (abs(arg - RATE) > 100)
  {
    fprintf(stderr, "*** error: Sampling speed could not be set to %dHz. "
      	      	    "The closest speed returned by the driver was %dHz\n",
		    RATE, arg);
    close();
    return false;
  }
  printf("Reported sampling rate: %dHz\n", arg);
  
  /*  Used for playing non-full fragments.
  if (ioctl(fd, SNDCTL_DSP_POST, 0) == -1)
  {
    perror("SNDCTL_DSP_POST ioctl failed");
    close();
    return false;
  }
  */
  
  current_mode = mode;
  
  arg = 0;
  if ((mode == MODE_RD) || (mode == MODE_RDWR))
  {
    read_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
    assert(read_watch != 0);
    read_watch->activity.connect(slot(this, &AudioDevice::audioReadHandler));
    arg |= PCM_ENABLE_INPUT;
  }
  
  if ((mode == MODE_WR) || (mode == MODE_RDWR))
  {
    write_watch = new FdWatch(fd, FdWatch::FD_WATCH_WR);
    assert(write_watch != 0);
    write_watch->activity.connect(
      	    slot(this, &AudioDevice::writeSpaceAvailable));
    write_watch->setEnabled(false);
    arg |= PCM_ENABLE_OUTPUT;
  }
  
  if (caps & DSP_CAP_TRIGGER)
  {
    if(ioctl(fd, SNDCTL_DSP_SETTRIGGER, &arg) == -1)
    {
      perror("SNDCTL_DSP_SETTRIGGER ioctl failed");
      close();
      return false;
    }
  }
      
  int frag_size;
  if (ioctl(fd, SNDCTL_DSP_GETBLKSIZE, &frag_size) == -1)
  {
    perror("SNDCTL_DSP_GETBLKSIZE ioctl failed");
    close();
    return false;
  }
  printf("frag_size=%d\n", frag_size);
  
  read_buf = new char[BUF_FRAG_COUNT*frag_size];
  
  return true;
    
} /* AudioDevice::open */


void AudioDevice::close(void)
{
  current_mode = MODE_NONE;
  
  delete write_watch;
  write_watch = 0;
  
  delete read_watch;
  read_watch = 0;
  
  delete [] read_buf;
  read_buf = 0;
  
  if (fd != -1)
  {
    ::close(fd);
    fd = -1;
  }
} /* AudioDevice::close */


void AudioDevice::audioToWriteAvailable(void)
{
  //printf("AudioDevice::audioToWriteAvailable\n");
  
  if (write_watch->isEnabled())
  {
    return;
  }
  
  write_watch->setEnabled(true);
  
} /* AudioDevice::audioToWriteAvailable */


int AudioDevice::samplesToWrite(void) const
{
  audio_buf_info info;
  if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
  {
    perror("SNDCTL_DSP_GETOSPACE ioctl failed");
    return -1;
  }
  
  return (info.fragsize * (info.fragstotal - info.fragments)) / sizeof(short);
  
} /* AudioDevice::samplesToWrite */



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
AudioDevice::AudioDevice(const string& dev_name)
  : dev_name(dev_name), use_count(0), current_mode(MODE_NONE), fd(-1),
    read_watch(0), write_watch(0), read_buf(0)
{

} /* AudioDevice::AudioDevice */


AudioDevice::~AudioDevice(void)
{
  
} /* AudioDevice::~AudioDevice */






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
void AudioDevice::audioReadHandler(FdWatch *watch)
{
  audio_buf_info info;
  if (ioctl(fd, SNDCTL_DSP_GETISPACE, &info) == -1)
  {
    perror("SNDCTL_DSP_GETISPACE ioctl failed");
    return;
  }
  //printf("fragments=%d  fragstotal=%d  fragsize=%d  bytes=%d\n",
    //  	 info.fragments, info.fragstotal, info.fragsize, info.bytes);
  
  if (info.fragments > 0)
  {
    int frags_to_read = info.fragments > BUF_FRAG_COUNT ?
      	    BUF_FRAG_COUNT : info.fragments;
    int cnt = read(fd, read_buf, frags_to_read * info.fragsize);
    if (cnt == -1)
    {
      perror("read in AudioDevice::audioReadHandler");
      return;
    }

    audioRead(reinterpret_cast<short *>(read_buf), cnt/sizeof(short));
  }
    
} /* AudioDevice::audioReadHandler */


static inline int min(int a, int b)
{
  return (a < b) ? a : b;
}


static inline int max(int a, int b)
{
  return (a > b) ? a : b;
}


void AudioDevice::writeSpaceAvailable(FdWatch *watch)
{
  //printf("AudioDevice::writeSpaceAvailable\n");
  
  //writeBufferFull(false);
  
  assert(fd >= 0);
  assert((current_mode == MODE_WR) || (current_mode == MODE_RDWR));
  
  unsigned samples_to_write ;
  audio_buf_info info;
  do
  {
    samples_to_write = 0;
    list<AudioIO*>::iterator it;
    for (it=aios.begin(); it!=aios.end(); ++it)
    {
      if (((*it)->mode == AudioIO::MODE_WR) ||
      	  ((*it)->mode == AudioIO::MODE_RDWR))
      {
	SampleFifo &fifo = (*it)->writeFifo();
	samples_to_write = max(samples_to_write, fifo.samplesInFifo());
      }
    }
    
    if (samples_to_write == 0)
    {
      watch->setEnabled(false);
      return;
    }

    short buf[32768];
    memset(buf, 0, sizeof(buf));
    samples_to_write = min(samples_to_write, sizeof(buf));

    if (ioctl(fd, SNDCTL_DSP_GETOSPACE, &info) == -1)
    {
      perror("SNDCTL_DSP_GETOSPACE ioctl failed");
      return;
    }
    /*
    printf("fragments=%d  fragstotal=%d  fragsize=%d  bytes=%d\n",
      	   info.fragments, info.fragstotal, info.fragsize, info.bytes);  

    int frags_to_write = sizeof(*buf) * count / info.fragsize;
    */
    samples_to_write = min(samples_to_write, info.bytes / sizeof(short));
    
    if (samples_to_write == 0)
    {
      break;
    }
    
    for (it=aios.begin(); it!=aios.end(); ++it)
    {
      if (((*it)->mode == AudioIO::MODE_WR) ||
      	  ((*it)->mode == AudioIO::MODE_RDWR))
      {
	short tmp[sizeof(buf)];
	int samples_read = (*it)->readSamples(tmp, samples_to_write);
	for (int i=0; i<samples_read; ++i)
	{
      	  buf[i] += tmp[i];
	}
      }
    }  

    //printf("Writing %d samples to audio device...\n", samples_to_write);
    int written = ::write(fd, buf, samples_to_write * sizeof(*buf));
    if (written == -1)
    {
      perror("write in AudioIO::write");
      return;
    }

    assert(written / sizeof(short) == samples_to_write);
    
  } while(samples_to_write == info.bytes / sizeof(short));
    
  watch->setEnabled(true);
  
} /* AudioDevice::writeSpaceAvailable */









/*
 * This file has not been truncated
 */

